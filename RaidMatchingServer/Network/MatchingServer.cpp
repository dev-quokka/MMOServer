#include "MatchingServer.h"

// =========================== INITIALIZATION ===========================

bool MatchingServer::Init(const uint16_t MaxThreadCnt_, int port_) {
    WSAData wsadata;
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
        std::cout << "Failed to Bind : " << WSAGetLastError() << std::endl;
        return false;
    }

    if (listen(serverSkt, SOMAXCONN)) {
        std::cout << "Failed to listen" << std::endl;
        return false;
    }

    IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    if (IOCPHandle == NULL) {
        std::cout << "Failed to Create IOCP Handle" << std::endl;
        return false;
    }

    auto bIOCPHandle = CreateIoCompletionPort((HANDLE)serverSkt, IOCPHandle, (ULONG_PTR)this, 0);
    if (bIOCPHandle == nullptr) {
        std::cout << "Failed to Bind IOCP Handle" << std::endl;
        return false;
    }

    overLappedManager = new OverLappedManager;
    overLappedManager->init();

    return true;
}

bool MatchingServer::StartWork() {
    if (!CreateWorkThread()) {
        return false;
    }

    if (!CreateAccepterThread()) {
        return false;
    }

    connServersManager = new ConnServersManager(SERVER_COUNT);

    matchingManager = new MatchingManager;
    redisManager = new RedisManager;

    matchingManager->Init(connServersManager);
    redisManager->init(1);

    // 0 : Center Server
    ConnServer* connServer = new ConnServer(MAX_CIRCLE_SIZE, 0, IOCPHandle, overLappedManager);
    connServersManager->InsertServer(0, connServer);

    CenterServerConnect();

    for (int i = 1; i < SERVER_COUNT; i++) { // Make ConnUsers Queue
        ConnServer* connServer = new ConnServer(MAX_CIRCLE_SIZE, i, IOCPHandle, overLappedManager);

        AcceptQueue.push(connServer); // Push ConnUser
        connServersManager->InsertServer(i, connServer); // Init ConnUsers
    }

    redisManager->SetManager(connServersManager, matchingManager);
}

