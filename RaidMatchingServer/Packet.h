#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

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

enum class MATCHING_ID : uint16_t {
	//SYSTEM
	IM_MATCHING_REQUEST = 1, // 유저는 1번으로 요청
	IM_MATCHING_RESPONSE = 2, // 유저는 1번으로 요청
	
	//RAID(11~)
	MATCHING_REQUEST = 11,
	MATCHING_RESPONSE = 12,
	MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER = 13,
	RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER = 14

};