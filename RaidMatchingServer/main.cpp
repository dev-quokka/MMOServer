#include <iostream>
#include <sw/redis++/redis++.h>

#include "MatchingServer.h"

constexpr uint16_t MATCHING_SERVER_PORT = 9131;
constexpr uint16_t maxThreadCount = 1;

int main() {
	MatchingServer matchingServer;
    matchingServer.Init(maxThreadCount, MATCHING_SERVER_PORT);
    matchingServer.StartWork();

    std::cout << "=== MATCHING SERVER START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write matching ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "matching") break;
    }

    matchingServer.ServerEnd();

	return 0;
}