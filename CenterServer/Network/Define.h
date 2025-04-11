#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <cstdint>
#include <string>
#include <unordered_map>

const uint32_t MAX_RECV_SIZE = 1024; // Set Max Recv Buf
const uint32_t MAX_CIRCLE_SIZE = 8096;

const short MAX_RETRY_COUNT = 3;

// ---------------------------- MYSQL  ----------------------------
// 유저의 현재 접속 중인와 서버, 채널 번호

//CenterServer = 1, 0
//ChannelServer01 = 10~, 1~
//ChannelServer02 = 20~, 2~


//  ---------------------------- SERVER INFO  ----------------------------

enum class ServerType : uint16_t { // 중앙 서버만 사용하는 번호
	// Channel Server (11~)
	ChannelServer01 = 1,
	ChannelServer02 = 2,

	// Game Server (51~)
	RaidGameServer01 = 51,

	// Server Type (101~)
	GatewayServer = 101,
	MatchingServer = 102,
};

enum class ChannelServerType : uint16_t {
	CH_01 = 1, // 1서버
	CH_02 = 2, // 2서버
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
};

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

