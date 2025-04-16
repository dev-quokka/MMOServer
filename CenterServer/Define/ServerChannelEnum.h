#pragma once
#include <cstdint>
#include <string>

//  ---------------------------- SERVER INFO  ----------------------------

enum class ServerType : uint16_t {
	// Center Server (0)
	CenterServer = 0,

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
	CH_01 = 1, // Channe Server1
	CH_02 = 2, // Channe Server2
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
};
