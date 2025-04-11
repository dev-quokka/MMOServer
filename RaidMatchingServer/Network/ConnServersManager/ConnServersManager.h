#pragma once

#include <winsock2.h>
#include <iostream>
#include <vector>

#include "ConnServer.h"

class ConnServersManager {
public:
    ConnServersManager(uint16_t maxClientCount_) : connServers(maxClientCount_) {}
    ~ConnServersManager() {
        for (int i = 0; i < connServers.size(); i++) {
            delete connServers[i];
        }
    }

    void InsertUser(uint16_t connObjNum, ConnServer* connServer_); // Init ConnUsers
    ConnServer* FindUser(uint16_t connObjNum);

private:
    // 576 bytes
    std::vector<ConnServer*> connServers; // IDX -  0: Center Server, 1 ~ : GameServer
};
