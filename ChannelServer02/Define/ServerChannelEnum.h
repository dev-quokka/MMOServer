#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

constexpr int CHANNEL_SERVER_NUM = 2;
constexpr uint16_t CHANNEL_SERVER_START_NUMBER = 0;

//  =========================== SERVER INFO  ===========================

enum class ServerType : uint16_t {
	// Center Server (0)
	CenterServer = 0,

	// Channel Server (1~)
	ChannelServer01 = 1,
	ChannelServer02 = 2
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
};

extern std::unordered_map<ServerType, ServerAddress> ServerAddressMap;