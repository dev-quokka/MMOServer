#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

const int MAX_USER_ID_LEN = 32;
const int MAX_JWT_TOKEN_LEN = 256;
const int MAX_SCORE_SIZE = 256;
const int MAX_INVEN_SIZE = 512;

struct PACKET_HEADER
{
	uint16_t PacketLength;
	uint16_t PacketId;
};

struct USERINFO {
	uint16_t level = 0;
	unsigned int exp = 0;
	unsigned int raidScore = 0;
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

//  ---------------------------- SYSTEM  ----------------------------


struct IM_SESSION_REQUEST : PACKET_HEADER {
	char Token[MAX_JWT_TOKEN_LEN + 1];
};

struct IM_SESSION_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct SYNCRONIZE_LEVEL_REQUEST : PACKET_HEADER {
	uint16_t level;
	uint16_t userPk;
	unsigned int currentExp;
};

struct SYNCRONIZE_LOGOUT_REQUEST : PACKET_HEADER {
	uint16_t userPk;
};


//  ---------------------------- SESSION  ----------------------------

struct USER_GAMESTART_REQUEST : PACKET_HEADER {
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_GAMESTART_RESPONSE : PACKET_HEADER {
	char Token[MAX_JWT_TOKEN_LEN + 1];
};

struct USERINFO_REQUEST : PACKET_HEADER {
	char userId[MAX_USER_ID_LEN + 1];
};

struct USERINFO_RESPONSE : PACKET_HEADER {
	USERINFO UserInfo;
};

struct EQUIPMENT_REQUEST : PACKET_HEADER {

};

struct EQUIPMENT_RESPONSE : PACKET_HEADER {
	uint16_t eqCount;
	char Equipments[MAX_INVEN_SIZE + 1];
};

struct CONSUMABLES_REQUEST : PACKET_HEADER {

};

struct CONSUMABLES_RESPONSE : PACKET_HEADER {
	uint16_t csCount;
	char Consumables[MAX_INVEN_SIZE + 1];
};

struct MATERIALS_REQUEST : PACKET_HEADER {

};

struct MATERIALS_RESPONSE : PACKET_HEADER {
	uint16_t mtCount;
	char Materials[MAX_INVEN_SIZE + 1];
};

enum class SESSION_ID : uint16_t {
	// SYSTEM (1~)
	IM_SESSION_REQUEST = 1,
	IM_SESSION_RESPONSE = 2,

	// USER LOGIN (11~)
	USER_LOGIN_REQUEST = 11,
	USER_LOGIN_RESPONSE = 12,
	USER_GAMESTART_REQUEST = 13,
	USER_GAMESTART_RESPONSE = 14,
	USERINFO_REQUEST = 15,
	USERINFO_RESPONSE = 16,
	EQUIPMENT_REQUEST = 17,
	EQUIPMENT_RESPONSE = 18,
	CONSUMABLES_REQUEST = 19,
	CONSUMABLES_RESPONSE = 20,
	MATERIALS_REQUEST = 21,
	MATERIALS_RESPONSE = 22,

	// SYNCRONIZATION (51~)
	SYNCRONIZE_LEVEL_REQUEST = 51,
	SYNCRONIZE_LOGOUT_REQUEST = 52,
	SYNCRONIZE_DISCONNECT_REQUEST = 53,

};
