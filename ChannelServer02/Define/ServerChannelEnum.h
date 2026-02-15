#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

constexpr uint16_t CHANNEL_SERVER_START_NUMBER = 0;

//  =========================== SERVER INFO  ===========================

enum class ServerType : uint16_t {
	// Center Server (0)
	CenterServer = 0,

	// Channel Server (1~)
	ChannelServer02 = 2
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
	uint16_t serverObjNum;
};

inline std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
	{ ServerType::CenterServer,     { "127.0.0.1", 9090 } },
	{ ServerType::ChannelServer02, { "127.0.0.1", 9221 } }
};