#pragma once
#include <iostream>

#include "LoginServer.h"
#include "MySQLManager.h"

const int PORT = 9091;
const uint16_t maxThreadCount = 1;

int main() {
    LoginServer loginServer;

    loginServer.init(maxThreadCount, PORT);
    loginServer.StartWork();

    std::cout << "=== LOGIN SERVER START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write login ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "login") break;
    }

    loginServer.ServerEnd();

    return 0;
}