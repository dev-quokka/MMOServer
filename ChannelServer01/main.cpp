#include "ChannelServer1.h"

const uint16_t maxThreadCount = 1;

std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
    { ServerType::CenterServer,     { "127.0.0.1", 9090 } },
    { ServerType::ChannelServer01, { "127.0.0.1", 9211 } },
    { ServerType::ChannelServer02, { "127.0.0.1", 9221 } }
};

int main() {
	ChannelServer1 channelServer1;

    channelServer1.init(maxThreadCount, ServerAddressMap[ServerType::ChannelServer01].port);
	channelServer1.StartWork();

    std::cout << "=== CHANNEL SERVER 1 START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write channel1 ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "channel1") break;
    }

    channelServer1.ServerEnd();

	return 0;
}