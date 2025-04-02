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

struct RAID_MATCHING_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userPk;
};

struct RAID_MATCHING_RESPONSE_TO_CENTER_SERVER : PACKET_HEADER {
	uint16_t roomNum;
	uint16_t userNum1;
	uint16_t userNum2;
};