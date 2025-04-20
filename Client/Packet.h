#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

const int MAX_INVEN_SIZE = 512;

const uint16_t RANKING_USER_COUNT = 3; // 몇명씩 유저 랭킹 정보 가져올건지

const int MAX_IP_LEN = 32;
const int MAX_SERVER_USERS = 128; // 서버 유저 수 전달 패킷
const int MAX_USER_ID_LEN = 32;
const int MAX_JWT_TOKEN_LEN = 256;
const int MAX_SCORE_SIZE = 256;

struct PACKET_HEADER
{
	uint16_t PacketLength;
	uint16_t PacketId;
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

struct RAID_USERINFO{
	std::string userId  = "";
	uint16_t userLevel = 0;
	uint16_t userScore = 0;
};

struct RANKING {
	uint16_t score = 0;
	char userId[MAX_USER_ID_LEN + 1] = {};
};

//  ---------------------------- SYSTEM  ----------------------------

struct USER_CONNECT_REQUEST_PACKET : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_RESPONSE_PACKET : PACKET_HEADER {
	bool isSuccess;
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
	char serverToken[MAX_JWT_TOKEN_LEN + 1]; // Token For Server Connection
	char ip[MAX_IP_LEN + 1];
	uint16_t port;
};

struct CHANNEL_USER_COUNTS_REQUEST : PACKET_HEADER {

};

struct CHANNEL_USER_COUNTS_RESPONSE : PACKET_HEADER {
	uint16_t channelCount;
	char channelUserCnt[MAX_SERVER_USERS + 1];
};

struct USER_CONNECT_CHANNEL_REQUEST_PACKET : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_CHANNEL_RESPONSE_PACKET : PACKET_HEADER {
	bool isSuccess;
};

struct MOVE_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelNum;
};

struct MOVE_CHANNEL_RESPONSE : PACKET_HEADER {
	bool isSuccess;
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



//  ---------------------------- USER STATUS  ----------------------------

struct EXP_UP_REQUEST : PACKET_HEADER {
	short mobNum; // Number of Mob
};

struct EXP_UP_RESPONSE : PACKET_HEADER {
	uint16_t increaseLevel;
	unsigned int currentExp;
};

struct LEVEL_UP_RESPONSE : PACKET_HEADER {
	uint16_t increaseLevel;
	unsigned int currentExp;
};

//  ---------------------------- INVENTORY  ----------------------------

struct ADD_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
	uint16_t itemCount; // (Max 99)
	uint16_t itemCode; // (Max 5000)
};

struct ADD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
};

struct DEL_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOD_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
	int8_t itemCount; // (Max 99)
	uint16_t itemCode; // (Max 5000)
};

struct MOD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOV_ITEM_REQUEST : PACKET_HEADER {
	uint16_t ItemType; // (Max 3)

	uint16_t dragItemPos; // (Max 10)
	uint16_t dragItemCode;
	uint16_t dragItemCount; // (Max 99)

	uint16_t targetItemPos; // (Max 10)
	uint16_t targetItemCode;
	uint16_t targetItemCount; // (Max 99)
};

struct MOV_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

//  ---------------------------- INVENTORY:EQUIPMENT  ----------------------------

struct ADD_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
	uint16_t Enhancement; // (Max 20)
	uint16_t itemCode; // (Max 5000)
};

struct ADD_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
};

struct DEL_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct ENH_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
};

struct ENH_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
	uint16_t Enhancement = 0;
};

struct MOV_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t dragItemPos; // (Max 10)
	uint16_t dragItemCode;
	uint16_t dragItemEnhancement;

	uint16_t targetItemPos; // (Max 10)
	uint16_t targetItemCode;
	uint16_t targetItemEnhancement;
};

struct MOV_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};


//  ---------------------------- RAID  ----------------------------

struct RAID_MATCHING_REQUEST : PACKET_HEADER { // Users Matching Request

};

struct RAID_MATCHING_RESPONSE : PACKET_HEADER {
	bool insertSuccess; // Insert Into Matching Queue Check
};

struct RAID_READY_REQUEST : PACKET_HEADER {
	char serverToken[MAX_JWT_TOKEN_LEN + 1]; // Token For Server Connection
	char ip[MAX_IP_LEN + 1];
	uint16_t port;
	uint16_t roomNum;
};

struct RAID_RANKING_REQUEST : PACKET_HEADER {
	uint16_t startRank;
};

struct RAID_RANKING_RESPONSE : PACKET_HEADER {
	uint16_t rkCount;
	char reqScore[MAX_SCORE_SIZE + 1];
};


//  ---------------------------- GAME SERVER  ----------------------------

struct USER_CONNECT_GAME_REQUEST : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_GAME_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct RAID_TEAMINFO_REQUEST : PACKET_HEADER {
	sockaddr_in userAddr;
};

struct RAID_TEAMINFO_RESPONSE : PACKET_HEADER {
	char teamId[MAX_USER_ID_LEN + 1];
	uint16_t userLevel;
	uint16_t userRaidServerObjNum;
};

struct RAID_START : PACKET_HEADER {
	std::chrono::time_point<std::chrono::steady_clock> endTime; // 설정한 종료 시간 + 5초 (모든 유저 들어오는 시간 5초 설정)
	unsigned int mobHp;
	uint16_t mapNum;
};

struct RAID_HIT_REQUEST : PACKET_HEADER {
	unsigned int damage;
};

struct RAID_HIT_RESPONSE : PACKET_HEADER {
	unsigned int yourScore;
	unsigned int currentMobHp;
};

struct RAID_END : PACKET_HEADER {

};

struct SEND_RAID_SCORE : PACKET_HEADER {
	unsigned int userScore;
	uint16_t userRaidServerObjNum;
};


enum class PACKET_ID : uint16_t {
	//  ---------------------------- CENTER (1~)  ----------------------------
	
	// SYSTEM (1~)
	USER_CONNECT_REQUEST = 1,
	USER_CONNECT_RESPONSE = 2,
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

	RAID_RANKING_REQUEST = 55,
	RAID_RANKING_RESPONSE = 56,

	//  ---------------------------- SESSION (801~)  ----------------------------
	
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

	//  ---------------------------- CHANNEL (1501~)  ----------------------------

	// CHANNEL SERVER (1511~)
	USER_CONNECT_CHANNEL_REQUEST = 1511,
	USER_CONNECT_CHANNEL_RESPONSE = 1512,
	CHANNEL_USER_COUNTS_REQUEST = 1513,
	CHANNEL_USER_COUNTS_RESPONSE = 1514,
	MOVE_CHANNEL_REQUEST = 1515,
	MOVE_CHANNEL_RESPONSE = 1516,

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

	//  ---------------------------- RAID(8001~)  ----------------------------

	USER_CONNECT_GAME_REQUEST = 8005,
	USER_CONNECT_GAME_RESPONSE = 8006,

	RAID_TEAMINFO_REQUEST = 8055,
	RAID_TEAMINFO_RESPONSE = 8056,
	RAID_START = 8057,
	RAID_HIT_REQUEST = 8058,
	RAID_HIT_RESPONSE = 8059,

	RAID_END = 8101,
};