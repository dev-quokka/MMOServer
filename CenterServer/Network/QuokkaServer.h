#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include "Define.h"
#include "ConnUser.h"
#include "OverLappedManager.h"
#include "ConnUsersManager.h"
#include "InGameUserManager.h"
#include "RedisManager.h"

class QuokkaServer {
public:
    QuokkaServer(uint16_t maxClientCount_) : maxClientCount(maxClientCount_), AcceptQueue(maxClientCount_), WaittingQueue(maxClientCount_) {}

    // ====================== INITIALIZATION =======================
    bool init();
    bool StartWork();
    void ServerEnd();

    // ==================== SERVER CONNECTION ======================
    bool CashServerConnect();

private:
    // ===================== THREAD MANAGEMENT =====================
    bool CreateWorkThread();
    bool CreateAccepterThread();
    void WorkThread();
    void AccepterThread();


    // 136 bytes 
    boost::lockfree::queue<ConnUser*> AcceptQueue; // For Aceept User Queue
    boost::lockfree::queue<ConnUser*> WaittingQueue; // Waitting User Queue

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

    // 4 bytes
    std::atomic<uint16_t> UserCnt = 0;

    // 2 bytes
    uint16_t MaxThreadCnt = 0;
    uint16_t maxClientCount = 0;

    // 1 bytes
    std::atomic<bool>  WorkRun = false;
    std::atomic<bool>  AccepterRun = false;
    std::atomic<bool> UserMaxCheck = false;
};
