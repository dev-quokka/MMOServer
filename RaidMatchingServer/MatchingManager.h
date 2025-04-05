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
#include <tbb/concurrent_hash_map.h>

#include "Packet.h"
#include "Define.h"
#include "OverLappedManager.h"

constexpr int UDP_PORT = 50000;
constexpr uint16_t USER_MAX_LEVEL = 15;
constexpr uint16_t MAX_ROOM = 10;
constexpr uint16_t IOCP_THREAD_CNT = 1;

struct MatchingRoom {
	uint16_t userObjNum;
	std::chrono::time_point<std::chrono::steady_clock> insertTime = std::chrono::steady_clock::now();
	MatchingRoom(uint16_t userObjNum_) : userObjNum(userObjNum_) {}
};

struct MatchingRoomComp {
	bool operator()(MatchingRoom* r1, MatchingRoom* r2) const {
		return r1->insertTime > r2->insertTime;
	}
};

class MatchingManager {
public:
	~MatchingManager() {
		matchRun = false;
		if (matchingThread.joinable()) {
			matchingThread.join();
		}

		for (int i = 1; i <= USER_MAX_LEVEL / 3 + 1; i++) {
			tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;

			if (matchingMap.find(accessor, i)) {
				for (auto tRoom : accessor->second) {
					delete tRoom;
				}

				accessor->second.clear();
			}
		}
	}

	bool RecvData();
	void PushSendMsg(const uint32_t dataSize_, char* sendMsg);
	void ProcSend();
	void SendComplete();

	bool Init(const uint16_t maxClientCount_);
	// void PushPacket(const uint32_t size_, char* recvData_);
	bool Insert(uint16_t userPk_, uint16_t userGroupNum_);
	bool CancelMatching(uint16_t userPk_, uint16_t userGroupNum_);
	bool CreateWorkThread();
	bool CreatePacketThread();
	bool CreateMatchThread();
	void WorkThread();
	void PacketThread();
	void MatchingThread();

private:
	// 576 bytes
	tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>> matchingMap; // {Level/3 + 1 (0~2 = 1, 3~5 = 2 ...), UserSkt}

	// 512 bytes
	char recvBuf[PACKET_SIZE] = {0};

	// 136 bytes
	boost::lockfree::queue<char*> procQueue{ 10 };
	boost::lockfree::queue<uint16_t> roomNumQueue{ 10 }; // MaxClient set
	boost::lockfree::queue<OverlappedEx*> sendQueue{ 10 };

	// 80 bytes
	std::mutex mDeleteRoom;
	std::mutex mDeleteMatch;

	// 16 bytes
	std::thread workThread;
	std::thread packetThread;
	std::thread matchingThread;

	// 8 bytes
	SOCKET serverIOSkt;
	HANDLE IOCPHandle;

	OverlappedEx* recvOvLap;
	OverlappedEx* sendOvLap;
	OverLappedManager* overlappedManager;

	// 2 bytes
	std::atomic<uint16_t> sendQueueSize{ 0 };

	// 1 bytes
	std::atomic<bool> workRun;
	std::atomic<bool> packetRun;
	std::atomic<bool> matchRun;
};