#pragma once
#define NOMINMAX

#include <winsock2.h>
#include <windef.h>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "ConnServersManager.h"
#include "MatchingManager.h"

class PacketManager {
public:
    ~PacketManager() {
        redisRun = false;

        for (int i = 0; i < redisThreads.size(); i++) { // End Redis Threads
            if (redisThreads[i].joinable()) {
                redisThreads[i].join();
            }
        }
    }

    void init(const uint16_t RedisThreadCnt_);
    void PushPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_);
    void SetManager(ConnServersManager* connServersManager_, MatchingManager* matchingManager_);

private:
    bool CreateRedisThread(const uint16_t RedisThreadCnt_);
    void RedisRun(const uint16_t RedisThreadCnt_);
    void RedisThread();

    //SYSTEM
    void ImMatchingResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ImGameRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ServerDisConnect(uint16_t connObjNum_);

    // RAID
    void MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 매치 대기열 삽입
    void MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 매치 대기열 삽입
    void MatchStartFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 매칭 완료 중에 실패 (매칭이 성공하는 시점에 유저가 나갔거나, 취소를 먼저 눌렀을때)

    typedef void(PacketManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 136 bytes (병목현상 발생하면 lock_guard,mutex 사용 또는 lockfree::queue의 크기를 늘리는 방법으로 전환)
    boost::lockfree::queue<DataPacket> procSktQueue{ 512 };

    // 80 bytes
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;

    // 32 bytes
    std::vector<std::thread> redisThreads;

    // 16 bytes
    std::unique_ptr<sw::redis::RedisCluster> redis;
    std::thread redisThread;

    // 8 bytes
    ConnServersManager* connServersManager;
    MatchingManager* matchingManager;

    // 1 bytes
    bool redisRun = false;
};

