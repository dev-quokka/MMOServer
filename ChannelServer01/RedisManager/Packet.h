#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

const uint16_t RANKING_USER_COUNT = 3; // 몇명씩 유저 랭킹 정보 가져올건지

const int MAX_USER_ID_LEN = 32;
const int MAX_SERVER_USERS = 128; // 서버 유저 수 전달 패킷
const int MAX_JWT_TOKEN_LEN = 256;
const int MAX_SCORE_SIZE = 512;

const int CHANNEL_NUM = 1;

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


//  ---------------------------- CENTER SERVER  ----------------------------

struct IM_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelServerNum;
};

struct IM_CHANNEL_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct USER_DISCONNECT_REQUEST_PACKET : PACKET_HEADER {
	uint16_t channelServerNum;
};


//  ---------------------------- CHANNEL SERVER  ----------------------------

struct USER_CONNECT_CHANNEL_REQUEST : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_CHANNEL_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct CHANNEL_USER_COUNTS_REQUEST : PACKET_HEADER {

};

struct CHANNEL_USER_COUNTS_RESPONSE : PACKET_HEADER {
	char channelUserCnt[MAX_SERVER_USERS + 1];
	uint16_t channelCount;
};

struct MOVE_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelNum;
};

struct MOVE_CHANNEL_RESPONSE : PACKET_HEADER { // 해당 채널 접속 가능한지 체크
	bool isSuccess;
};

//  ---------------------------- USER STATUS  ----------------------------

struct EXP_UP_REQUEST : PACKET_HEADER {
	short mobNum; // Number of Mob
};

struct EXP_UP_RESPONSE : PACKET_HEADER {
	unsigned int currentExp;
	uint16_t increaseLevel;
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
	uint16_t itemCount; // (Max 99)
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
	uint16_t Enhancement = 0;
	bool isSuccess;
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


enum class CHANNEL_ID : uint16_t {
	// CENTER SERVER (1~)
	IM_CHANNEL_REQUEST = 1,
	IM_CHANNEL_RESPONSE = 2,
	USER_DISCONNECT_REQUEST = 3,
	MOVE_CENTER_SERVER_REQUEST = 4,
	MOVE_CENTER_SERVER_RESPONSE = 5,

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