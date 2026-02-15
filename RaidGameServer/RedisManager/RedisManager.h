#pragma once
#define NOMINMAX

#include <jwt-cpp/jwt.h>
#include <random>
#include <unordered_map>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "ServerEnum.h"
#include "RaidConfig.h"
#include "RoomManager.h"
#include "ConnUsersManager.h"

constexpr int MAX_RAID_PACKET_SIZE = 128;

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
    void init(const uint16_t packetThreadCnt_);
    void SetManager(ConnUsersManager* connUsersManager_, RoomManager* roomManager_);
    

    // ===================== PACKET MANAGEMENT ====================
    void PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_);
    
    
    // ==================== CONNECTION INTERFACE ==================
    void Disconnect(uint16_t connObjNum_);

private:
    // ===================== REDIS MANAGEMENT =====================
    void RedisRun(const uint16_t packetThreadCnt_);
    bool CreatePacketThread(const uint16_t packetThreadCnt_);
    void RedisThread();


    // ======================= CENTER SERVER =======================
    void CenterServerConnectResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    // ======================= MATCHING SERVER =======================
    void MatchingServerConnectResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    // ======================= RAID GAME SERVER =======================
    void MakeRoom(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void RaidTeamInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void RaidHit(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    typedef void(RedisManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

    // 5000 bytes
    thread_local static std::mt19937 gen;

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 136 bytes
    boost::lockfree::queue<DataPacket> procSktQueue{ MAX_RAID_PACKET_SIZE };

    // 80 bytes
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;

    // 32 bytes
    std::vector<std::thread> redisThreads;
    std::vector<int> mapProbabilities = { 30, 30, 40 }; // Map probabilities on room creation

    // 16 bytes
    std::unique_ptr<sw::redis::RedisCluster> redis;

    // 8 bytes
    std::uniform_int_distribution<int> dist;

    ConnUsersManager* connUsersManager;
    RoomManager* roomManager;

    // 1 bytes
    std::atomic<bool> redisRun = false;
};

