#pragma once
#pragma comment(lib, "ws2_32.lib") 
#pragma comment(lib,"mswsock.lib")

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
#include "OverLappedManager.h"
#include "ConnServersManager.h"

constexpr uint16_t MAX_RAID_ROOM_PLAYERS = 2;
constexpr uint16_t MATCHING_THREAD_COUNT = 2;
constexpr uint16_t MAX_ROOM = 10; // Max rooms per Game Server
constexpr uint16_t USER_LEVEL_GROUPS = 15 / 3 + 1; // To set the number of groups

struct MatchingRoom { // User data used for matching 
	std::chrono::time_point<std::chrono::steady_clock> insertTime = std::chrono::steady_clock::now();
	uint16_t userPk; 
	uint16_t userCenterObjNum;
	MatchingRoom(uint16_t userPk_, uint16_t userCenterObjNum_) : userPk(userPk_), userCenterObjNum(userCenterObjNum_) {}
};

struct MatchingRoomComp { // Comparator struct based on matchmaking start time
	bool operator()(MatchingRoom* r1, MatchingRoom* r2) const {
		return r1->insertTime > r2->insertTime;
	}
};

class MatchingManager {
public:
	~MatchingManager() {
		matchRun = false;

		for (int i = 0; i < matchingThreads.size(); i++) { // Shutdown matching threads
			if (matchingThreads[i].joinable()) {
				matchingThreads[i].join();
			}
		}

		for (int i = 1; i <= USER_LEVEL_GROUPS; i++) {
			tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;

			if (matchingMap.find(accessor, i)) {
				for (auto tRoom : accessor->second) {
					delete tRoom;
				}

				accessor->second.clear();
			}
		}
	}

	bool Init(ConnServersManager* connServersManager_);
	uint16_t Insert(uint16_t userPk_, uint16_t userCenterObjNum, uint16_t userGroupNum_);
	uint16_t CancelMatching(uint16_t userCenterObjNum_, uint16_t userGroupNum_);


	// Creates match threads by evenly splitting user level groups based on the given thread count.
	bool CreateMatchThread(uint16_t matchThreadCount_);
	void MatchingThread(uint16_t groupStartIdx_, uint16_t groupEndIdx_);

private:
	// 576 bytes
	tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>> matchingMap;

	// 512 bytes
	char recvBuf[PACKET_SIZE] = {0};

	// 136 bytes
	boost::lockfree::queue<char*> procQueue{ 10 };
	boost::lockfree::queue<uint16_t> roomNumQueue{ MAX_ROOM };
	boost::lockfree::queue<OverlappedEx*> sendQueue{ 10 };

	// 80 bytes
	std::mutex mDeleteRoom;
	std::mutex mDeleteMatch;

	// 16 bytes
	std::vector<std::thread> matchingThreads;

	// 8 bytes
	SOCKET serverIOSkt;
	HANDLE IOCPHandle;

	OverlappedEx* recvOvLap;
	OverlappedEx* sendOvLap;
	OverLappedManager* overlappedManager;
	ConnServersManager* connServersManager;

	// 2 bytes
	std::atomic<uint16_t> sendQueueSize{ 0 };

	// 1 bytes
	std::atomic<bool> matchRun;
};