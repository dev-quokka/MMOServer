#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

#include "RaidConfig.h"

class Room {
public:
	Room(SOCKET* udpSkt_) {
		ruInfos.resize(MAX_RAID_ROOM_PLAYERS + 1); // MAX_RAID_ROOM_PLAYERS + 1 to avoid using index 0
		udpSkt = udpSkt_;
	}
	~Room() {
		for (int i = 0; i < ruInfos.size(); i++) {
			delete ruInfos[i];
		}
	}

	//  ---------------------------- SET  ----------------------------

	bool Set(uint16_t roomNum_, int mobHp_) {
		roomNum = roomNum_;
		mobHp.store(mobHp_);
		return true;
	}

	bool SetUserConnObjNum(uint16_t userRaidServerObjNum_, uint16_t userConnObjNum_) { // Set user unique ID for use in Game Server
		if (userRaidServerObjNum_ == 1) ruInfos[1]->userConnObjNum = userConnObjNum_;
		else if (userRaidServerObjNum_ == 2) ruInfos[2]->userConnObjNum = userConnObjNum_;
		return true;
	}

	void SetSockAddr(uint16_t userRaidServerObjNum_, sockaddr_in userAddr_) { // Set UDP socket address received from user 
		ruInfos[userRaidServerObjNum_]->userAddr = userAddr_;
	}

	void SetMapNum(uint16_t mapNum_) {
		mapNum = mapNum_;
	}

	std::chrono::time_point<std::chrono::steady_clock> SetEndTime() { // Set raid end time
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		return endTime;
	}


	//  ---------------------------- GET  ----------------------------

	int GetMobHp() {
		return mobHp.load();
	}

	uint16_t GetRoomNum() {
		return roomNum;
	}

	uint16_t GetMapNum() {
		return mapNum;
	}

	RaidUserInfo* GetUserInfoByObjNum(uint16_t userRaidServerObjNum_) {
		return ruInfos[userRaidServerObjNum_];
	}

	unsigned int GetUserScoreByObjNum(uint16_t userRaidServerObjNum_) {
		return ruInfos[userRaidServerObjNum_]->userScore.load();
	}

	std::chrono::time_point<std::chrono::steady_clock> GetEndTime() {
		return endTime;
	}


	//  ---------------------------- END CHECK  ----------------------------

	void TimeOver() {
		finishCheck.store(true);
		mobHp.store(0);
		timeOver = true;
	}

	bool TimeOverCheck() {
		return timeOver;
	}

	uint16_t UserSetCheck(RaidUserInfo* raidUserInfo_) { // Check if all users are ready
		
		uint16_t tempNum = startCheck.fetch_add(1) + 1;

		raidUserInfo_->userRaidServerObjNum = tempNum;
		ruInfos[tempNum] = raidUserInfo_;

		return tempNum;
	}

	bool StartCheck() { // Check if all users are ready
		if (startCheck.fetch_add(1) + 1 == MAX_RAID_ROOM_PLAYERS) {
			endTime = std::chrono::steady_clock::now() + std::chrono::minutes(2) + std::chrono::seconds(8);
			return true;
		}
		return false;
	}

	bool EndCheck() { // Check if the room has already ended 
		if (startCheck.fetch_sub(1) - 1 == 0) {
			return true;
		}
		return false;
	}


	//  ---------------------------- RAID  ----------------------------

	void SendSyncMsg() { // Send sync messages to active players
		unsigned int tempMobHp = mobHp.load();
		for (int i = 1; i < ruInfos.size(); i++) { // 게임중인 유저들에게 동기화 메시지 전송
			sendto(*udpSkt, (char*)&tempMobHp, sizeof(tempMobHp), 0, (sockaddr*)&ruInfos[i]->userAddr, sizeof(ruInfos[i]->userAddr));
		}
		std::cout << "RoomNum : " << roomNum << ", Mob Hp : " << tempMobHp << std::endl;
	}

	std::pair<unsigned int, unsigned int> Hit(uint16_t userNum_, unsigned int damage_) { // {Current mobhp, Acquired score}
		if (mobHp <= 0 || finishCheck.load()) {
			return { 0,0 };
		}

		unsigned int score_;
		unsigned int currentMobHp_;

		if ((currentMobHp_ = mobHp.fetch_sub(damage_) - damage_) <= 0) { // Hit
			finishCheck.store(true);
			score_ = ruInfos[userNum_]->userScore.fetch_add(currentMobHp_ + damage_) + (currentMobHp_ + damage_);
			std::cout << "Mob already defeated, send acquired score" << std::endl;
			return { 0, score_ };
		}

		score_ = ruInfos[userNum_]->userScore.fetch_add(damage_) + damage_;

		return { currentMobHp_, score_ };
	}

private:
	// 32 bytes
	std::vector<RaidUserInfo*> ruInfos;

	// 8 bytes
	SOCKET* udpSkt;
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now() + std::chrono::seconds(WAITING_USERS_TIME); // Add buffer time to prevent instant room deletion

	// 4 bytes
	std::atomic<int> mobHp;

	// 2 bytes
	uint16_t roomNum;
	uint16_t mapNum;
	std::atomic<uint16_t> startCheck = 0;

	// 1 bytes
	std::atomic<bool> timeOver = false;
	std::atomic<bool> finishCheck = false;
};