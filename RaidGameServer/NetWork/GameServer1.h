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

#include "Packet.h"
#include "Define.h"
#include "ConnUser.h"

#include "OverLappedManager.h"
#include "ConnUsersManager.h"
#include "RoomManager.h"
#include "PacketManager.h"

constexpr uint16_t MAX_USERS_OBJECT = 13; // 1서버 평균 접속 유저를 10으로 가정하고 미리 동적할당한 유저 객체 (30 + 중앙 서버, 매칭 서버 + 1)
constexpr int MAX_CHANNEL1_USERS_COUNT = 10; // 각 채널 접속 가능 인원 * 채널 수

#define CENTER_SERVER_IP "127.0.0.1"
#define CENTER_SERVER_PORT 9090

#define MATCHING_SERVER_IP "127.0.0.1"
#define MATCHING_SERVER_PORT 9131

class GameServer1 {
public:
    bool init(const uint16_t MaxThreadCnt_, int port_);
    bool CenterServerConnect();
    bool MatchingServerConnect();
    bool StartWork();
    void ServerEnd();

private:
    bool CreateWorkThread();
    bool CreateAccepterThread();

    void WorkThread(); // IOCP Complete Event Thread
    void AccepterThread(); // Accept req Thread

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
    RoomManager* roomManager;
    PacketManager* packetManager;

    // 2 bytes
    uint16_t MaxThreadCnt = 0;

    // 1 bytes
    bool WorkRun = false;
    bool AccepterRun = false;
};