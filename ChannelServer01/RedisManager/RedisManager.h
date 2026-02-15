#pragma once
#define NOMINMAX

#include <jwt-cpp/jwt.h>
#include <winsock2.h>
#include <windef.h>
#include <random>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "LuaScript.h"
#include "InGameUser.h"
#include "ServerChannelEnum.h"
#include "InGameUserManager.h"
#include "ChannelManager.h"
#include "ConnUsersManager.h"

constexpr int MAX_CHANNEL_PACKET_SIZE = 128;

class RedisManager {
public:
    ~RedisManager() {
        redisRun = false;

        for (int i = 0; i < redisThreads.size(); i++) { // End Redis Threads
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
    

	// ==================== INGAME MANAGEMENT ======================
    bool EquipmentEnhance(uint16_t currentEnhanceCount_);


    // ==================== CONNECTION INTERFACE ===================
    void Disconnect(uint16_t connObjNum_);

private:
    // ===================== REDIS MANAGEMENT =====================
    bool RedisRun(const uint16_t RedisThreadCnt_);
    bool CreateRedisThread(const uint16_t RedisThreadCnt_);
    void RedisThread();


    // ======================= CENTER SERVER =======================
    void ChannelServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    // ======================= CHANNEL SERVER =======================
    // SYSTEM
	void UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); 
    void UserDisConnect(uint16_t connObjNum_); 
    void SendChannelUserCounts(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MoveChannel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetRanking(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

	// USER STATUS
    void ExpUp(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // INVENTORY
    void AddItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void DeleteItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ModifyItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MoveItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // INVENTORY:EQUIPMENT
    void AddEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void DeleteEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void EnhanceEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MoveEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    typedef void(RedisManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

    // 5000 bytes
    thread_local static std::mt19937 gen;

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 136 bytes 
    boost::lockfree::queue<DataPacket> procSktQueue{ MAX_CHANNEL_PACKET_SIZE };

    // 80 bytes
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;

    // 32 bytes
    std::vector<std::thread> redisThreads;
    std::vector<uint16_t> enhanceProbabilities = { 100,80,60,40,20,10,5,3,2,1 }; // Equipment enhance probabilities
    std::vector<unsigned int> mobExp = { 0,1,2,3,4,5,6,7,8,9,10 }; // Experience points for each monster based on monster index
    std::vector<std::string> itemType = { "equipment", "consumables", "materials" };

    // 16 bytes
    std::unique_ptr<sw::redis::RedisCluster> redis;
    std::thread redisThread;

    // 8 bytes
    std::uniform_int_distribution<int> dist;

    ConnUsersManager* connUsersManager;
    InGameUserManager* inGameUserManager;
    ChannelManager* channelManager;

    // 1 bytes
    bool redisRun = false;
};

