#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <winsock2.h>
#include <cstdint>

const UINT32 MAX_SOCK = 1024; // Set Max Socket Buf
const UINT32 MAX_RECV_DATA = 8096;

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
