#pragma once
#define NOMINMAX

#include <jwt-cpp/jwt.h>
#include <winsock2.h>
#include <windef.h>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "RoomManager.h"
#include "ConnUsersManager.h"

class PacketManager {
public:
    ~PacketManager() {
        packetRun = false;

        for (int i = 0; i < packetThreads.size(); i++) { // End Redis Threads
            if (packetThreads[i].joinable()) {
                packetThreads[i].join();
            }
        }
    }

    void init(const uint16_t packetThreadCnt_);
    void SetManager(ConnUsersManager* connUsersManager_, RoomManager* roomManager_);
    void PushPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_); // Push Packet
    void Disconnect(uint16_t connObjNum_);

private:
    bool CreatePacketThread(const uint16_t packetThreadCnt_);
    void PacketRun(const uint16_t packetThreadCnt_);
    void PacketThread();

    void MakeRoom(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 매칭 서버에서 레이드 시작 요청 들어오면 방 생성
    void UserDisConnect(uint16_t connObjNum_); // Send Message To Center Server

    //SYSTEM
    void ImGameRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // Game Server Socket Check
    void UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 해당 서버로 유저 접속 요청 From Center Server

    void RaidTeamInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void RaidHit(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    typedef void(PacketManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

    // 5000 bytes
    thread_local static std::mt19937 gen;

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 136 bytes (병목현상 발생하면 lock_guard,mutex 사용 또는 lockfree::queue의 크기를 늘리는 방법으로 전환)
    boost::lockfree::queue<DataPacket> procSktQueue{ 512 };

    // 80 bytes
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;

    // 32 bytes
    std::vector<std::thread> packetThreads;
    std::vector<int> mapProbabilities = { 30, 30, 40 }; // 방 생성시 맵 확률 설정

    // 16 bytes
    std::unique_ptr<sw::redis::RedisCluster> redis;
    std::thread packetThread;

    // 8 bytes
    std::uniform_int_distribution<int> dist;

    ConnUsersManager* connUsersManager;
    RoomManager* roomManager;

    // 2 bytes
    uint16_t centerServerObjNum = 0;

    // 1 bytes
    bool packetRun = false;
};

