#include "QuokkaServer.h"

bool QuokkaServer::init(const uint16_t MaxThreadCnt_, int port_) {
    WSADATA wsadata;
    int check = 0;
    MaxThreadCnt = MaxThreadCnt_; // 워크 스레드 개수 설정

    check = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (check) {
        std::cout << "WSAStartup 실패" << std::endl;
        return false;
    }

    serverSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (serverSkt == INVALID_SOCKET) {
        std::cout << "Server Socket 생성 실패" << std::endl;
        return false;
    }

    SOCKADDR_IN addr;
    addr.sin_port = htons(port_);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    check = bind(serverSkt, (SOCKADDR*)&addr, sizeof(addr));
    if (check) {
        std::cout << "bind 함수 실패:" << WSAGetLastError() <<std::endl;
        return false;
    }

    check = listen(serverSkt, SOMAXCONN);
    if (check) {
        std::cout << "listen 함수 실패" << std::endl;
        return false;
    }

    sIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxThreadCnt);
    if (sIOCPHandle == NULL) {
        std::cout << "iocp 핸들 생성 실패" << std::endl;
        return false;
    }

    auto bIOCPHandle = CreateIoCompletionPort((HANDLE)serverSkt, sIOCPHandle, (uint32_t)0, 0);
    if (bIOCPHandle == nullptr) {
        std::cout << "iocp 핸들 바인드 실패" << std::endl;
        return false;
    }

    overLappedManager = new OverLappedManager;
    overLappedManager->init();

    udpOverLappedManager = new UdpOverLappedManager;
    udpOverLappedManager->init();

    udpSkt = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (udpSkt == INVALID_SOCKET) {
        std::cout << "Server Socket 생성 실패" << std::endl;
        return false;
    }

    sockaddr_in udpAddr = {0};
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port = htons(UDP_PORT);
    udpAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSkt, (SOCKADDR*)&udpAddr, sizeof(udpAddr)) == SOCKET_ERROR) {
        std::cout << "UDP SOCKET BIND FAIL" << std::endl;
        closesocket(udpSkt);
    }

    udpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);

    auto result = CreateIoCompletionPort((HANDLE)udpSkt, udpHandle, (ULONG_PTR)udpSkt, 0);
    if (result == nullptr) {
        std::cout << "iocp UDP 핸들 바인드 실패" << std::endl;
        return false;
    }

    return true;
}

bool QuokkaServer::StartWork() {

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

    check = CreateUDPWorkThread();
    if (!check) {
        std::cout << "CreateAccepterThread 생성 실패" << std::endl;
        return false;
    }

    connUsersManager = new ConnUsersManager(maxClientCount);
    inGameUserManager = new InGameUserManager;
    roomManager = new RoomManager(&udpSkt, udpOverLappedManager);
    matchingManager = new MatchingManager;
    redisManager = new RedisManager;

    for (int i = 0; i < maxClientCount; i++) { // Make ConnUsers Queue
        ConnUser* connUser = new ConnUser(MAX_CIRCLE_SIZE,i, sIOCPHandle, overLappedManager);

        AcceptQueue.push(connUser); // Push ConnUser
        connUsersManager->InsertUser(i, connUser); // Init ConnUsers
    }

    for (int i = maxClientCount; i < maxClientCount*2; i++) { // Make Waitting Users Queue
        ConnUser* connUser = new ConnUser(MAX_CIRCLE_SIZE, i, sIOCPHandle, overLappedManager);

        WaittingQueue.push(connUser); // Push ConnUser
    }

    redisManager->init(MaxThreadCnt, maxClientCount, sIOCPHandle);// Run MySQL && Run Redis Threads (The number of Clsuter Master Nodes + 1)
    inGameUserManager->Init(maxClientCount);
    matchingManager->Init(maxClientCount, redisManager, inGameUserManager, roomManager, connUsersManager);
    redisManager->SetManager(connUsersManager, inGameUserManager, roomManager, matchingManager);

    return true;
}

bool QuokkaServer::CreateWorkThread() {
    WorkRun = true;
    auto threadCnt = MaxThreadCnt; // core
    for (int i = 0; i < threadCnt; i++) {
        workThreads.emplace_back([this]() { WorkThread(); });
    }
    std::cout << "WorkThread Start" << std::endl;
    return true;
}

bool QuokkaServer::CreateUDPWorkThread() {
    udpWorkRun = true;
    udpWorkThread = std::thread([this]() {UDPWorkThread(); });
    std::cout << "UDPWorkThread 시작" << std::endl;
    return true;
}

