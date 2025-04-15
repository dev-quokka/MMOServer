#pragma once
#include "InGameUser.h"

#include <vector>
#include <string>
#include <cstdint>
#include <ws2tcpip.h>
#include <utility>
#include <iostream>

class InGameUserManager {
public:
	~InGameUserManager() {
		for (int i = 0; i < inGmaeUsers.size(); i++) {
			delete inGmaeUsers[i];
		}
	}

	void Init(uint16_t maxClientCount_);
	InGameUser* GetInGameUserByObjNum(uint16_t connObjNum_);
	void Reset(uint16_t connObjNum_);

private:
	std::vector<InGameUser*> inGmaeUsers;
};