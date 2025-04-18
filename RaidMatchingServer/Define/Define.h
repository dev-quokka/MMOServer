#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <cstdint>

const uint32_t MAX_RECV_SIZE = 1024; // Set Max Recv Buf
const uint32_t MAX_CIRCLE_SIZE = 1024;

// ======================= IOCP EXTENDED OVERLAPPED STRUCT =======================

enum class TaskType {
	ACCEPT,
	RECV,
	SEND,
	NEWRECV,
	NEWSEND
};

struct OverlappedEx {
	WSAOVERLAPPED wsaOverlapped;

	// 16 bytes
	WSABUF wsaBuf;

	// 4 bytes
	TaskType taskType;

	// 2 bytes
	uint16_t connObjNum;
};