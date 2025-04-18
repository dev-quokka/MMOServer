#pragma once
#pragma comment(lib, "ws2_32.lib") 
#pragma comment(lib,"mswsock.lib")

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <boost/lockfree/queue.hpp>

#include "Packet.h"
#include "Define.h"
#include "ServerEnum.h"
#include "ConnServer.h"
#include "OverLappedManager.h"
#include "ConnServersManager.h"
#include "MatchingManager.h"
#include "RedisManager.h"

class MatchingServer {
public:
	// ======================= INITIALIZATION =======================
	bool Init(const uint16_t MaxThreadCnt_, int port_);
	bool StartWork();
	void ServerEnd();


	// ====================== SERVER CONNECTION ======================
	bool CenterServerConnect();

private:
	// ===================== THREAD MANAGEMENT ======================
	bool CreateWorkThread();
	bool CreateAccepterThread();
	void WorkThread();
	void AccepterThread();


	// 136 bytes 
	boost::lockfree::queue<ConnServer*> AcceptQueue{ SERVER_COUNT };

	// 32 bytes
	std::vector<std::thread> workThreads;
	std::vector<std::thread> acceptThreads;

	// 8 bytes
	SOCKET serverSkt;
	HANDLE IOCPHandle;

	OverLappedManager* overLappedManager;
	ConnServersManager* connServersManager;
	MatchingManager* matchingManager;
	RedisManager* redisManager;

	// 2 bytes
	uint16_t MaxThreadCnt = 0;

	// 1 bytes
	std::atomic<bool> workRun = false;
	std::atomic<bool> AccepterRun = false;
};
