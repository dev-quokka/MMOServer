#pragma once
#include <cstdint>
#include <string>

enum class ServerType : uint16_t {
	// Channel Server (11~)
	ChannelServer01 = 1,
	ChannelServer02 = 2,

	// Game Server (51~)
	RaidGameServer01 = 51,

	// Server Type (101~)
	GatewayServer = 101,
	MatchingServer = 102,
};

enum class ChannelType : uint16_t {
	CH_011 = 1, // 1서버 1 채널
	CH_012 = 2, // 1서버 2 채널
	CH_013 = 3, // 1서버 3 채널

};
