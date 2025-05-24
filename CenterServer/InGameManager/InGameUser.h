#pragma once

#include <cstdint>
#include <vector>  
#include <string>
#include <iostream>

#include "UserStateEnum.h"

class InGameUser {
public:
	// ======================= INITIALIZATION =======================

	void Set(uint32_t userPk_, uint16_t userLevel_, unsigned int userExp_,unsigned int raidScore_, std::string userId_) {
		userPk = userPk_;
		userLevel = userLevel_;
		userExp = userExp_;
		raidScore = raidScore_;
		userId = userId_;
	}

	void Reset() {
		userPk = 0;
		userLevel = 0;
		userExp = 0;
		raidScore = 0;
		userId = "";
		userState.store(static_cast<uint16_t>(UserState::online));
	}


	// ======================= IDENTIFICATION =======================

	void SetUserState(UserState userState_) {
		userState.store(static_cast<uint16_t>(userState_));
	}

	uint16_t GetLevel() const {
		return userLevel;
	}

	uint32_t GetPk() const {
		return userPk;
	}

	uint32_t GetUserGroupNum() const {
		return userLevel/3 + 1;
	}

	unsigned int GetScore() {
		return raidScore;
	}

	std::string GetId() {
		return userId;
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