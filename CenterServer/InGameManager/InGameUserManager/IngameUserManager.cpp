#include "InGameUserManager.h"

void InGameUserManager::Init(uint16_t maxClientCount_) {
	inGmaeUsers.resize(maxClientCount_, nullptr);

	for (int i = 0; i < maxClientCount_; i++) {
		inGmaeUsers[i] = new InGameUser;
	}
}

InGameUser* InGameUserManager::GetInGameUserByObjNum(uint16_t connObjNum_) {
	return inGmaeUsers[connObjNum_];
}

void InGameUserManager::Reset(uint16_t connObjNum_) {
	inGmaeUsers[connObjNum_]->Reset();

}