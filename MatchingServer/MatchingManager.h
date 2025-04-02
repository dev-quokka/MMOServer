#pragma once

#include <set>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <cstdint>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

constexpr int UDP_PORT = 50000;
constexpr uint16_t USER_MAX_LEVEL = 15;
constexpr uint16_t MAX_ROOM = 10;

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

	void Init(const uint16_t maxClientCount_);
	bool Insert(uint16_t userObjNum_, uint16_t groupNum_);
	bool CancelMatching(uint16_t userObjNum_, uint16_t groupNum_);
	bool CreateMatchThread();
	void MatchingThread();

private:
	// 576 bytes
	tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>> matchingMap; // {Level/3 + 1 (0~2 = 1, 3~5 = 2 ...), UserSkt}

	// 136 bytes
	boost::lockfree::queue<uint16_t> roomNumQueue{ 10 }; // MaxClient set

	// 80 bytes
	std::mutex mDeleteRoom;
	std::mutex mDeleteMatch;

	// 16 bytes
	std::thread matchingThread;

	// 1 bytes
	std::atomic<bool> matchRun;
};