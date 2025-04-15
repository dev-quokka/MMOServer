#pragma once

#include <cstdint>
#include <vector>  
#include <string>
#include <iostream>
#include "UserStateEnum.h"

class InGameUser {
public:
	uint16_t GetLevel() {
		return userLevel;
	}

	void Set(std::string userId_, uint32_t userPk_, unsigned int userExp_, uint16_t userLevel_, unsigned int raidScore_) {
		userLevel = userLevel_;
		userExp = userExp_;
		userPk = userPk_;
		userId = userId_;
		raidScore = raidScore_;
	}

	void Reset() {
		userId = "";
		userLevel = 0;
		userPk = 0;
		userExp = 0;
		raidScore = 0;
		userState.store(1);
	}

	bool CheckMatching() {
		return userState == static_cast<uint16_t>(UserState::raidMatching);
	}

	uint32_t GetPk() {
		return userPk;
	}

	std::string GetId() {
		return userId;
	}

	unsigned int GetScore() {
		return raidScore;
	}

private:
	// 40 bytes
	std::string userId;

	// 4 bytes
	uint32_t userPk;
	unsigned int userExp;
	unsigned int raidScore;

	// 2 bytes
	uint16_t userLevel;
	std::atomic<uint16_t> userState = 1; // // Initialize userState as 1 (indicating the user is offline)
};