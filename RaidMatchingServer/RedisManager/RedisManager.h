#pragma once
#define NOMINMAX

#include <winsock2.h>
#include <windef.h>
#include <random>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "ServerEnum.h"
#include "ConnServersManager.h"
#include "MatchingManager.h"

constexpr int MAX_MATCHNIG_PACKET_SIZE = 128;

class RedisManager {
public:
    ~RedisManager() {
        redisRun = false;

        for (int i = 0; i < redisThreads.size(); i++) { // Shutdown Redis Threads
            if (redisThreads[i].joinable()) {
                redisThreads[i].join();
            }
        }
    }
    // ====================== INITIALIZATION =======================
    void init(const uint16_t RedisThreadCnt_);
    void SetManager(ConnServersManager* connServersManager_, MatchingManager* matchingManager_);


    // ===================== PACKET MANAGEMENT =====================
    void PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_);

private:
    // ===================== REDIS MANAGEMENT =====================
    bool CreateRedisThread(const uint16_t RedisThreadCnt_);
    void RedisRun(const uint16_t RedisThreadCnt_);
    void RedisThread();

    // ======================= MATCHING SERVER =======================
    void ImMatchingResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ImGameRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // ======================= RAID GAME SERVER =======================
    void MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void RaidEnd(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    typedef void(RedisManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;
    boost::lockfree::queue<DataPacket> procSktQueue{ MAX_MATCHNIG_PACKET_SIZE };

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
    std::atomic<bool> redisRun = false;
};

