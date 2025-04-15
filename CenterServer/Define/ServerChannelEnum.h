#pragma once
#include <cstdint>
#include <string>

// ---------------------------- MYSQL  ----------------------------
// 유저의 현재 접속 중인와 서버, 채널 번호

//CenterServer = 1, 0
//ChannelServer01 = 10~, 1~
//ChannelServer02 = 20~, 2~


//  ---------------------------- SERVER INFO  ----------------------------

enum class ServerType : uint16_t { // 중앙 서버만 사용하는 번호
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
	CH_01 = 1, // 1서버
	CH_02 = 2, // 2서버
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
};
