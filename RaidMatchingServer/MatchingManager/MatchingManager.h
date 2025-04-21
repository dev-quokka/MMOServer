#pragma once
#include <thread>
#include <set>
#include <tbb/concurrent_hash_map.h>

#include "Packet.h"
#include "MatchingConfig.h"
#include "OverLappedManager.h"
#include "ConnServersManager.h"

class MatchingManager {
public:
	~MatchingManager() {
		matchRun = false;

		for (int i = 0; i < matchingThreads.size(); i++) { // Shutdown matching threads
			if (matchingThreads[i].joinable()) {
				matchingThreads[i].join();
			}
		}

		for (int i = 1; i <= USER_LEVEL_GROUPS; i++) { // Delete user objects
			tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;

			if (matchingMap.find(accessor, i)) {
				for (auto tRoom : accessor->second) {
					delete tRoom;
				}

				accessor->second.clear();
			}
		}
	}

	// ====================== INITIALIZATION =======================
	bool Init(ConnServersManager* connServersManager_);


	// ===================== MATCHING MANAGEMENT =====================
	bool CreateMatchThread(uint16_t matchThreadCount_); // Creates match threads by evenly splitting user level groups based on the given thread count.
	void MatchingThread(uint16_t groupStartIdx_, uint16_t groupEndIdx_);

	uint16_t Insert(uint16_t userPk_, uint16_t userCenterObjNum, uint16_t userGroupNum_);
	uint16_t CancelMatching(uint16_t userCenterObjNum_, uint16_t userGroupNum_);
	void InserRoomNum(uint16_t roomNum_);

private:
	// 576 bytes
	tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>> matchingMap;

	// 136 bytes
	boost::lockfree::queue<uint16_t> roomNumQueue{ MAX_ROOM };

	// 80 bytes
	std::mutex mDeleteRoom;
	std::mutex mDeleteMatch;

	// 16 bytes
	std::vector<std::thread> matchingThreads;

	// 8 bytes
	ConnServersManager* connServersManager;

	// 1 bytes
	std::atomic<bool> matchRun;
};