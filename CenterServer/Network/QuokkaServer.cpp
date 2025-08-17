#include "QuokkaServer.h"

// ========================== INITIALIZATION ===========================

bool QuokkaServer::init(const uint16_t MaxThreadCnt_, int port_) {
    WSADATA wsadata;
    MaxThreadCnt = MaxThreadCnt_; // Set the number of worker threads

    if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
        std::cout << "Failed to WSAStartup" << std::endl;
        return false;
    }

    serverSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (serverSkt == INVALID_SOCKET) {
        std::cout << "Failed to Create Server Socket" << std::endl;
        return false;
    }

    SOCKADDR_IN addr;
    addr.sin_port = htons(port_);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverSkt, (SOCKADDR*)&addr, sizeof(addr))) {
        std::cout << "Failed to Bind :" << WSAGetLastError() << std::endl;
        return false;
    }

    if (listen(serverSkt, SOMAXCONN)) {
        std::cout << "Failed to listen" << std::endl;
        return false;
    }

    sIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxThreadCnt);
    if (sIOCPHandle == NULL) {
        std::cout << "Failed to Create IOCP Handle" << std::endl;
        return false;
    }

    auto bIOCPHandle = CreateIoCompletionPort((HANDLE)serverSkt, sIOCPHandle, (uint32_t)0, 0);
    if (bIOCPHandle == nullptr) {
        std::cout << "Failed to Bind IOCP Handle" << std::endl;
        return false;
    }

    overLappedManager = new OverLappedManager;
    overLappedManager->init();

    return true;
}

bool QuokkaServer::StartWork() {
    if (!CreateWorkThread()) {
        return false;
    }

    if (!CreateAccepterThread()) {
        return false;
    }

    connUsersManager = new ConnUsersManager(maxClientCount);
    inGameUserManager = new InGameUserManager;
    redisManager = new RedisManager;

    // Cash Server : 11
    ConnUser* cashConnObj = new ConnUser(MAX_CIRCLE_SIZE, static_cast<uint16_t>(ServerType::CashServer), sIOCPHandle, overLappedManager);
    connUsersManager->InsertUser(static_cast<uint16_t>(ServerType::CashServer), cashConnObj);

    for (int i = 0; i < maxClientCount; i++) { // Create a user object
        if (i == 11) continue;

        ConnUser* connUser = new ConnUser(MAX_CIRCLE_SIZE,i, sIOCPHandle, overLappedManager);

        AcceptQueue.push(connUser);
        connUsersManager->InsertUser(i, connUser);
    }

    for (int i = maxClientCount; i < maxClientCount*2; i++) { // Create a waiting user object
        ConnUser* connUser = new ConnUser(MAX_CIRCLE_SIZE, i, sIOCPHandle, overLappedManager);

        WaittingQueue.push(connUser);
    }

    redisManager->init(MaxThreadCnt);// Run Redis Threads (The number of Clsuter Master Nodes + 1)
    inGameUserManager->Init(maxClientCount);
    redisManager->SetManager(connUsersManager, inGameUserManager);

    // CashServerConnect();

    return true;
}


void QuokkaServer::ServerEnd() {
    WorkRun = false;
    AccepterRun = false;

    for (int i = 0; i < workThreads.size(); i++) {
        PostQueuedCompletionStatus(sIOCPHandle, 0, 0, nullptr);
    }

    for (int i = 0; i < workThreads.size(); i++) { // Shutdown worker threads
        if (workThreads[i].joinable()) {
            workThreads[i].join();
        }
    }
    for (int i = 0; i < acceptThreads.size(); i++) { // Shutdown accept threads
        if (acceptThreads[i].joinable()) {
            acceptThreads[i].join();
        }
    }

    ConnUser* connUser;

    while (WaittingQueue.pop(connUser)) {
        closesocket(connUser->GetSocket());
        delete connUser;
    }

    delete redisManager;
    delete connUsersManager;
    delete inGameUserManager;

    CloseHandle(sIOCPHandle);
    closesocket(serverSkt);
    WSACleanup();

    std::cout << "Wait 5 Seconds Before Shutdown" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait 5 seconds before server shutdown
    std::cout << "Center Server Shutdown" << std::endl;
}


// ======================== SERVER CONNECTION ==========================

