#include "QuokkaServer.h"
#include <iostream>
#include <cstdint>

const uint16_t PORT = 9090;
const uint16_t maxThreadCount = 1;
const uint16_t maxClientCount = 5;

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



