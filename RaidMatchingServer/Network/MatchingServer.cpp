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

    serverIOSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (serverIOSkt == INVALID_SOCKET) {
        std::cout << "Server Socket Make Fail" << std::endl;
        return false;
    }

    IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

    if (IOCPHandle == NULL) {
        std::cout << "Iocp Handle Make Fail" << std::endl;
        return false;
    }

    auto bIOCPHandle = CreateIoCompletionPort((HANDLE)serverIOSkt, IOCPHandle, (ULONG_PTR)this, 0);
    if (bIOCPHandle == nullptr) {
        std::cout << "Iocp Handle Bind Fail" << std::endl;
        return false;
    }

    overLappedManager = new OverLappedManager;
    overLappedManager->init();

    return true;
}

void MatchingServer::CenterServerConnect() {
    auto centerObj = connServersManager->FindUser(0);

    SOCKADDR_IN addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CENTER_SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    std::cout << "Connect To Center Server" << std::endl;

    connect(centerObj->GetSocket(), (SOCKADDR*)&addr, sizeof(addr));

    std::cout << "Connect Center Server Success" << std::endl;

    centerObj->ServerRecv();

    IM_MATCHING_REQUEST imReq;
    imReq.PacketId = (UINT16)PACKET_ID::IM_MATCHING_REQUEST;
    imReq.PacketLength = sizeof(IM_MATCHING_REQUEST);

    centerObj->PushSendMsg(sizeof(IM_MATCHING_REQUEST), (char*)&imReq);
}

void MatchingServer::GameServerConnect() {
    auto centerObj = connServersManager->FindUser(1);

    SOCKADDR_IN addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(GAME_SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    std::cout << "Connect To Center Server" << std::endl;

    connect(centerObj->GetSocket(), (SOCKADDR*)&addr, sizeof(addr));

    std::cout << "Connect Center Server Success" << std::endl;

    centerObj->ServerRecv();

    IM_MATCHING_REQUEST imReq;
    imReq.PacketId = (UINT16)PACKET_ID::IM_MATCHING_REQUEST;
    imReq.PacketLength = sizeof(IM_MATCHING_REQUEST);

    centerObj->PushSendMsg(sizeof(IM_MATCHING_REQUEST), (char*)&imReq);
}


bool MatchingServer::StartWork() {
    bool check = CreateWorkThread();
    if (!check) {
        std::cout << "WorkThread 생성 실패" << std::endl;
        return false;
    }

    connServersManager = new ConnServersManager(SERVER_COUNT);

    // 0 : Center Server
    ConnServer* connServer = new ConnServer(MAX_CIRCLE_SIZE, 0, IOCPHandle, overLappedManager);
    connServersManager->InsertUser(0, connServer);

    CenterServerConnect();

    auto imRes = reinterpret_cast<IM_MATCHING_RESPONSE*>(recvBuf);

    if (!imRes->isSuccess) {
        std::cout << "Fail to Connet" << std::endl;
        return false;
    }

    //// 1 : Game Server
    //ConnServer* connServer = new ConnServer(MAX_CIRCLE_SIZE, 1, IOCPHandle, overLappedManager);
    //connServersManager->InsertUser(1, connServer);

    //GameServerConnect();

    //auto imRes = reinterpret_cast<IM_MATCHING_RESPONSE*>(recvBuf);

    //if (!imRes->isSuccess) {
    //    std::cout << "Fail to Connet" << std::endl;
    //    return false;
    //}

    matchingManager = new MatchingManager;
    packetManager = new PacketManager;

    matchingManager->Init();
    packetManager->init(1);
}

bool MatchingServer::CreateWorkThread() {
    workRun = true;
    auto threadCnt = MaxThreadCnt; // core
    for (int i = 0; i < threadCnt; i++) {
        workThreads.emplace_back([this]() { WorkThread(); });
    }
    std::cout << "WorkThread Start" << std::endl;
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
            else if (connObjNum == 1) {
                std::cout << "Game Server 1 Disconnect" << std::endl;
            }
            continue;
        }

        if (overlappedEx->taskType == TaskType::RECV) {
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

void MatchingServer::ServerEnd() {
    workRun = false;

    for (int i = 0; i < workThreads.size(); i++) {
        PostQueuedCompletionStatus(IOCPHandle, 0, 0, nullptr);
    }

    for (int i = 0; i < workThreads.size(); i++) { // Work 쓰레드 종료
        if (workThreads[i].joinable()) {
            workThreads[i].join();
        }
    }

    delete packetManager;
    delete connServersManager;
    delete matchingManager;
    CloseHandle(IOCPHandle);
    closesocket(serverIOSkt);
    WSACleanup();

    std::cout << "종료 5초 대기" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 5초 대기
    std::cout << "종료" << std::endl;
}
