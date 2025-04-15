#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

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

//  ---------------------------- SYSTEM  ----------------------------

struct IM_MATCHING_REQUEST : PACKET_HEADER {

};

struct IM_MATCHING_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};


//  ---------------------------- MATCHING  ----------------------------

struct MATCHING_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userPk; 
	uint16_t userCenterObjNum;
	uint16_t userGroupNum;
};

struct MATCHING_RESPONSE_FROM_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	bool isSuccess;
};

struct RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t roomNum;
};

struct MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	uint16_t userGroupNum;
};

struct MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	bool isSuccess;
};


//  ---------------------------- RAID  ----------------------------

struct MATCHING_SERVER_CONNECT_REQUEST : PACKET_HEADER {
	uint16_t gameServerNum;
};

struct MATCHING_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MATCHING_REQUEST_TO_GAME_SERVER : PACKET_HEADER {
	uint16_t userPk1;
	uint16_t userPk2;
	uint16_t userCenterObjNum1;
	uint16_t userCenterObjNum2;
	uint16_t roomNum;
};

enum class PACKET_ID : uint16_t {
	//SYSTEM (5001~)
	IM_MATCHING_REQUEST = 5001,
	IM_MATCHING_RESPONSE = 5002,

	//RAID(5011~)
	MATCHING_REQUEST_TO_MATCHING_SERVER = 5011,
	MATCHING_RESPONSE_FROM_MATCHING_SERVER = 5012,
	RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER = 5014,

	MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER = 5021,
	MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER = 5022,


	//  ---------------------------- GAME(8001~)  ----------------------------
	MATCHING_SERVER_CONNECT_REQUEST = 8003,
	MATCHING_SERVER_CONNECT_RESPONSE = 8004,

	MATCHING_REQUEST_TO_GAME_SERVER = 8011,

};