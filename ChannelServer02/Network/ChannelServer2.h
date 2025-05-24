#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include <tbb/concurrent_hash_map.h>

#include "Packet.h"
#include "Define.h"
#include "ConnUser.h"
#include "OverLappedManager.h"
#include "ConnUsersManager.h"
#include "InGameUserManager.h"
#include "RedisManager.h"

constexpr uint16_t MAX_USERS_OBJECT = 30; // User objects allocated for average Channel Server1 load
constexpr uint16_t MAX_CHANNEL1_USERS_COUNT = 10; // // Maximum number of users per channel * number of channel

class ChannelServer2 {
public:
    // ====================== INITIALIZATION =======================
    bool init(const uint16_t MaxThreadCnt_, int port_);
    bool StartWork();
    void ServerEnd();


    // ==================== SERVER CONNECTION ======================
    bool CenterServerConnect();


private:
    // ===================== THREAD MANAGEMENT =====================
    bool CreateWorkThread();
    bool CreateAccepterThread();
    void WorkThread();
    void AccepterThread();


    // 136 bytes 
    boost::lockfree::queue<ConnUser*> AcceptQueue{ MAX_USERS_OBJECT + 1 };

    // 32 bytes
    std::vector<std::thread> workThreads;
    std::vector<std::thread> acceptThreads;

    // 8 bytes
    SOCKET serverSkt = INVALID_SOCKET;
    HANDLE sIOCPHandle = INVALID_HANDLE_VALUE;

    OverLappedManager* overLappedManager;
    ConnUsersManager* connUsersManager;
    InGameUserManager* inGameUserManager;
    RedisManager* redisManager;

    // 2 bytes
    uint16_t MaxThreadCnt = 0;

    // 1 bytes
    std::atomic<bool>  WorkRun = false;
    std::atomic<bool>  AccepterRun = false;
};