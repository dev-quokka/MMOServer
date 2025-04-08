#include <iostream>
#include "ChannelServer1.h"

const uint16_t PORT = 9211;
const uint16_t maxThreadCount = 1;

int main() {
	ChannelServer1 channelServer1;
	channelServer1.init(maxThreadCount, PORT);
	channelServer1.StartWork();

    std::cout << "=== CHANNEL SERVER 1 START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write CHANNEL1 ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "CHANNEL1") break;
    }

    channelServer1.ServerEnd();
    return 0;

	return 0;
}