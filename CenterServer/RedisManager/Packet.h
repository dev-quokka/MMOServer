#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

constexpr uint16_t RANKING_USER_COUNT = 3; // Number of users to display per ranking page

constexpr int MAX_IP_LEN = 32;
constexpr int MAX_USER_ID_LEN = 32;
constexpr int MAX_SERVER_USERS = 128;
constexpr int MAX_JWT_TOKEN_LEN = 256;
constexpr int MAX_SCORE_SIZE = 512;

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

struct RANKING {
	uint16_t score = 0;
	char userId[MAX_USER_ID_LEN + 1] = {};
};

//  ---------------------------- SYSTEM  ----------------------------

struct USER_CONNECT_REQUEST_PACKET : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1];
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_RESPONSE_PACKET : PACKET_HEADER {
	bool isSuccess;
};

struct USER_LOGOUT_REQUEST_PACKET : PACKET_HEADER {

};

struct IM_SESSION_REQUEST : PACKET_HEADER {
	char Token[MAX_JWT_TOKEN_LEN + 1];
};

struct IM_SESSION_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct IM_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelServerNum;
};

struct IM_CHANNEL_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct IM_MATCHING_REQUEST : PACKET_HEADER {

};

struct IM_MATCHING_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct IM_GAME_REQUEST : PACKET_HEADER {
	uint16_t gameServerNum;
};

struct IM_GAME_RESPONSE : PACKET_HEADER {
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

struct SERVER_USER_COUNTS_REQUEST : PACKET_HEADER {

};

struct SERVER_USER_COUNTS_RESPONSE : PACKET_HEADER {
	uint16_t serverCount;
	char serverUserCnt[MAX_SERVER_USERS + 1];
};

struct MOVE_SERVER_REQUEST : PACKET_HEADER {
	uint16_t serverNum;
};

struct MOVE_SERVER_RESPONSE : PACKET_HEADER {
	char serverToken[MAX_JWT_TOKEN_LEN + 1];
	char ip[MAX_IP_LEN + 1];
	uint16_t port;
};

struct RAID_READY_REQUEST : PACKET_HEADER {
	char serverToken[MAX_JWT_TOKEN_LEN + 1];
	char ip[MAX_IP_LEN + 1];
	uint16_t port;
	uint16_t roomNum;
};

struct RAID_END_REQUEST_TO_GAME_SERVER : PACKET_HEADER {
	uint16_t gameServerNum;
	uint16_t roomNum;
};


//  ---------------------------- RAID  ----------------------------

struct RAID_MATCHING_REQUEST : PACKET_HEADER {

};

struct RAID_MATCHING_RESPONSE : PACKET_HEADER {
	bool insertSuccess;
};

struct RAID_RANKING_REQUEST : PACKET_HEADER {
	uint16_t startRank;
};

struct RAID_RANKING_RESPONSE : PACKET_HEADER {
	uint16_t rkCount;
	char reqScore[MAX_SCORE_SIZE + 1];
};


//  ---------------------------- CHANNEL SERVER  ----------------------------

struct USER_DISCONNECT_AT_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelServerNum;
};


//  ---------------------------- MATCHING SERVER  ----------------------------

struct MATCHING_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userPk;
	uint16_t userCenterObjNum;
	uint16_t userGroupNum;
};

struct MATCHING_RESPONSE_FROM_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	bool isSuccess;
};

struct MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER : PACKET_HEADER {
	uint16_t roomNum;
	uint16_t userCenterObjNum1;
	uint16_t userCenterObjNum2;
};

struct RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t roomNum;
};

struct MATCHING_CANCEL_REQUEST : PACKET_HEADER {

};

struct MATCHING_CANCEL_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	uint16_t userGroupNum;
};

struct MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	bool isSuccess;
};


//  ---------------------------- RAID SERVER  ----------------------------

struct MATCHING_RESPONSE_FROM_GAME_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum1;
	uint16_t userCenterObjNum2;
	uint16_t userRaidServerObjNum1;
	uint16_t userRaidServerObjNum2;
	uint16_t roomNum;
};

