#include "ServerEnum.h"
#include "MatchingServer.h"
#include "ServerAddress.h"

constexpr uint16_t maxThreadCount = 1;

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