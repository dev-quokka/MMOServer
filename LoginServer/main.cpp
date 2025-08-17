#include "LoginServer.h"
#include "MySQLManager.h"

const uint16_t maxThreadCount = 1;

std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
    { ServerType::CenterServer,     { "127.0.0.1", 9090 } },
    { ServerType::LoginServer,   { "127.0.0.1", 9091 } },
};

int main() {
    LoginServer loginServer;

    loginServer.init(maxThreadCount, ServerAddressMap[ServerType::LoginServer].port);
    loginServer.StartWork();

    std::cout << "========= LOGIN SERVER START ========" << std::endl;
    std::cout << "=== If You Want Exit, Write login ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "login") break;
    }

    loginServer.ServerEnd();

    return 0;
}