bool QuokkaServer::CashServerConnect() {
    auto centerObj = connUsersManager->FindUser(static_cast<uint16_t>(ServerType::CashServer));

    SOCKADDR_IN addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ServerAddressMap[ServerType::CashServer].port);
    inet_pton(AF_INET, ServerAddressMap[ServerType::CashServer].ip.c_str(), &addr.sin_addr.s_addr);

    std::cout << "Connecting to Cash Server" << std::endl;

    if (connect(centerObj->GetSocket(), (SOCKADDR*)&addr, sizeof(addr))) {
        std::cout << "Failed to Connect to Cash Server" << std::endl;
        return false;
    }

    std::cout << "Cash Server Connected" << std::endl;

    centerObj->ConnUserRecv();

    CASH_SERVER_CONNECT_REQUEST cashReq;
    cashReq.PacketId = (UINT16)PACKET_ID::CASH_SERVER_CONNECT_REQUEST;
    cashReq.PacketLength = sizeof(CASH_SERVER_CONNECT_REQUEST);

    centerObj->PushSendMsg(sizeof(CASH_SERVER_CONNECT_REQUEST), (char*)&cashReq);
    return true;
}


// ========================= THREAD MANAGEMENT =========================

bool QuokkaServer::CreateWorkThread() {
    WorkRun = true;

    try {
        auto threadCnt = MaxThreadCnt;
        for (int i = 0; i < threadCnt; i++) {
            workThreads.emplace_back([this]() { WorkThread(); });
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Failed to Create Work Threads : " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool QuokkaServer::CreateAccepterThread() {
    AccepterRun = true;

    try {
        auto threadCnt = MaxThreadCnt / 4 + 1;
        for (int i = 0; i < threadCnt; i++) {
            workThreads.emplace_back([this]() { AccepterThread(); });
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Failed to Create Accepter Threads : " << e.what() << std::endl;
        return false;
    }

    return true;
}

void QuokkaServer::WorkThread() {
    LPOVERLAPPED lpOverlapped = NULL;
    ConnUser* connUser = nullptr;
    DWORD dwIoSize = 0;
    bool gqSucces = TRUE;

    while (WorkRun) {
        gqSucces = GetQueuedCompletionStatus(
            sIOCPHandle,
            &dwIoSize,
            (PULONG_PTR)&connUser,
            &lpOverlapped,
            INFINITE
        );

        if (gqSucces && dwIoSize == 0 && lpOverlapped == NULL) {
            WorkRun = false;
            continue;
        }

        auto overlappedEx = (OverlappedEx*)lpOverlapped;
        uint16_t connObjNum = overlappedEx->connObjNum;
        connUser = connUsersManager->FindUser(connObjNum);

        if (!gqSucces || (dwIoSize == 0 && overlappedEx->taskType != TaskType::ACCEPT)) { // User Disconnected
            std::cout << "socket " << connUser->GetSocket() << " Disconnected" << std::endl;

            redisManager->Disconnect(connObjNum);
            inGameUserManager->Reset(connObjNum);
            connUser->Reset(); // Reset 
            UserCnt.fetch_sub(1); // UserCnt -1
            AcceptQueue.push(connUser);
            continue;
        }

        if (overlappedEx->taskType == TaskType::ACCEPT) { // User Connect
                if (connUser->ConnUserRecv()) {
                    std::cout << "socket " << connUser->GetSocket() << " Connection Requset" << std::endl;
                    UserCnt.fetch_add(1); // UserCnt +1
                }
                else { // Bind Fail
                    connUser->Reset(); // Reset ConnUser
                    AcceptQueue.push(connUser);
                    std::cout << "socket " << connUser->GetSocket() << " Connection Fail" << std::endl;
                }
        }
        else if (overlappedEx->taskType == TaskType::RECV) {
            redisManager->PushRedisPacket(connObjNum, dwIoSize, overlappedEx->wsaBuf.buf); // Proccess In Redismanager
            connUser->ConnUserRecv(); // Wsarecv Again
            overLappedManager->returnOvLap(overlappedEx);
        }
        else if (overlappedEx->taskType == TaskType::SEND) {
            overLappedManager->returnOvLap(overlappedEx);
            connUser->SendComplete();
        }
        else if (overlappedEx->taskType == TaskType::NEWRECV) {
            redisManager->PushRedisPacket(connObjNum, dwIoSize, overlappedEx->wsaBuf.buf); // Proccess In Redismanager
            connUser->ConnUserRecv(); // Wsarecv Again
            delete[] overlappedEx->wsaBuf.buf;
            delete overlappedEx;
        }
        else if (overlappedEx->taskType == TaskType::NEWSEND) {
            delete[] overlappedEx->wsaBuf.buf;
            delete overlappedEx;
            connUser->SendComplete();
        }
    }
}

void QuokkaServer::AccepterThread() {
    ConnUser* connUser;

    while (AccepterRun) {
        if (AcceptQueue.pop(connUser)) { // AcceptQueue not empty
            if (!connUser->PostAccept(serverSkt)) {
                AcceptQueue.push(connUser);
            }
        }
        else { // AcceptQueue empty
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}


