#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <chrono>

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

//  ---------------------------- RAID SERVER ----------------------------

struct RAID_SERVER_CONNECT_REQUEST : PACKET_HEADER {
	uint16_t gameServerNum;
};

struct RAID_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MATCHING_SERVER_CONNECT_REQUEST_FROM_RAID_SERVER : PACKET_HEADER {
	uint16_t gameServerNum;
};

struct MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER : PACKET_HEADER {
	bool isSuccess;
};

struct MATCHING_REQUEST_TO_GAME_SERVER : PACKET_HEADER {
	uint16_t userPk;
	uint16_t userCenterObjNum;
	uint16_t roomNum;
};

struct MATCHING_RESPONSE_FROM_GAME_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	uint16_t userRaidServerObjNum;
	uint16_t serverNum;
	uint16_t roomNum;
};

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
	uint16_t teamLevel;
	uint16_t userRaidServerObjNum;
};

struct RAID_START : PACKET_HEADER {
	std::chrono::time_point<std::chrono::steady_clock> endTime;
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

struct SYNC_HIGHSCORE_REQUEST : PACKET_HEADER {
	char userId[MAX_USER_ID_LEN + 1];
	unsigned int userScore;
	uint16_t userPk;
};

struct RAID_END_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t gameServerNum;
	uint16_t roomNum;
};

enum class PACKET_ID : uint16_t {

	//  ---------------------------- RAID(8001~)  ----------------------------
	RAID_SERVER_CONNECT_REQUEST = 8001,
	RAID_SERVER_CONNECT_RESPONSE = 8002,
	MATCHING_SERVER_CONNECT_REQUEST_FROM_RAID_SERVER = 8003,
	MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER = 8004,

	USER_CONNECT_GAME_REQUEST = 8005,
	USER_CONNECT_GAME_RESPONSE = 8006,

	MATCHING_REQUEST_TO_GAME_SERVER = 8011,
	MATCHING_RESPONSE_FROM_GAME_SERVER = 8012,

	ROOM_INSERT_REQUEST = 8051,
	ROOM_INSERT_RESPONSE = 8052,

	RAID_TEAMINFO_REQUEST = 8055,
	RAID_TEAMINFO_RESPONSE = 8056,
	RAID_START = 8057,
	RAID_HIT_REQUEST = 8058,
	RAID_HIT_RESPONSE = 8059,

	SYNC_HIGHSCORE_REQUEST = 8091,

	RAID_END = 8101,
	SEND_RAID_SCORE = 8102,

	RAID_END_REQUEST_TO_MATCHING_SERVER = 8111,
};