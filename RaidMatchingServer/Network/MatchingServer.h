#pragma once
#pragma comment(lib, "ws2_32.lib") 
#pragma comment(lib,"mswsock.lib")

#define CENTER_SERVER_IP "127.0.0.1"
#define CENTER_SERVER_PORT 9090

#define GAME_SERVER_IP "127.0.0.1"
#define GAME_SERVER_PORT 9501

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
#include "ConnServer.h"
#include "OverLappedManager.h"
#include "ConnServersManager.h"
#include "MatchingManager.h"
#include "PacketManager.h"

constexpr uint16_t IOCP_THREAD_CNT = 1;

class MatchingServer {
public:
	bool Init(const uint16_t MaxThreadCnt_, int port_);
	bool StartWork();
	bool CreateWorkThread();
	bool CreateAccepterThread();
	void WorkThread();
	void AccepterThread(); // Accept req Thread
	void ServerEnd();

	bool CenterServerConnect();
private:
	// 512 bytes
	char recvBuf[PACKET_SIZE];

	// 136 bytes 
	boost::lockfree::queue<OverlappedEx*> sendQueue{ 10 };
	boost::lockfree::queue<ConnServer*> AcceptQueue{ SERVER_COUNT }; // For Aceept User Queue

	// 32 bytes
	std::vector<std::thread> workThreads;
	std::vector<std::thread> acceptThreads;

	// 8 bytes
	SOCKET serverSkt;
	HANDLE IOCPHandle;

	OverLappedManager* overLappedManager;
	ConnServersManager* connServersManager;
	MatchingManager* matchingManager;
	PacketManager* packetManager;

	OverlappedEx* recvOvLap;
	OverlappedEx* sendOvLap;

	// 2 bytes
	uint16_t MaxThreadCnt = 0;

	// 1 bytes
	bool workRun = false;
	bool AccepterRun = false;
	std::atomic<uint16_t> sendQueueSize{ 0 };
};
