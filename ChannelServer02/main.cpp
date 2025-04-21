#include "ChannelServer2.h"

const uint16_t maxThreadCount = 1;

std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
    { ServerType::CenterServer,     { "127.0.0.1", 9090 } },
    { ServerType::ChannelServer01, { "127.0.0.1", 9211 } },
    { ServerType::ChannelServer02, { "127.0.0.1", 9221 } }
};

int main() {
    ChannelServer2 channelServer2;

    channelServer2.init(maxThreadCount, ServerAddressMap[ServerType::ChannelServer02].port);
    channelServer2.StartWork();

    std::cout << "=== CHANNEL SERVER 2 START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write channel2 ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "channel2") break;
    }

    channelServer2.ServerEnd();

    return 0;
}