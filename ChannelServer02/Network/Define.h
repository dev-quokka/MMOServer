#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <cstdint>
#include <string>
#include <unordered_map>

const uint32_t MAX_RECV_SIZE = 1024; // Set Max RECV Buf
const uint32_t MAX_CIRCLE_SIZE = 8096;

enum class ChannelType : uint16_t {
	CH_021 = 1, // 2서버 1 채널
	CH_022 = 2, // 2서버 2 채널
	CH_023 = 3, // 2서버 3 채널

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
	// 16 bytes
	WSABUF wsaBuf; // TCP Buffer
	// 4 bytes
	TaskType taskType; // ACCPET, RECV, SEND INFO
	// 2 bytes
	uint16_t connObjNum;
};

