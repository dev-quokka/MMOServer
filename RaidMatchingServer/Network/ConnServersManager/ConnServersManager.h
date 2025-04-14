#pragma once

#include <winsock2.h>
#include <iostream>
#include <vector>

#include "ConnServer.h"

class ConnServersManager {
public:
    ConnServersManager(uint16_t maxClientCount_) {
        connServers.resize(maxClientCount_, nullptr); // 중앙 서버 + 게임 서버 수 (0 : 중앙 서버)
        gameServerObjNums.resize(2, 0); // 게임 서버 수 + 1 (0 사용 X)
    }
    ~ConnServersManager() {
        for (int i = 0; i < connServers.size(); i++) {
            delete connServers[i];
        }
    }

    void InsertUser(uint16_t connObjNum, ConnServer* connServer_); // Init ConnUsers
    ConnServer* FindUser(uint16_t connObjNum);
    bool CheckGameServerObjNum(uint16_t idx_);
    void SetGameServerObjNum(uint16_t idx_, uint16_t gameServerObjNums_); // 게임 서버 고유 번호 설정
    ConnServer* GetGameServerObjNum(uint16_t idx_);

private:
    // 32 bytes
    std::vector<ConnServer*> connServers; // IDX -  0: Center Server, 1 ~ : GameServerNum
    std::vector<uint16_t> gameServerObjNums; // IDX - 1 ~ : GameServerNum
};
