#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

enum class ServerType : uint16_t {
	// Center Server (0)
	CenterServer = 0,

	// Game Server (3~)
	RaidGameServer01 = 3,

	// Matching Server (5)
	MatchingServer = 5,
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
};

extern std::unordered_map<ServerType, ServerAddress> ServerAddressMap;