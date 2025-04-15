#include <iostream>
#include "ChannelServer1.h"

const uint16_t PORT = 9211;
const uint16_t maxThreadCount = 1;

int main() {
	ChannelServer1 channelServer1;

    if (!channelServer1.init(maxThreadCount, PORT)) {
        return 0;
    }

	channelServer1.StartWork();
    channelServer1.CenterConnect();

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