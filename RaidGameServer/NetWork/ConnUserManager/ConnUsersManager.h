#pragma once
#include <vector>

#include "ConnUser.h"

class ConnUsersManager {
public:
    ConnUsersManager(uint16_t maxClientCount_) : ConnUsers(maxClientCount_) {}
    ~ConnUsersManager() {
        for (int i = 0; i < ConnUsers.size(); i++) {
            delete ConnUsers[i];
        }
    }

    // ================== CONNECTION USER MANAGEMENT ==================

    void InsertUser(uint16_t connObjNum, ConnUser* connUser);
    ConnUser* FindUser(uint16_t connObjNum);

private:
    std::vector<ConnUser*> ConnUsers; // ConnUsers Obj
};
