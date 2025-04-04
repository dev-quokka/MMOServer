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

struct MATCHING_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userPk;
	uint16_t userGroupNum;
};

// 매칭 insert 성공하면 메시지 따로 전송 X

struct MATCHING_FAIL_RESPONSE : PACKET_HEADER { // MATCHING SERVER TO CENTER SERVER
	uint16_t userPk;
};

struct MATCHING_SUCCESS_RESPONSE : PACKET_HEADER {
	uint16_t roomNum;
	uint16_t userNum1;
	uint16_t userNum2;
};

enum class MATCHING_ID : uint16_t {
	//SYSTEM
	IM_MATCHING_REQUEST = 1, // 유저는 1번으로 요청
	IM_MATCHING_RESPONSE = 2, // 유저는 1번으로 요청

	//RAID(11~)
	MATCHING_REQUEST_TO_MATCHING_SERVER = 11,
	MATCHING_FAIL_RESPONSE = 12,
	MATCHING_SUCCESS_RESPONSE = 13,
};