#include "InGameUserManager.h"

// ========================== INITIALIZATION ==========================

void InGameUserManager::Init(uint16_t maxClientCount_) {
	inGmaeUsers.resize(maxClientCount_, nullptr);

	for (int i = 0; i < maxClientCount_; i++) {
		inGmaeUsers[i] = new InGameUser(expLimit);
	}
}


// ======================= INGAME USER MANAGEMENT ======================

void InGameUserManager::Set(uint16_t connObjNum_, std::string userId_, uint32_t userPk_, unsigned int userExp_, uint16_t userLevel_, unsigned int raidScore_) {
	inGmaeUsers[connObjNum_]->Set(userId_, userPk_, userExp_, userLevel_, raidScore_);
}

InGameUser* InGameUserManager::GetInGameUserByObjNum(uint16_t connObjNum_) {
	return inGmaeUsers[connObjNum_];
}

void InGameUserManager::Reset(uint16_t connObjNum_) {
	inGmaeUsers[connObjNum_]->Reset();

}