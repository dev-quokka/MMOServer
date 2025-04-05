#pragma once
#pragma comment(lib, "ws2_32.lib") // 비주얼에서 소켓프로그래밍 하기 위한 것
#pragma comment(lib,"mswsock.lib") //AcceptEx를 사용하기 위한 것

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9090
#define PACKET_SIZE 512

#include <set>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <cstdint>
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <boost/lockfree/queue.hpp>

#include "Packet.h"
#include "Define.h"
#include "OverLappedManager.h"
#include "MatchingManager.h"

constexpr uint16_t IOCP_THREAD_CNT = 1;

class IOCPManager {
public:
	bool Init();
	bool CreateWorkThread();
	void WorkThread();

private:
	// 512 bytes
	char recvBuf[PACKET_SIZE];

	// 16 bytes
	std::thread workThread;

	// 8 bytes
	SOCKET serverIOSkt;
	HANDLE IOCPHandle;

	OverlappedEx* recvOvLap;
	OverlappedEx* sendOvLap;
	MatchingManager* matchingManager;
	OverLappedManager* overlappedManager;

	// 1 bytes
	bool workRun = false;
};
