#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "RaidUserInfo.h"

constexpr int waitingUsersTime = 30; // Time to wait for all users to be ready

class Room {
public:
	Room(SOCKET* udpSkt_) {
		ruInfos.resize(3); // // Add +1 to avoid using index 0
		udpSkt = udpSkt_;
	}
	~Room() {
		for (int i = 0; i < ruInfos.size(); i++) {
			delete ruInfos[i];
		}
	}


	//  ---------------------------- SET  ----------------------------

	bool Set(uint16_t roomNum_, uint16_t mapNum_, uint16_t timer_, int mobHp_, RaidUserInfo* raidUserInfo1, RaidUserInfo* raidUserInfo2) {
		RaidUserInfo* ruInfo1 = new RaidUserInfo;
		ruInfos[1] = raidUserInfo1;

		RaidUserInfo* ruInfo2 = new RaidUserInfo;
		ruInfos[2] = raidUserInfo2;

		mapNum = mapNum_;
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

	RaidUserInfo* GetMyInfo(uint16_t userRaidServerObjNum_) {
		if (userRaidServerObjNum_ == 1) return ruInfos[1];
		else if (userRaidServerObjNum_ == 2) return ruInfos[2];
	}

	RaidUserInfo* GetTeamInfo(uint16_t userRaidServerObjNum_) {
		if (userRaidServerObjNum_ == 2) return ruInfos[1];
		else if (userRaidServerObjNum_ == 1) return ruInfos[2];
	}

	unsigned int GetMyScore(uint16_t userRaidServerObjNum_) {
		if (userRaidServerObjNum_ == 1) return ruInfos[1]->userScore.load();
		else if (userRaidServerObjNum_ == 2) return ruInfos[2]->userScore.load();
	}

	unsigned int GetTeamScore(uint16_t userRaidServerObjNum_) {
		if (userRaidServerObjNum_ == 2) return ruInfos[1]->userScore.load();
		else if (userRaidServerObjNum_ == 1) return ruInfos[2]->userScore.load();
	}

	uint16_t GetRoomUserCnt() {
		return static_cast<uint16_t>(ruInfos.size());
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

	bool StartCheck() { // Check if all users are ready
		if (startCheck.fetch_add(1) + 1 == 2) {
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
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now() + std::chrono::seconds(waitingUsersTime); // Add buffer time to prevent instant room deletion

	// 4 bytes
	std::atomic<int> mobHp;
	char mobHpBuf[sizeof(unsigned int)];

	// 2 bytes
	uint16_t roomNum;
	uint16_t mapNum;

	// 1 bytes
	bool timeOver = false;
	std::atomic<bool> finishCheck = false;
	std::atomic<uint16_t> startCheck = 0;
};