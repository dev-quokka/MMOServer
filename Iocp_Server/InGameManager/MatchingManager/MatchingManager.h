#pragma once

#include <set>
#include <thread>
#include <chrono>
#include <queue>
#include <cstdint>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

#include "Define.h"
#include "RedisManager.h"
#include "ConnUsersManager.h"
#include "Room.h"
#include "RoomManager.h"
#include "InGameUserManager.h"

constexpr int UDP_PORT = 50000;
constexpr uint16_t USER_MAX_LEVEL = 15;

class RedisManager;

struct EndTimeComp {
	bool operator()(Room* r1, Room* r2) const {
		return r1->GetEndTime() > r2->GetEndTime();
	}
};

struct MatchingRoom {
	uint16_t userObjNum;
	InGameUser* inGameUser;
	std::chrono::time_point<std::chrono::steady_clock> insertTime = std::chrono::steady_clock::now();
	MatchingRoom(uint16_t userObjNum_, InGameUser* inGameUser_) : userObjNum(userObjNum_), inGameUser(inGameUser_) {}
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

		timeChekcRun = false;
		if (timeCheckThread.joinable()) {
			timeCheckThread.join();
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

	void Init(const uint16_t maxClientCount_, RedisManager* redisManager_, InGameUserManager* inGameUserManager_, RoomManager* roomManager_, ConnUsersManager* connUsersManager_);
	bool Insert(uint16_t userObjNum_, InGameUser* inGameUser_);
	bool CreateMatchThread();
	bool CreateTimeCheckThread();
	void MatchingThread();
	void TimeCheckThread();
	void DeleteMob(Room* room_);

private:
	// 1 bytes
	bool matchRun;
	bool timeChekcRun;

	// 8 bytes
	SOCKET udpSocket;
	InGameUserManager* inGameUserManager;
	RoomManager* roomManager;
	RedisManager* redisManager;
	ConnUsersManager* connUsersManager;

	// 16 bytes
	std::thread matchingThread;
	std::thread timeCheckThread;

	// 24 bytes
	std::set<Room*, EndTimeComp> endRoomCheckSet;
	// 80 bytes
	std::mutex mDeleteRoom;
	// 136 bytes
	boost::lockfree::queue<uint16_t> roomNumQueue{10}; // MaxClient set
	// 576 bytes
	tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>> matchingMap; // {Level/3 + 1 (0~2 = 1, 3~5 = 2 ...), UserSkt}
};