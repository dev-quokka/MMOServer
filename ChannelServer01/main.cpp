#include "ChannelServer1.h"

int main() {
	ChannelServer1 channelServer1;

    channelServer1.init();
	channelServer1.StartWork();

    std::cout << "========== 채널 서버1 ==========" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "channel1") break;
    }

    channelServer1.ServerEnd();

	return 0;
}