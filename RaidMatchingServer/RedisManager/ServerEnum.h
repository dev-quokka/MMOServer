#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

constexpr uint16_t GAME_SERVER_START_NUMBER = 50;
constexpr uint16_t SERVER_COUNT = 3;

//  =========================== SERVER INFO  ===========================

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

inline std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
	{ ServerType::CenterServer,     { "127.0.0.1", 9090 } },
	{ ServerType::RaidGameServer01, { "127.0.0.1", 9501 } },
	{ ServerType::MatchingServer,   { "127.0.0.1", 9131 } }
};