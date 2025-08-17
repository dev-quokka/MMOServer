#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <winsock2.h>
#include <ws2tcpip.h>

const uint32_t MAX_RECV_SIZE = 8096; // Set Max RECV Buf

enum class TaskType {
	TCP_RECV,
	TCP_SEND,
	UDP_RECV,
	UDP_SEND
};

enum class ChannelServerType : uint16_t {
	CH_01 = 1, // 1서버
	CH_02 = 2, // 2서버
};

enum class ChannelType : uint16_t {
	CH_011 = 1, // 1서버 1 채널
	CH_012 = 2, // 1서버 2 채널
	CH_013 = 3, // 1서버 3 채널

};
