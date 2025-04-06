#pragma once

#include "Define.h"
#include "ConnUser.h"

#include "OverLappedManager.h"
#include "UdpOverLappedManager.h"
#include "ConnUsersManager.h"
#include "InGameUserManager.h"
#include "RedisManager.h"

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

#pragma comment(lib, "ws2_32.lib") // 소켓 프로그래밍용
#pragma comment(lib, "mswsock.lib") // AcceptEx 사용

class QuokkaServer {
public:
    QuokkaServer(uint16_t maxClientCount_) : maxClientCount(maxClientCount_), AcceptQueue(maxClientCount_), WaittingQueue(maxClientCount_) {}

    void SetServerAddressMap();
    bool init(const uint16_t MaxThreadCnt_, int port_);
    bool StartWork();
    void ServerEnd();

private:
    bool CreateWorkThread();
    bool CreateAccepterThread();

    void WorkThread(); // IOCP Complete Event Thread
    void AccepterThread(); // Accept req Thread

    // 1 bytes
    bool WorkRun = false;
    bool udpWorkRun = false;
    bool AccepterRun = false;
    std::atomic<bool> UserMaxCheck = false;

    // 2 bytes
    uint16_t MaxThreadCnt = 0;
    uint16_t maxClientCount = 0;

    // 4 bytes
    std::atomic<int> UserCnt = 0; //Check Current UserCnt

    // 8 bytes
    SOCKET serverSkt = INVALID_SOCKET;
    HANDLE sIOCPHandle = INVALID_HANDLE_VALUE;
    SOCKET udpSkt = INVALID_SOCKET;
 
    OverLappedManager* overLappedManager;
    UdpOverLappedManager* udpOverLappedManager;
    ConnUsersManager* connUsersManager;
    InGameUserManager* inGameUserManager;
    RedisManager* redisManager;

    std::thread udpWorkThread;

    // 32 bytes
    std::vector<std::thread> workThreads;
    std::vector<std::thread> acceptThreads;

    // 136 bytes 
    boost::lockfree::queue<ConnUser*> AcceptQueue; // For Aceept User Queue
    boost::lockfree::queue<ConnUser*> WaittingQueue; // Waitting User Queue
};
