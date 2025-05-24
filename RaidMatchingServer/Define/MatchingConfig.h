#pragma once
#include <cstdint>
#include <chrono>

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