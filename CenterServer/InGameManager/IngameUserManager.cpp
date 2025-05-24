#include "InGameUserManager.h"

// =================== INITIALIZATION ===================

void InGameUserManager::Init(uint16_t maxClientCount_) {
	inGmaeUsers.resize(maxClientCount_, nullptr);

	for (int i = 0; i < maxClientCount_; i++) {
		inGmaeUsers[i] = new InGameUser;
	}
}

void InGameUserManager::Reset(uint16_t connObjNum_) {
	inGmaeUsers[connObjNum_]->Reset();
}


// =================== USER ACCESS ===================
InGameUser* InGameUserManager::GetInGameUserByObjNum(uint16_t connObjNum_){
	return inGmaeUsers[connObjNum_];
}