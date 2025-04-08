#pragma once
#pragma comment(lib, "ws2_32.lib") // 소켓 프로그래밍용
#pragma comment(lib, "mswsock.lib") // AcceptEx 사용

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

#define SERVER_IP "127.0.0.1"
#define CENTER_SERVER_PORT 9090

constexpr uint16_t MAX_CHANNEL1_USER_COUNT = 10; // 1서버 평균 접속 유저를 10으로 가정하고 미리 동적할당한 유저 객체 (나중에 1서버 평균 유저 늘어나면 카운트 수정)

class ChannelServer1 {
public:
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