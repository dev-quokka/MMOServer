#include "QuokkaServer.h"
#include <iostream>
#include <cstdint>

constexpr uint16_t PORT = 9090;
constexpr uint16_t maxThreadCount = 1;
constexpr uint16_t maxClientCount = 30; // User objects allocated for average Center Server load + additional allocation for connected servers 

int main() {
    QuokkaServer server(maxClientCount);

    server.init(maxThreadCount, PORT);

    server.StartWork();

    std::cout << "=== CENTER SERVER START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write quokka ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "quokka") break;
    }

    server.ServerEnd();
    return 0;
}



