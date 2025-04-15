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
	CH_011 = 1, // 1-1 Channel
	CH_012 = 2, // 1-2 Channel
	CH_013 = 3, // 1-3 Channel

};
