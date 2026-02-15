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
	MatchingServer = 5
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
	uint16_t serverObjNum;
};

inline std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
	{ ServerType::CenterServer,     { "127.0.0.1", 9090 } },
	{ ServerType::MatchingServer,   { "127.0.0.1", 9131 } },
	{ ServerType::RaidGameServer01, { "127.0.0.1", 9510 } }
};