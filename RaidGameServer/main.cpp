#include "GameServer1.h"
#include "ServerAddress.h"

// *** Start the matching server first, then the game server ***

const uint16_t maxThreadCount = 1;

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