bool MatchingServer::CreateWorkThread() {
    workRun = true;

    try {
        for (int i = 0; i < MaxThreadCnt; i++) {
            workThreads.emplace_back([this]() { WorkThread(); });
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Failed to Create Work Thread : " << e.what() << std::endl;
        return false;
    }

    return true;
}

void MatchingServer::ServerEnd() {
    workRun = false;
    AccepterRun = false;

    for (int i = 0; i < workThreads.size(); i++) {
        PostQueuedCompletionStatus(IOCPHandle, 0, 0, nullptr);
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

    delete redisManager;
    delete connServersManager;
    delete matchingManager;

    CloseHandle(IOCPHandle);
    closesocket(serverSkt);
    WSACleanup();

    std::cout << "Wait 5 Seconds Before Shutdown" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait 5 seconds before server shutdown
    std::cout << "Game Server1 Shutdown" << std::endl;
}


// ========================== SERVER CONNECTION ==========================

bool MatchingServer::CenterServerConnect() {
    auto centerObj = connServersManager->FindServer(0);

    SOCKADDR_IN addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ServerAddressMap[ServerType::CenterServer].port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    std::cout << "Connecting To Center Server" << std::endl;

    if (connect(centerObj->GetSocket(), (SOCKADDR*)&addr, sizeof(addr))) {
        std::cout << "Failed to Connect to Center Server" << std::endl;
        return false;
    }

    std::cout << "Center Server Connected" << std::endl;

    centerObj->ServerRecv();

    MATCHING_SERVER_CONNECT_REQUEST imReq;
    imReq.PacketId = (UINT16)PACKET_ID::MATCHING_SERVER_CONNECT_REQUEST;
    imReq.PacketLength = sizeof(MATCHING_SERVER_CONNECT_REQUEST);

    centerObj->PushSendMsg(sizeof(MATCHING_SERVER_CONNECT_REQUEST), (char*)&imReq);

    return true;
}

bool MatchingServer::CreateAccepterThread() {
    AccepterRun = true;

    try {
        for (int i = 0; i < MaxThreadCnt / 4 + 1; i++) {
            acceptThreads.emplace_back([this]() { AccepterThread(); });
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Failed to Create Accepter Thread : " << e.what() << std::endl;
        return false;
    }

    return true;
}

void MatchingServer::WorkThread() {
    LPOVERLAPPED lpOverlapped = NULL;
    ConnServer* connServer = nullptr;
    DWORD dwIoSize = 0;
    bool gqSucces = TRUE;

    while (workRun) {
        gqSucces = GetQueuedCompletionStatus(
            IOCPHandle,
            &dwIoSize,
            (PULONG_PTR)&connServer,
            &lpOverlapped,
            INFINITE
        );

        if (gqSucces && dwIoSize == 0 && lpOverlapped == NULL) { // Server End Request
            workRun = false;
            continue;
        }

        auto overlappedEx = (OverlappedEx*)lpOverlapped;
        uint16_t connObjNum = overlappedEx->connObjNum;
        connServer = connServersManager->FindServer(connObjNum);

        if (!gqSucces || (dwIoSize == 0 && overlappedEx->taskType != TaskType::ACCEPT)) { // Server Disconnect
            std::cout << "socket " << connServer->GetSocket() << " Disconnected" << std::endl;
            
            if (connObjNum == 0) {
                std::cout << "========= Center Server Disconnected =========" << std::endl;
            }

            connServer->Reset(); // Reset 
            AcceptQueue.push(connServer);
            continue;
        }
        if (overlappedEx->taskType == TaskType::ACCEPT) { // User Connect
            if (connServer->ServerRecv()) {
                std::cout << "socket " << connServer->GetSocket() << " Connection Request" << std::endl;
            }
            else { // Bind Fail
                connServer->Reset(); // Reset ConnUser
                AcceptQueue.push(connServer);
                std::cout << "socket " << connServer->GetSocket() << " Connection Failed" << std::endl;
            }
        }
        else if (overlappedEx->taskType == TaskType::RECV) {
            redisManager->PushRedisPacket(connObjNum, dwIoSize, overlappedEx->wsaBuf.buf);
            connServer->ServerRecv(); // Wsarecv Again
            overLappedManager->returnOvLap(overlappedEx);
        }
        else if (overlappedEx->taskType == TaskType::SEND) {
            overLappedManager->returnOvLap(overlappedEx);
            connServer->SendComplete();
        }
        else if (overlappedEx->taskType == TaskType::NEWRECV) {
            redisManager->PushRedisPacket(connObjNum, dwIoSize, overlappedEx->wsaBuf.buf);
            connServer->ServerRecv(); // Wsarecv Again
            delete[] overlappedEx->wsaBuf.buf;
            delete overlappedEx;
        }
        else if (overlappedEx->taskType == TaskType::NEWSEND) {
            delete[] overlappedEx->wsaBuf.buf;
            delete overlappedEx;
            connServer->SendComplete();
        }
    }
}

void MatchingServer::AccepterThread() {
    ConnServer* connServer;

    while (AccepterRun) {
        if (AcceptQueue.pop(connServer)) { // AcceptQueue not empty
            if (!connServer->PostAccept(serverSkt)) {
                AcceptQueue.push(connServer);
            }
        }
        else { // AcceptQueue empty
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            //while (AccepterRun) {
            //    if (WaittingQueue.pop(connUser)) { // WaittingQueue not empty
            //        WaittingQueue.push(connUser);
            //    }
            //    else { // WaittingQueue empty
            //        std::this_thread::sleep_for(std::chrono::milliseconds(10));
            //        break;
            //    }
            //}
        }
    }
}
