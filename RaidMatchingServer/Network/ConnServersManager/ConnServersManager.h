#pragma once
#include <vector>

#include "ConnServer.h"

class ConnServersManager {
public:
    ConnServersManager(uint16_t maxClientCount_) {
        connServers.resize(maxClientCount_, nullptr); // Total count of center server + game servers (0: center server)
        gameServerObjNums.resize(2, 0); // Game server ID start from 1 (index 0 is not used)
    }
    ~ConnServersManager() {
        for (int i = 0; i < connServers.size(); i++) {
            delete connServers[i];
        }
    }


    // ================== CONNECTION SERVER MANAGEMENT ==================
    void InsertServer(uint16_t connObjNum, ConnServer* connServer_); // Init ConnUsers
    ConnServer* FindServer(uint16_t connObjNum);


    // =============== CONNECTION GAME SERVER MANAGEMENT ================
    bool CheckGameServerObjNum(uint16_t idx_);
    void SetGameServerObjNum(uint16_t idx_,uint16_t gameServerObjNums_); // Set unique ID for the Game Server
    ConnServer* GetGameServerObjNum(uint16_t idx_);

private:
    // 32 bytes
    std::vector<ConnServer*> connServers; // IDX -  0: Center Server, 1 ~ : GameServerNum
    std::vector<uint16_t> gameServerObjNums; // IDX - 1 ~ : GameServerNum
};