bool QuokkaServer::CreateAccepterThread() {
    AccepterRun = true;
    auto threadCnt = MaxThreadCnt/4+1; // (core/4)
    for (int i = 0; i < threadCnt; i++) {
        acceptThreads.emplace_back([this]() { AccepterThread(); });
    }
    std::cout << "AcceptThread Start" << std::endl;
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

        if (gqSucces && dwIoSize == 0 && lpOverlapped == NULL) { // Server End Request
            WorkRun = false;
            continue;
        }

        auto overlappedTCP = (OverlappedTCP*)lpOverlapped;
        uint16_t connObjNum = overlappedTCP->connObjNum;
        connUser = connUsersManager->FindUser(connObjNum);

        if (!gqSucces || (dwIoSize == 0 && overlappedTCP->taskType != TaskType::ACCEPT)) { // User Disconnect
            std::cout << "socket " << connUser->GetSocket() << " Disconnect" << std::endl;
            
            redisManager->Disconnect(connObjNum);
            inGameUserManager->Reset(connObjNum);
            connUser->Reset(); // Reset 
            UserCnt.fetch_sub(1); // UserCnt -1
            AcceptQueue.push(connUser);
            continue;
        }

        if (overlappedTCP->taskType == TaskType::ACCEPT) { // User Connect
                if (connUser->ConnUserRecv()) {
                    std::cout << "socket " << connUser->GetSocket() << " Connect Requset" << std::endl;
                    UserCnt.fetch_add(1); // UserCnt +1
                }
                else { // Bind Fail
                    connUser->Reset(); // Reset ConnUser
                    AcceptQueue.push(connUser);
                    std::cout << "socket " << connUser->GetSocket() << " ConnectFail" << std::endl;
                }
        }
        else if (overlappedTCP->taskType == TaskType::RECV) {
            redisManager->PushRedisPacket(connObjNum, dwIoSize, overlappedTCP->wsaBuf.buf); // Proccess In Redismanager
            connUser->ConnUserRecv(); // Wsarecv Again
            overLappedManager->returnOvLap(overlappedTCP);
        }
        else if (overlappedTCP->taskType == TaskType::NEWRECV) {
            redisManager->PushRedisPacket(connObjNum, dwIoSize, overlappedTCP->wsaBuf.buf); // Proccess In Redismanager
            connUser->ConnUserRecv(); // Wsarecv Again
            delete[] overlappedTCP->wsaBuf.buf;
            delete overlappedTCP;
        }
        else if (overlappedTCP->taskType == TaskType::SEND) {
            overLappedManager->returnOvLap(overlappedTCP);
            connUser->SendComplete();
        }
        else if (overlappedTCP->taskType == TaskType::NEWSEND) {
            delete[] overlappedTCP->wsaBuf.buf;
            delete overlappedTCP;
            connUser->SendComplete();
        }
    }
}

void QuokkaServer::UDPWorkThread() {
    LPOVERLAPPED lpOverlapped = NULL;
    DWORD dwIoSize = 0;
    ULONG_PTR completionKey;
    bool gqSucces = TRUE;

    while (udpWorkRun) {
        gqSucces = GetQueuedCompletionStatus(
            udpHandle,
            &dwIoSize,
            &completionKey,
            &lpOverlapped,
            INFINITE
        );

        if (gqSucces && dwIoSize == 0 && lpOverlapped == NULL) { // Server End Request
            udpWorkRun = false;
            continue;
        }

        auto overlappedUDP = (OverlappedUDP*)lpOverlapped;

        if (overlappedUDP->taskType == TaskType::SEND) {
            udpOverLappedManager->returnOvLap(overlappedUDP);
        }
        else if (overlappedUDP->taskType == TaskType::NEWSEND) {
            delete[] overlappedUDP->wsaBuf.buf;
            delete overlappedUDP;
        }
        else if (overlappedUDP->taskType == TaskType::RECV) { // 나중에 필요할때 추가 생성

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

void QuokkaServer::ServerEnd() {

    WorkRun = false;
    udpWorkRun = false;
    AccepterRun = false;


    for (int i = 0; i < workThreads.size(); i++) {
        PostQueuedCompletionStatus(sIOCPHandle, 0, 0, nullptr);
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

    PostQueuedCompletionStatus(udpHandle, 0, 0, nullptr);

    if (udpWorkThread.joinable()) {
        udpWorkThread.join();
    }

    ConnUser* connUser;

    while (WaittingQueue.pop(connUser)) { // 접속 대기큐 유저 객체 삭제
        closesocket(connUser->GetSocket());
        delete connUser;
    }

    delete redisManager;
    delete connUsersManager;
    delete inGameUserManager;
    delete roomManager;
    delete matchingManager;

    CloseHandle(sIOCPHandle); 
    CloseHandle(udpHandle);
    closesocket(serverSkt);
    closesocket(udpSkt);
    WSACleanup();

    std::cout << "종료 5초 대기" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 5초 대기
    std::cout << "종료" << std::endl;
}

