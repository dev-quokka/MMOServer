#include "QuokkaServer.h"
#include "ServerAddress.h"

constexpr uint16_t maxThreadCount = 2;
constexpr uint16_t maxClientCount = 100;

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