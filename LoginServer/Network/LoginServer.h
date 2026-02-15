#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include <tbb/concurrent_hash_map.h>

#include "Define.h"
#include "Packet.h"
#include "ConnUser.h"
#include "OverLappedManager.h"
#include "ConnUsersManager.h"
#include "RedisManager.h"
#include "MySQLManager.h"

constexpr uint16_t MAX_USERS_OBJECT = 13; // User objects allocated for average Gmae Server1 load + additional allocation for connected servers 

#define CENTER_SERVER_IP "127.0.0.1"
#define CENTER_SERVER_PORT 9090

class LoginServer {
public:

    // ====================== INITIALIZATION =======================
    bool init();
    bool StartWork();
    void ServerEnd();


    // ======================== CONNECTION =========================
    bool CenterServerConnect();

private:
    // ===================== THREAD MANAGEMENT =====================
    bool CreateWorkThread();
    bool CreateAccepterThread();

    void WorkThread();
    void AccepterThread();

    // 136 bytes 
    boost::lockfree::queue<ConnUser*> AcceptQueue{ MAX_USERS_OBJECT }; // For Aceept User Queue

    // 32 bytes
    std::vector<std::thread> workThreads;
    std::vector<std::thread> acceptThreads;

    // 8 bytes
    SOCKET serverSkt = INVALID_SOCKET;
    HANDLE sIOCPHandle = INVALID_HANDLE_VALUE;

    OverLappedManager* overLappedManager;
    ConnUsersManager* connUsersManager;
    RedisManager* redisManager;

    // 2 bytes
    uint16_t MaxThreadCnt = 0;

    // 1 bytes
    std::atomic<bool>  WorkRun = false;
    std::atomic<bool>  AccepterRun = false;
};