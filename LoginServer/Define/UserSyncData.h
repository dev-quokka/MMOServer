#pragma once
#include <cstdint>

constexpr uint16_t INVENTORY_SIZE = 11; // Inventory start from 1 (index 0 is not used)
constexpr uint16_t MAX_USER_ID_LEN = 32;
constexpr uint16_t MAX_INVEN_SIZE = 512;

struct LOGIN_USERINFO {
	std::string lastLogin = "";
	unsigned int raidScore = 0;
	unsigned int exp = 0;
	uint16_t level = 0;
};

struct USERINFO {
	unsigned int raidScore = 0;
	unsigned int exp = 0;
	uint16_t level = 0;
};

struct EQUIPMENT {
	uint16_t itemCode = 0;
	uint16_t position = 0;
	uint16_t enhance = 0;
};

struct CONSUMABLES {
	uint16_t itemCode = 0;
	uint16_t position = 0;
	uint16_t count = 0;
};

struct MATERIALS {
	uint16_t itemCode = 0;
	uint16_t position = 0;
	uint16_t count = 0;
};

struct RANKING {
	uint16_t score = 0;
	char userId[MAX_USER_ID_LEN + 1] = {};
};