#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <ws2tcpip.h>
#include <utility>
#include <iostream>

#include "InGameUser.h"

class InGameUserManager {
public:
	~InGameUserManager() {
		for (int i = 0; i < inGmaeUsers.size(); i++) {
			delete inGmaeUsers[i];
		}
	}

	// =================== INITIALIZATION ===================
	void Init(uint16_t maxClientCount_);
	void Reset(uint16_t connObjNum_);


	// =================== USER ACCESS ===================
	InGameUser* GetInGameUserByObjNum(uint16_t connObjNum_);

private:
	std::vector<InGameUser*> inGmaeUsers;
};