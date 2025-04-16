#pragma once
#include <iostream>

#include "LoginServer.h"
#include "MySQLManager.h"

const int PORT = 9501;
const uint16_t maxThreadCount = 1;

int main() {
    LoginServer loginServer;

    if (!loginServer.init(maxThreadCount, PORT)) {
        return 0;
    }

    loginServer.StartWork();

    std::cout << "=== LOGIN SERVER 1 START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write login ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "login") break;
    }

    loginServer.ServerEnd();

    return 0;
}