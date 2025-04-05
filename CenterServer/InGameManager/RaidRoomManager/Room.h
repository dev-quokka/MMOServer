#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Define.h"
#include "UdpOverLappedManager.h"

class MatchingManager;
class InGameUser;

struct RaidUserInfo {
	std::atomic<unsigned int> userScore = 0;
	uint16_t userObjNum; // TCP Socket
	sockaddr_in userAddr;
	InGameUser* inGameUser;
};

class Room {
public:
	Room() {
		RaidUserInfo* ruInfo1 = new RaidUserInfo;
		ruInfos.emplace_back(ruInfo1);

		RaidUserInfo* ruInfo2 = new RaidUserInfo;
		ruInfos.emplace_back(ruInfo2);
	}
	~Room() {
		for (int i = 0; i < ruInfos.size(); i++) {
			delete ruInfos[i];
		}
	}

	void set(uint16_t roomNum_, uint16_t timer_, int mobHp_, uint16_t userObjNum1_, uint16_t userObjNum2_, InGameUser* user1_, InGameUser* user2_) {
		ruInfos[0]->userObjNum = userObjNum1_;
		ruInfos[0]->inGameUser = user1_;

		ruInfos[1]->userObjNum = userObjNum2_;
		ruInfos[1]->inGameUser = user2_;

		mobHp.store(mobHp_);
	}

	void setSockAddr(uint16_t userNum_, sockaddr_in userAddr_) {
		ruInfos[userNum_]->userAddr = userAddr_;
	}

	void TimeOver() {
		finishCheck.store(true);
		mobHp.store(0);
		timeOver = true;
	}

	bool TimeOverCheck() {
		return timeOver;
	}

	bool StartCheck() {
		if (startCheck.fetch_add(1) + 1 == 2) {
			endTime = std::chrono::steady_clock::now() + std::chrono::minutes(2) + std::chrono::seconds(8);
			return true;
		}
		return false;
	}

	bool EndCheck() {
		if (startCheck.fetch_sub(1) - 1 == 0) {
			return true;
		}
		return false;
	}

	uint16_t GetRoomNum() {
		return roomNum;
	}

	uint16_t GetRoomUserCnt() {
		return ruInfos.size();
	}

	InGameUser* GetUser(uint16_t userNum_) {
		if (userNum_ == 0) return ruInfos[0]->inGameUser;
		else if (userNum_ == 1) return ruInfos[1]->inGameUser;
	}

	std::chrono::time_point<std::chrono::steady_clock> SetEndTime() {
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		return endTime;
	}

	std::chrono::time_point<std::chrono::steady_clock> GetEndTime() {
		return endTime;
	}

	SOCKET GetUserObjNum(uint16_t userNum) {
		if (userNum == 0) return ruInfos[0]->userObjNum;
		else if (userNum == 1) return ruInfos[1]->userObjNum;
	}

	unsigned int GetScore(uint16_t userNum) {
		if (userNum == 0) return ruInfos[0]->userScore;
		else if (userNum == 1) return ruInfos[1]->userScore;
	}

	SOCKET GetTeamObjNum(uint16_t userNum_) {
		if (userNum_ == 1) return ruInfos[0]->userObjNum;
		else if (userNum_ == 0) return ruInfos[1]->userObjNum;
	}

	InGameUser* GetTeamUser(uint16_t userNum_) {
		if (userNum_ == 1) return ruInfos[0]->inGameUser;
		else if (userNum_ == 0) return ruInfos[1]->inGameUser;
	}

	unsigned int GetTeamScore(uint16_t userNum) {
		if (userNum == 1) return ruInfos[0]->userScore;
		else if (userNum == 0) return ruInfos[1]->userScore;
	}

private:
	// 32 bytes
	std::vector<RaidUserInfo*> ruInfos;

	// 2 bytes
	uint16_t roomNum;

	// 1 bytes
	bool timeOver = false;
	std::atomic<uint16_t> startCheck = 0;
};