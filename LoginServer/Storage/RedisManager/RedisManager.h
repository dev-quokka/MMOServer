#pragma once
#define NOMINMAX

#include <jwt-cpp/jwt.h>
#include <winsock2.h>
#include <windef.h>
#include <random>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "UserSyncData.h"
#include "ServerEnum.h"
#include "ConnUsersManager.h"
#include "MySQLManager.h"

const std::string JWT_SECRET = "quokka_lover";
constexpr int MAX_LOGIN_PACKET_SIZE = 128;

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
    void SetManager(ConnUsersManager* connUsersManager_);
    void SetRanking(std::vector<RANKING> ranks_);
    void SetEventPassId();


    // ===================== PACKET MANAGEMENT =====================
    void PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_);


private:
    // ===================== REDIS MANAGEMENT ======================
    bool CreateRedisThread(const uint16_t packetThreadCnt_);
    void RedisRun(const uint16_t packetThreadCnt_);
    void RedisThread();


    // ======================= LOGIN SERVER =======================
    void ImLoginResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetUserInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetConsumables(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetMaterials(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetPassInfos(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetPassRewordInfos(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GameStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    typedef void(RedisManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 136 bytes
    boost::lockfree::queue<DataPacket> procSktQueue{ MAX_LOGIN_PACKET_SIZE };

    // 80 bytes
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;

    // 32 bytes
    std::vector<std::thread> redisThreads;
    std::unordered_map<std::string, PASSINFO> eventPassInfoMap;// 현재 진행중인 패스 ID Map { 패스 ID, 패스 정보 }

    // 16 bytes
    std::unique_ptr<sw::redis::RedisCluster> redis;

    ConnUsersManager* connUsersManager;
    MySQLManager* mySQLManager;

    // 2 bytes
    uint16_t centerServerObjNum = 0;

    // 1 bytes
    std::atomic<bool>  redisRun = false;
};

