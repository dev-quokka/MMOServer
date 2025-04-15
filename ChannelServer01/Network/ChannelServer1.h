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

#include "Packet.h"
#include "Define.h"
#include "ConnUser.h"
#include "OverLappedManager.h"
#include "ConnUsersManager.h"
#include "InGameUserManager.h"
#include "RedisManager.h"

constexpr uint16_t MAX_USERS_OBJECT = 30; // User objects allocated for average Channel Server1 load
constexpr int MAX_CHANNEL1_USERS_COUNT = 30; // // Maximum number of users per channel * number of channel

class ChannelServer1 {
public:
    bool init(const uint16_t MaxThreadCnt_, int port_);
    bool CenterConnect();
    bool StartWork();
    void ServerEnd();

    bool CenterServerConnect();
private:
    bool CreateWorkThread();
    bool CreateAccepterThread();

    void WorkThread();
    void AccepterThread();

    // 136 bytes 
    boost::lockfree::queue<ConnUser*> AcceptQueue{ MAX_USERS_OBJECT };

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
    bool WorkRun = false;
    bool AccepterRun = false;
};