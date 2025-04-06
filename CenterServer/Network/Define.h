#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <cstdint>
#include <unordered_map>

const uint32_t MAX_RECV_SIZE = 1024; // Set Max RECV Buf
const uint32_t MAX_CIRCLE_SIZE = 8096;

const short MAX_RETRY_COUNT = 3;

std::string JWT_SECRET = "Cute_Quokka";

// ---------------------------- MYSQL  ----------------------------
//CenterServer = 1
//ChannelServer01 = 10~, 1~
//ChannelServer02 = 20~, 2~



//  ---------------------------- SERVER INFO  ----------------------------

enum class ServerType : uint16_t
{
	// Server Type (1~)
	GatewayServer = 1,
	MatchingServer = 2,

	// Channel Server (11~)
	ChannelServer01 = 11,
	ChannelServer02 = 12,

	// Game Server (31~)
	RaidGameServer01 = 31,
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
};

std::unordered_map<ServerType, ServerAddress> ServerAddressMap;


//  ---------------------------- SYSTEM  ----------------------------

enum class TaskType {
	ACCEPT,
	RECV,
	SEND,
	NEWRECV, // 오버랩 풀 다 써서 새로 만들어서 사용한것. (이건 다 쓰면 삭제)
	NEWSEND
};

struct OverlappedEx {
	WSAOVERLAPPED wsaOverlapped;
	// 4 bytes
	TaskType taskType; // ACCPET, RECV, SEND INFO
};

struct OverlappedTCP : OverlappedEx {
	// 2 bytes
	uint16_t connObjNum;
	// 16 bytes
	WSABUF wsaBuf; // TCP Buffer
};

struct OverlappedUDP : OverlappedEx {
	// 4 bytes
	int addrSize = sizeof(sockaddr_in);
	// 16 bytes
	WSABUF wsaBuf; // UDP Buffer
	sockaddr_in userAddr;  // Client Ip && Port Info
};

