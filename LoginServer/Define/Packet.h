#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <string>
#include <chrono>

#include "UserSyncData.h"

constexpr uint16_t MAX_JWT_TOKEN_LEN = 256;
constexpr uint16_t MAX_SCORE_SIZE = 512;

struct DataPacket {
	uint32_t dataSize;
	uint16_t connObjNum;
	DataPacket(uint32_t dataSize_, uint16_t connObjNum_) : dataSize(dataSize_), connObjNum(connObjNum_) {}
	DataPacket() = default;
};

struct PacketInfo
{
	uint16_t packetId = 0;
	uint16_t dataSize = 0;
	uint16_t connObjNum = 0;
	char* pData = nullptr;
};

struct PACKET_HEADER
{
	uint16_t PacketLength;
	uint16_t PacketId;
};

// ======================= LOGIN SERVER =======================

struct LOGIN_SERVER_CONNECT_REQUEST : PACKET_HEADER {
	char Token[MAX_JWT_TOKEN_LEN + 1];
};

struct LOGIN_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
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
	char Equipments[MAX_INVEN_SIZE+1];
};

struct CONSUMABLES_REQUEST : PACKET_HEADER {

};

struct CONSUMABLES_RESPONSE : PACKET_HEADER {
	uint16_t csCount;
	char Consumables[MAX_INVEN_SIZE+1];
};

struct MATERIALS_REQUEST : PACKET_HEADER {

};

struct MATERIALS_RESPONSE : PACKET_HEADER {
	uint16_t mtCount;
	char Materials[MAX_INVEN_SIZE+1];
};


enum class PACKET_ID : uint16_t {

	// ======================= LOGIN SERVER (801~ ) =======================
	
	// SYSTEM (801~)
	LOGIN_SERVER_CONNECT_REQUEST = 801,
	LOGIN_SERVER_CONNECT_RESPONSE = 802,

	// USER LOGIN (811~)
	USER_LOGIN_REQUEST = 811,
	USER_LOGIN_RESPONSE = 812,
	USER_GAMESTART_REQUEST = 813,
	USER_GAMESTART_RESPONSE = 814,
	USERINFO_REQUEST = 815,
	USERINFO_RESPONSE = 816,
	EQUIPMENT_REQUEST = 817,
	EQUIPMENT_RESPONSE = 818,
	CONSUMABLES_REQUEST = 819,
	CONSUMABLES_RESPONSE = 820,
	MATERIALS_REQUEST = 821,
	MATERIALS_RESPONSE = 822,

	// SYNCRONIZATION (851~)
	SYNCRONIZE_LEVEL_REQUEST = 851,
	SYNCRONIZE_LOGOUT_REQUEST = 852,
	SYNCRONIZE_DISCONNECT_REQUEST = 853,
};
