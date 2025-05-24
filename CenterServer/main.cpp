#include "QuokkaServer.h"

constexpr uint16_t maxThreadCount = 1;
constexpr uint16_t maxClientCount = 30; // User objects allocated for average Center Server load + additional allocation for connected servers 

std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
    { ServerType::CenterServer,     { "127.0.0.1", 9090 } },
    { ServerType::ChannelServer01, { "127.0.0.1", 9211 } },
    { ServerType::ChannelServer02, { "127.0.0.1", 9221 } },
    { ServerType::RaidGameServer01, { "127.0.0.1", 9510 } },
    { ServerType::LoginServer,   { "127.0.0.1", 9091 } },
    { ServerType::MatchingServer,   { "127.0.0.1", 9131 } },
};

int main() {
    QuokkaServer server(maxClientCount);

    server.init(maxThreadCount, ServerAddressMap[ServerType::CenterServer].port);
    server.StartWork();

    std::cout << "========= CENTER SERVER START ========" << std::endl;
    std::cout << "=== If You Want Exit, Write quokka ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "quokka") break;
    }

    server.ServerEnd();
    return 0;
}



