#pragma once
#include "InGameUser.h"

class InGameUserManager {
public:
	~InGameUserManager() {
		for (int i = 0; i < inGmaeUsers.size(); i++) {
			delete inGmaeUsers[i];
		}
	}

	// ====================== INITIALIZATION ======================
	void Init(uint16_t maxClientCount_);


	// ================== INGAME USER MANAGEMENT ==================
	InGameUser* GetInGameUserByObjNum(uint16_t connObjNum_);
	void Set(uint16_t connObjNum_, std::string userId_, uint32_t userPk_, unsigned int userExp_, uint16_t userLevel_, unsigned int raidScore_);
	void Reset(uint16_t connObjNum_);

private:
	std::vector<InGameUser*> inGmaeUsers;
	std::vector<uint16_t> expLimit = { 0,1,2,4,6,10,16,23,31,40,51, 60, 71, 80, 91, 100 }; // // Set experience points required for each level
};