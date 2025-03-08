#pragma once

#include <cstdint>
#include <vector>  
#include <string>
#include <iostream>

class InGameUser {
public:
	InGameUser(std::vector<uint16_t> &expLimit_) : expLimit(expLimit_) {}

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
		userLevel = 0;
		userPk = 0;
		userExp = 0;
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

	std::pair<uint16_t, unsigned int> ExpUp(short mobExp_) {
		userExp += mobExp_;

		uint16_t levelUpCnt = 0;

		if (expLimit[userLevel] <= userExp) { // LEVEL UP
			while (userExp >= expLimit[userLevel]) {
				userLevel++;
				levelUpCnt++;
			}
		}

		return { levelUpCnt , userExp }; // Increase Level, Current Exp
	}

private:
	// 1 bytes
	uint16_t userLevel;

	// 4 bytes
	uint32_t userPk;
	unsigned int userExp;
	unsigned int raidScore;

	// 40 bytes
	std::string userId;

	std::vector<uint16_t>& expLimit;
};