#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

constexpr uint16_t CHANNEL_SERVER_START_NUMBER = 0;
constexpr uint16_t GAME_SERVER_START_NUMBER = 2;

//  =========================== SERVER INFO  ===========================

enum class ServerType : uint16_t {
	// Center Server (0)
	CenterServer = 0,

	// Channel Server (1~)
	ChannelServer01 = 1,
	ChannelServer02 = 2,

	// Game Server (3~)
	RaidGameServer01 = 3,

	// Login Server (4)
	LoginServer = 4,

	// Matching Server (5)
	MatchingServer = 5,

	// Cash Server (11)
	CashServer = 11,
};

enum class ChannelServerType : uint16_t {
	CH_01 = 1, // Channe Server1
	CH_02 = 2, // Channe Server2
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
	uint16_t serverObjNum;
};

inline std::unordered_map<ServerType, ServerAddress> ServerAddressMap = {
	{ ServerType::CashServer,     { "127.0.0.1", 5050 } },
	{ ServerType::CenterServer,     { "127.0.0.1", 9090 } },
	{ ServerType::ChannelServer01, { "127.0.0.1", 9211 } },
	{ ServerType::ChannelServer02, { "127.0.0.1", 9221 } },
	{ ServerType::RaidGameServer01, { "127.0.0.1", 9510 } },
	{ ServerType::LoginServer,   { "127.0.0.1", 9091 } },
	{ ServerType::MatchingServer,   { "127.0.0.1", 9131 } },
};