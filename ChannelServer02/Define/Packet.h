#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

constexpr uint16_t RANKING_USER_COUNT = 3; // Number of users to display per ranking page
const int MAX_USER_ID_LEN = 32;
const int MAX_SERVER_USERS = 128;
const int MAX_JWT_TOKEN_LEN = 256;
const int MAX_SCORE_SIZE = 512;

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

struct CHANNEL_SERVER_CONNECT_REQUEST : PACKET_HEADER {
	uint16_t channelServerNum;
};

struct CHANNEL_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct USER_DISCONNECT_AT_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelServerNum;
};


//  ---------------------------- CHANNEL SERVER  ----------------------------

struct USER_CONNECT_CHANNEL_REQUEST : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1];
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_CHANNEL_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct CHANNEL_USER_COUNTS_REQUEST : PACKET_HEADER {

};

struct CHANNEL_USER_COUNTS_RESPONSE : PACKET_HEADER {
	uint16_t channelCount;
	char channelUserCnt[MAX_SERVER_USERS + 1];
};

struct MOVE_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelNum;
};

struct MOVE_CHANNEL_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct RAID_RANKING_REQUEST : PACKET_HEADER {
	uint16_t startRank;
};

struct RAID_RANKING_RESPONSE : PACKET_HEADER {
	uint16_t rkCount;
	char reqScore[MAX_SCORE_SIZE + 1];
};

//  ---------------------------- USER STATUS  ----------------------------

struct EXP_UP_REQUEST : PACKET_HEADER {
	short mobNum;
};

struct EXP_UP_RESPONSE : PACKET_HEADER {
	unsigned int currentExp;
	uint16_t increaseLevel;
};

//  ---------------------------- INVENTORY  ----------------------------

struct ADD_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType;
	uint16_t itemPosition;
	uint16_t itemCount;
	uint16_t itemCode;
};

struct ADD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType;
	uint16_t itemPosition;
};

struct DEL_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOD_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType;
	uint16_t itemPosition;
	uint16_t itemCount;
	uint16_t itemCode;
};

struct MOD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOV_ITEM_REQUEST : PACKET_HEADER {
	uint16_t ItemType;

	uint16_t dragItemPos;
	uint16_t dragItemCode;
	uint16_t dragItemCount;

	uint16_t targetItemPos;
	uint16_t targetItemCode;
	uint16_t targetItemCount;
};

struct MOV_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

//  ---------------------------- INVENTORY:EQUIPMENT  ----------------------------

struct ADD_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition;
	uint16_t Enhancement;
	uint16_t itemCode;
};

struct ADD_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition;
};

struct DEL_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct ENH_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition;
};

struct ENH_EQUIPMENT_RESPONSE : PACKET_HEADER {
	uint16_t Enhancement = 0;
	bool isSuccess;
};

struct SYNC_EQUIPMENT_ENHANCE_REQUEST : PACKET_HEADER {
	uint16_t itemPosition;
	uint16_t enhancement;
	uint16_t userPk;
};

struct MOV_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t dragItemPos;
	uint16_t dragItemCode;
	uint16_t dragItemEnhancement;

	uint16_t targetItemPos;
	uint16_t targetItemCode;
	uint16_t targetItemEnhancement;
};

struct MOV_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};


enum class PACKET_ID : uint16_t {
	//  ---------------------------- CHANNEL  ----------------------------
	// SYSTEM (1501~)
	CHANNEL_SERVER_CONNECT_REQUEST = 1501,
	CHANNEL_SERVER_CONNECT_RESPONSE = 1502,
	USER_DISCONNECT_AT_CHANNEL_REQUEST = 1503,
	MOVE_CENTER_SERVER_REQUEST = 1504,
	MOVE_CENTER_SERVER_RESPONSE = 1505,

	// CHANNEL SERVER (1511~)
	USER_CONNECT_CHANNEL_REQUEST = 1511,
	USER_CONNECT_CHANNEL_RESPONSE = 1512,
	CHANNEL_USER_COUNTS_REQUEST = 1513,
	CHANNEL_USER_COUNTS_RESPONSE = 1514,
	MOVE_CHANNEL_REQUEST = 1515,
	MOVE_CHANNEL_RESPONSE = 1516,
	RAID_RANKING_REQUEST = 1517,
	RAID_RANKING_RESPONSE = 1518,

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
	SYNC_EQUIPMENT_ENHANCE_REQUEST = 1541,
};