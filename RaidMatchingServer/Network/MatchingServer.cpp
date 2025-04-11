#include "MatchingServer.h"

bool MatchingServer::Init() {
    WSAData wsadata;
    int check = 0;

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

    // 1 : Game Server
    ConnServer* connServer = new ConnServer(MAX_CIRCLE_SIZE, 1, IOCPHandle, overLappedManager);
    connServersManager->InsertUser(1, connServer);

    GameServerConnect();

    auto imRes = reinterpret_cast<IM_MATCHING_RESPONSE*>(recvBuf);

    if (!imRes->isSuccess) {
        std::cout << "Fail to Connet" << std::endl;
        return false;
    }

    matchingManager = new MatchingManager;
}

bool MatchingServer::CreateWorkThread() {
    workRun = true;
    workThread = std::thread([this]() {WorkThread(); });
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
            std::cout << "socket " << connServer->GetSocket() << " Disconnect" << std::endl;

            continue;
        }
        if (overlappedEx->taskType == TaskType::RECV) {
            redisManager->PushRedisPacket(connObjNum, dwIoSize, overlappedEx->wsaBuf.buf); // Proccess In Redismanager
            connServer->ServerRecv(); // Wsarecv Again
            overLappedManager->returnOvLap(overlappedEx);
        }
        else if (overlappedEx->taskType == TaskType::NEWRECV) {
            redisManager->PushRedisPacket(connObjNum, dwIoSize, overlappedEx->wsaBuf.buf); // Proccess In Redismanager
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
