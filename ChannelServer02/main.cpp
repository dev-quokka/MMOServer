#include <iostream>
#include "ChannelServer2.h"

const uint16_t PORT = 9221;
const uint16_t maxThreadCount = 1;

int main() {
    ChannelServer2 channelServer2;

    if (!channelServer2.init(maxThreadCount, PORT)) {
        return 0;
    }

    channelServer2.StartWork();
    channelServer2.CenterConnect();

    std::cout << "=== CHANNEL SERVER 2 START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write CHANNEL2 ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "CHANNEL2") break;
    }

    channelServer2.ServerEnd();

    return 0;
}