struct RAID_END_REQUEST_TO_CENTER_SERVER : PACKET_HEADER {
	uint16_t gameServerNum;
	uint16_t roomNum;
};


enum class PACKET_ID : uint16_t {

	//  ---------------------------- CENTER (1~)  ----------------------------
	// SYSTEM (1~)
	USER_CONNECT_REQUEST = 1,
	USER_CONNECT_RESPONSE = 2,
	USER_LOGOUT_REQUEST = 3, 
	USER_FULL_REQUEST = 6, 
	WAITTING_NUMBER_REQUSET = 7,
	SERVER_USER_COUNTS_REQUEST = 8,
	SERVER_USER_COUNTS_RESPONSE = 9,
	MOVE_SERVER_REQUEST = 10,
	MOVE_SERVER_RESPONSE = 11,

	// RAID (45~)
	RAID_MATCHING_REQUEST = 45,
	RAID_MATCHING_RESPONSE = 46,
	MATCHING_CANCEL_REQUEST = 47,
	MATCHING_CANCEL_RESPONSE = 48,
	RAID_READY_REQUEST = 49,

	RAID_END_REQUEST_TO_GAME_SERVER = 52,

	RAID_RANKING_REQUEST = 55, 
	RAID_RANKING_RESPONSE = 56,


	//  ---------------------------- SESSION (801~)  ----------------------------

	// SYSTEM (801~)
	IM_SESSION_REQUEST = 801,
	IM_SESSION_RESPONSE = 802,

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

	//  ---------------------------- CHANNEL (1501~)  ----------------------------
	
	// SYSTEM (1501~)
	IM_CHANNEL_REQUEST = 1501,
	IM_CHANNEL_RESPONSE = 1502,
	USER_DISCONNECT_AT_CHANNEL_REQUEST = 1503,
	MOVE_CENTER_SERVER_REQUEST = 1504,
	MOVE_CENTER_SERVER_RESPONSE = 1505,

	// USER STATUS (1521~)
	EXP_UP_REQUEST = 1521,
	EXP_UP_RESPONSE = 1522,
	LEVEL_UP_REQUEST = 1523,
	LEVEL_UP_RESPONSE = 1524,

	// INVENTORY (1525~)
	ADD_ITEM_REQUEST = 1525,
	ADD_ITEM_RESPONSE = 1526,
	DEL_ITEM_REQUEST = 1527,
	DEL_ITEM_RESPONSE = 1528,
	MOD_ITEM_REQUEST = 1529,
	MOD_ITEM_RESPONSE = 1530,
	MOV_ITEM_REQUEST = 1531,
	MOV_ITEM_RESPONSE = 1532,

	// INVENTORY::EQUIPMENT 
	ADD_EQUIPMENT_REQUEST = 1533,
	ADD_EQUIPMENT_RESPONSE = 1534,
	DEL_EQUIPMENT_REQUEST = 1535,
	DEL_EQUIPMENT_RESPONSE = 1536,
	ENH_EQUIPMENT_REQUEST = 1537,
	ENH_EQUIPMENT_RESPONSE = 1538,
	MOV_EQUIPMENT_REQUEST = 1539,
	MOV_EQUIPMENT_RESPONSE = 1540,


	//  ---------------------------- MATCHING (5001~)  ----------------------------
	
	//SYSTEM (5001~)
	IM_MATCHING_REQUEST = 5001,
	IM_MATCHING_RESPONSE = 5002,

	//RAID(5011~)
	MATCHING_REQUEST_TO_MATCHING_SERVER = 5011,
	MATCHING_RESPONSE_FROM_MATCHING_SERVER = 5012,
	MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER = 5013,
	RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER = 5014,

	MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER = 5021,
	MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER = 5022,


	//  ---------------------------- RAID(8001~)  ----------------------------
	
	IM_GAME_REQUEST = 8001,
	IM_GAME_RESPONSE = 8002,

	MATCHING_RESPONSE_FROM_GAME_SERVER = 8012,

	RAID_END_REQUEST_TO_CENTER_SERVER = 8102,

};