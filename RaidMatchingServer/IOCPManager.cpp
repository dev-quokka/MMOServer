#include "IOCPManager.h"

bool IOCPManager::Init() {
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

    SOCKADDR_IN addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    std::cout << "Connect To Center Server" << std::endl;

    connect(serverIOSkt, (SOCKADDR*)&addr, sizeof(addr));

    std::cout << "Connect Center Server Success" << std::endl;

    IM_MATCHING_REQUEST imReq;
    imReq.PacketId = (UINT16)MATCHING_ID::IM_MATCHING_REQUEST;
    imReq.PacketLength = sizeof(IM_MATCHING_REQUEST);

    send(serverIOSkt, (char*)&imReq, sizeof(imReq), 0);
    recv(serverIOSkt, recvBuf, PACKET_SIZE, 0);

    auto imRes = reinterpret_cast<IM_MATCHING_RESPONSE*>(recvBuf);

    if (!imRes->isSuccess) {
        std::cout << "Fail to Connet" << std::endl;
        return false;
    }

	matchingManager = new MatchingManager;
    overlappedManager = new OverLappedManager;
}

void IOCPManager::WorkThread() {
    LPOVERLAPPED lpOverlapped = NULL;
    DWORD dwIoSize = 0;
    bool gqSucces = TRUE;
    ULONG_PTR compKey = 0;

    while (workRun) {
        gqSucces = GetQueuedCompletionStatus(
            IOCPHandle,
            &dwIoSize,
            &compKey,
            &lpOverlapped,
            INFINITE
        );

        if (gqSucces && dwIoSize == 0 && lpOverlapped == NULL) { // Server End Request
            workRun = false;
            continue;
        }

        auto overlappedEx = (OverlappedEx*)lpOverlapped;
        
        if (overlappedEx->taskType == TaskType::RECV) {

        }
        else if (overlappedEx->taskType == TaskType::SEND) {

        }
        else if (overlappedEx->taskType == TaskType::NEWSEND) {

        }
		else if (overlappedEx->taskType == TaskType::NEWSEND) {
		
        }
    }
}
