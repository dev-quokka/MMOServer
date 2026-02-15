#include "ChannelServer2.h"

int main() {
    ChannelServer2 channelServer2;

    channelServer2.init();
    channelServer2.StartWork();

    std::cout << "========== 채널 서버2 ==========" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "channel2") break;
    }

    channelServer2.ServerEnd();

    return 0;
}