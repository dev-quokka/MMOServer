#include "GameServer1.h"

// *** Start the matching server first, then the game server ***

const uint16_t maxThreadCount = 1;

std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
    { ServerType::CenterServer,     { "127.0.0.1", 9090 } },
    { ServerType::RaidGameServer01, { "127.0.0.1", 9510 } },
    { ServerType::MatchingServer,   { "127.0.0.1", 9131 } }
};

int main() {
	GameServer1 gameServer1;

    if (!gameServer1.init(maxThreadCount, ServerAddressMap[ServerType::RaidGameServer01].port)) {
        return 0;
    }

    gameServer1.StartWork();

    std::cout << "======== GAME SERVER 1 START ========" << std::endl;
    std::cout << "=== If You Want Exit, Write game1 ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "game1") break;
    }

    gameServer1.ServerEnd();

	return 0;
}