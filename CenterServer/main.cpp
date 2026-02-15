#include "QuokkaServer.h"

constexpr uint16_t maxClientCount = 100;

int main() {
    QuokkaServer server(maxClientCount);

    server.init();
    server.StartWork();

    std::cout << "========== Áß¾Ó ¼­¹ö ==========" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "quokka") break;
    }

    server.ServerEnd();
    return 0;
}