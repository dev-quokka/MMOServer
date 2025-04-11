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


//  ---------------------------- RAID  ----------------------------

struct MATCHING_REQUEST : PACKET_HEADER {
	uint16_t userObjNum;
	uint16_t userGroupNum;
};

struct MATCHING_RESPONSE : PACKET_HEADER {
	uint16_t userObjNum;
	bool isSuccess;
};

struct MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER : PACKET_HEADER {
	uint16_t roomNum;
	uint16_t userObjNum1;
	uint16_t userObjNum2;
};

struct RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER { // 서버에서 매칭 서버로 전달
	uint16_t roomNum;
};

enum class PACKET_ID : uint16_t {
	//SYSTEM (5001~)
	IM_MATCHING_REQUEST = 5001,
	IM_MATCHING_RESPONSE = 5002,

	//RAID(5011~)
	MATCHING_REQUEST_TO_MATCHING_SERVER = 5011,
	MATCHING_RESPONSE_FROM_MATCHING_SERVER = 5012,
	MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER = 5013,
	RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER = 5014
};