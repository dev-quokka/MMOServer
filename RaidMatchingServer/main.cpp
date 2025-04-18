#include <iostream>

#include "ServerEnum.h"
#include "MatchingServer.h"

constexpr uint16_t maxThreadCount = 1;

std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
    { ServerType::CenterServer,     { "127.0.0.1", 9090 } },
    { ServerType::RaidGameServer01, { "127.0.0.1", 9501 } },
    { ServerType::MatchingServer,   { "127.0.0.1", 9131 } }
};

int main() {

	MatchingServer matchingServer;
    matchingServer.Init(maxThreadCount, ServerAddressMap[ServerType::MatchingServer].port);
    matchingServer.StartWork();

    std::cout << "========= MATCHING SERVER START ========" << std::endl;
    std::cout << "=== If You Want Exit, Write matching ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "matching") break;
    }

    matchingServer.ServerEnd();

	return 0;
}