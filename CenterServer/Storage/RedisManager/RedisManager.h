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
#include "UserSyncData.h"
#include "InGameUser.h"
#include "ServerChannelEnum.h"
#include "InGameUserManager.h"
#include "ChannelServersManager.h"
#include "ConnUsersManager.h"
#include "MySQLManager.h"

const std::string JWT_SECRET = "Cute_Quokka";
constexpr int MAX_CENTER_PACKET_SIZE = 256;

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
    void SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_);


    // ===================== PACKET MANAGEMENT =====================
    void PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_);


    // ==================== CONNECTION INTERFACE ===================
    void Disconnect(uint16_t connObjNum_);

private:
    // ===================== REDIS MANAGEMENT =====================
    void RedisRun(const uint16_t RedisThreadCnt_);
    bool CreateRedisThread(const uint16_t RedisThreadCnt_);
    void RedisThread();

    typedef void(RedisManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);


    // ======================= SYNCRONIZATION =======================
    USERINFO GetUpdatedUserInfo(uint16_t userPk_);
    std::vector<EQUIPMENT> GetUpdatedEquipment(uint16_t userPk_);
    std::vector<CONSUMABLES> GetUpdatedConsumables(uint16_t userPk_);
    std::vector<MATERIALS> GetUpdatedMaterials(uint16_t userPk_);


    // ======================= CENTER SERVER =======================
    void UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void Logout(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void UserDisConnect(uint16_t connObjNum_);
    void LoginServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); 
    void ChannelServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchingServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GameServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void SendServerUserCounts(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MoveServer(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetRanking(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    // ======================= CHANNEL SERVER =======================
    void ChannelDisConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    // ======================= MATCHING SERVER =======================
    void MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchStartResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchingCancelResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
	void CheckMatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    

    // ======================= RAID GAME SERVER =======================
    void SyncUserRaidScore(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);



    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 136 bytes
    boost::lockfree::queue<DataPacket> procSktQueue{ MAX_CENTER_PACKET_SIZE };

    // 80 bytes
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;
    std::unordered_map<ServerType, ServerAddress> ServerAddressMap;

    // 32 bytes
    std::vector<uint16_t> channelServerObjNums;
    std::vector<uint16_t> raidGameServerObjNums;
    std::vector<std::thread> redisThreads;
    
    // 16 bytes
    std::unique_ptr<sw::redis::RedisCluster> redis;
    std::thread redisThread;

    MySQLManager* mySQLManager;
    ConnUsersManager* connUsersManager;
    InGameUserManager* inGameUserManager;
    ChannelServersManager* channelServersManager;

    // 2 bytes
    uint16_t GatewayServerObjNum = 0;
    uint16_t MatchingServerObjNum = 0;

    // 1 bytes
    bool redisRun = false;
};

