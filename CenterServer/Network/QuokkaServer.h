#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include <winsock2.h>
#include <windows.h>
#include <cstdint>
#include <atomic>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <queue>
#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

#include "Define.h"
#include "ConnUser.h"
#include "OverLappedManager.h"
#include "ConnUsersManager.h"
#include "InGameUserManager.h"
#include "RedisManager.h"

class QuokkaServer {
public:
    QuokkaServer(uint16_t maxClientCount_) : maxClientCount(maxClientCount_), AcceptQueue(maxClientCount_), WaittingQueue(maxClientCount_) {}

    bool init(const uint16_t MaxThreadCnt_, int port_);
    bool StartWork();
    void ServerEnd();

private:
    bool CreateWorkThread();
    bool CreateAccepterThread();

    void WorkThread(); // IOCP Complete Event Thread
    void AccepterThread(); // Accept req Thread

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
    bool WorkRun = false;
    bool AccepterRun = false;
    std::atomic<bool> UserMaxCheck = false;
};
