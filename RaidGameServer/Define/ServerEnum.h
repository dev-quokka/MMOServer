#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

enum class ServerType : uint16_t {
	// Center Server (0)
	CenterServer = 0,

	// Channel Server (11~)
	ChannelServer01 = 1,
	ChannelServer02 = 2,

	// Game Server (51~)
	RaidGameServer01 = 51,

	// Login Server (101~)
	LoginServer = 101,

	// Matching Server (111~)
	MatchingServer = 111,
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
};

extern std::unordered_map<ServerType, ServerAddress> ServerAddressMap;