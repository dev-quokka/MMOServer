#include "MatchingServer.h"

bool MatchingServer::Init(const uint16_t MaxThreadCnt_, int port_) {
    WSAData wsadata;
    int check = 0;
    MaxThreadCnt = MaxThreadCnt_;

    check = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (check) {
        std::cout << "WSAStartup() Fail" << std::endl;
        return false;
    }

    serverSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (serverSkt == INVALID_SOCKET) {
        std::cout << "Server Socket Make Fail" << std::endl;
        return false;
    }

    SOCKADDR_IN addr;
    addr.sin_port = htons(port_);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    check = bind(serverSkt, (SOCKADDR*)&addr, sizeof(addr));
    if (check) {
        std::cout << "bind Fail:" << WSAGetLastError() << std::endl;
        return false;
    }

    check = listen(serverSkt, SOMAXCONN);
    if (check) {
        std::cout << "listen Fail" << std::endl;
        return false;
    }

    IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    if (IOCPHandle == NULL) {
        std::cout << "Iocp Handle Make Fail" << std::endl;
        return false;
    }

    auto bIOCPHandle = CreateIoCompletionPort((HANDLE)serverSkt, IOCPHandle, (ULONG_PTR)this, 0);
    if (bIOCPHandle == nullptr) {
        std::cout << "Iocp Handle Bind Fail" << std::endl;
        return false;
    }

    overLappedManager = new OverLappedManager;
    overLappedManager->init();

    return true;
}

bool MatchingServer::CenterServerConnect() {
    auto centerObj = connServersManager->FindUser(0);

    SOCKADDR_IN addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CENTER_SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    std::cout << "Connecting To Center Server" << std::endl;

    if (connect(centerObj->GetSocket(), (SOCKADDR*)&addr, sizeof(addr))) {
        std::cout << "Center Server Connect Fail" << std::endl;
        return false;
    }

    std::cout << "Center Server Connected" << std::endl;

    centerObj->ServerRecv();

    IM_MATCHING_REQUEST imReq;
    imReq.PacketId = (UINT16)PACKET_ID::IM_MATCHING_REQUEST;
    imReq.PacketLength = sizeof(IM_MATCHING_REQUEST);

    centerObj->PushSendMsg(sizeof(IM_MATCHING_REQUEST), (char*)&imReq);

    return true;
}

bool MatchingServer::StartWork() {
    bool check = CreateWorkThread();
    if (!check) {
        std::cout << "WorkThread 생성 실패" << std::endl;
        return false;
    }

    check = CreateAccepterThread();
    if (!check) {
        std::cout << "CreateAccepterThread 생성 실패" << std::endl;
        return false;
    }

    connServersManager = new ConnServersManager(SERVER_COUNT);

    matchingManager = new MatchingManager;
    packetManager = new PacketManager;

    matchingManager->Init(connServersManager);
    packetManager->init(1);

    // 0 : Center Server
    ConnServer* connServer = new ConnServer(MAX_CIRCLE_SIZE, 0, IOCPHandle, overLappedManager);
    connServersManager->InsertUser(0, connServer);

    CenterServerConnect();

    for (int i = 1; i < SERVER_COUNT; i++) { // Make ConnUsers Queue
        ConnServer* connServer = new ConnServer(MAX_CIRCLE_SIZE, i, IOCPHandle, overLappedManager);

        AcceptQueue.push(connServer); // Push ConnUser
        connServersManager->InsertUser(i, connServer); // Init ConnUsers
    }

    packetManager->SetManager(connServersManager, matchingManager);
}

bool MatchingServer::CreateWorkThread() {
    workRun = true;
    auto threadCnt = MaxThreadCnt;
    try {
        for (int i = 0; i < threadCnt; i++) {
            workThreads.emplace_back([this]() { WorkThread(); });
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Create Work Thread Failed : " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool MatchingServer::CreateAccepterThread() {
    AccepterRun = true;
    auto threadCnt = MaxThreadCnt / 4 + 1;

    try {
        for (int i = 0; i < threadCnt; i++) {
            acceptThreads.emplace_back([this]() { AccepterThread(); });
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Create Accept Thread Failed : " << e.what() << std::endl;
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
        connServer = connServersManager->FindUser(connObjNum);

        if (!gqSucces || (dwIoSize == 0 && overlappedEx->taskType != TaskType::ACCEPT)) { // Server Disconnect
            if (connObjNum == 0) {
                std::cout << "Center Server Disconnect" << std::endl;
            }
            continue;
        }
        if (overlappedEx->taskType == TaskType::ACCEPT) { // User Connect
            if (connServer->ServerRecv()) {
                std::cout << "socket " << connServer->GetSocket() << " Connect Request" << std::endl;
            }
            else { // Bind Fail
                connServer->Reset(); // Reset ConnUser
                AcceptQueue.push(connServer);
                std::cout << "socket " << connServer->GetSocket() << " Connect Fail" << std::endl;
            }
        }
        else if (overlappedEx->taskType == TaskType::RECV) {
            packetManager->PushPacket(connObjNum, dwIoSize, overlappedEx->wsaBuf.buf);
            connServer->ServerRecv(); // Wsarecv Again
            overLappedManager->returnOvLap(overlappedEx);
        }
        else if (overlappedEx->taskType == TaskType::NEWRECV) {
            packetManager->PushPacket(connObjNum, dwIoSize, overlappedEx->wsaBuf.buf);
            connServer->ServerRecv(); // Wsarecv Again
            delete[] overlappedEx->wsaBuf.buf;
            delete overlappedEx;
        }
        else if (overlappedEx->taskType == TaskType::SEND) {
            overLappedManager->returnOvLap(overlappedEx);
            connServer->SendComplete();
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

void MatchingServer::ServerEnd() {
    workRun = false;
    AccepterRun = false;

    for (int i = 0; i < workThreads.size(); i++) {
        PostQueuedCompletionStatus(IOCPHandle, 0, 0, nullptr);
    }

    for (int i = 0; i < workThreads.size(); i++) { // Work 쓰레드 종료
        if (workThreads[i].joinable()) {
            workThreads[i].join();
        }
    }

    for (int i = 0; i < acceptThreads.size(); i++) { // Accept 쓰레드 종료
        if (acceptThreads[i].joinable()) {
            acceptThreads[i].join();
        }
    }

    delete packetManager;
    delete connServersManager;
    delete matchingManager;

    CloseHandle(IOCPHandle);
    closesocket(serverSkt);
    WSACleanup();

    std::cout << "종료 5초 대기" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 5초 대기
    std::cout << "종료" << std::endl;
}
