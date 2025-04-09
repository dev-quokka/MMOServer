#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

const int MAX_SERVER_USERS = 128; // 서버 유저 수 전달 패킷

const uint16_t RANKING_USER_COUNT = 3; // 몇명씩 유저 랭킹 정보 가져올건지

const int MAX_IP_LEN = 32;
const int MAX_USER_ID_LEN = 32;
const int MAX_JWT_TOKEN_LEN = 256;
const int MAX_SCORE_SIZE = 256;

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

struct USER_CONNECT_REQUEST_PACKET : PACKET_HEADER {
	char userId[MAX_USER_ID_LEN + 1];
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
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

const int MAX_INVEN_SIZE = 512;

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
	uint16_t timer; // Minutes
	uint16_t roomNum; // If Max RoomNum Up to Short Range, Back to Number One
	uint16_t yourNum;
	int mobHp;
};

struct RAID_TEAMINFO_REQUEST : PACKET_HEADER { // User To Server
	bool imReady;
	uint16_t roomNum;
	uint16_t myNum;
	sockaddr_in userAddr;// 유저가 만든 udp 소켓의 sockaddr_in 전달
};

struct RAID_TEAMINFO_RESPONSE : PACKET_HEADER {
	uint16_t teamLevel;
	char teamId[MAX_USER_ID_LEN + 1];
};

struct RAID_START_REQUEST : PACKET_HEADER {
	std::chrono::time_point<std::chrono::steady_clock> endTime;
};

struct RAID_HIT_REQUEST : PACKET_HEADER {
	uint16_t roomNum;
	uint16_t myNum;
	unsigned int damage;
};

struct RAID_HIT_RESPONSE : PACKET_HEADER {
	unsigned int yourScore;
	unsigned int currentMobHp;
};

struct RAID_END_REQUEST : PACKET_HEADER { // Server to USER
	unsigned int userScore;
	unsigned int teamScore;
};

struct RAID_END_RESPONSE : PACKET_HEADER { // User to Server (If Server Get This Packet, Return Room Number)

};

struct RAID_RANKING_REQUEST : PACKET_HEADER {
	uint16_t startRank;
};

struct RAID_RANKING_RESPONSE : PACKET_HEADER {
	uint16_t rkCount;
	char reqScore[MAX_SCORE_SIZE + 1];
};

enum class PACKET_ID : uint16_t {
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
	RAID_READY_REQUEST = 47,
	RAID_RANKING_REQUEST = 55,
	RAID_RANKING_RESPONSE = 56,
};

enum class SESSIONPACKET_ID : uint16_t {
	USER_LOGIN_REQUEST = 1,
	USER_LOGIN_RESPONSE = 2,
	USER_GAMESTART_REQUEST = 3,
	USER_GAMESTART_RESPONSE = 4,
	USERINFO_REQUEST = 5,
	USERINFO_RESPONSE = 6,
	EQUIPMENT_REQUEST = 7,
	EQUIPMENT_RESPONSE = 8,
	CONSUMABLES_REQUEST = 9,
	CONSUMABLES_RESPONSE = 10,
	MATERIALS_REQUEST = 11,
	MATERIALS_RESPONSE = 12,
};

enum class CHANNEL_ID : uint16_t {
	// CHANNEL SERVER (11~)
	USER_CONNECT_CHANNEL_REQUEST = 11,
	USER_CONNECT_CHANNEL_RESPONSE = 12,
	CHANNEL_USER_COUNTS_REQUEST = 13,
	CHANNEL_USER_COUNTS_RESPONSE = 14,
	MOVE_CHANNEL_REQUEST = 15,
	MOVE_CHANNEL_RESPONSE = 16,

	// USER STATUS (21~)
	EXP_UP_REQUEST = 21,
	EXP_UP_RESPONSE = 22,
	LEVEL_UP_REQUEST = 23,
	LEVEL_UP_RESPONSE = 24,

	// INVENTORY (25~)
	ADD_ITEM_REQUEST = 25,
	ADD_ITEM_RESPONSE = 26,
	DEL_ITEM_REQUEST = 27,
	DEL_ITEM_RESPONSE = 28,
	MOD_ITEM_REQUEST = 29,
	MOD_ITEM_RESPONSE = 30,
	MOV_ITEM_REQUEST = 31,
	MOV_ITEM_RESPONSE = 32,

	// INVENTORY::EQUIPMENT 
	ADD_EQUIPMENT_REQUEST = 33,
	ADD_EQUIPMENT_RESPONSE = 34,
	DEL_EQUIPMENT_REQUEST = 35,
	DEL_EQUIPMENT_RESPONSE = 36,
	ENH_EQUIPMENT_REQUEST = 37,
	ENH_EQUIPMENT_RESPONSE = 38,
	MOV_EQUIPMENT_REQUEST = 39,
	MOV_EQUIPMENT_RESPONSE = 40,
};

enum class GAME_ID : uint16_t {
	RAID_TEAMINFO_REQUEST = 1,
	RAID_HIT_REQUEST = 2,
};