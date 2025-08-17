#pragma once
#define NOMINMAX

#include <jwt-cpp/jwt.h>
#include <winsock2.h>
#include <windef.h>
#include <random>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "InGameUser.h"
#include "ServerChannelEnum.h"
#include "InGameUserManager.h"
#include "ChannelServersManager.h"
#include "ConnUsersManager.h"
#include "MySQLManager.h"
#include "ShopDataManager.h"
#include "PassRewardManager.h"

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

    // ======================= TEST =======================
    void Test_CashCahrge(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    
    // ====================== INITIALIZATION =======================
    void init(const uint16_t RedisThreadCnt_);
    void SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_);
    void InitItemData();
    void InitShopData();
    void InitPassData();


    // ===================== PACKET MANAGEMENT =====================
    void PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_);


    // ==================== CONNECTION INTERFACE ===================
    void Disconnect(uint16_t connObjNum_);


private:
    // ===================== REDIS MANAGEMENT =====================
    void RedisRun(const uint16_t RedisThreadCnt_);
    bool CreateRedisThread(const uint16_t RedisThreadCnt_);
    void RedisThread();


    // ======================= SYNCRONIZATION =======================
    USERINFO GetUpdatedUserInfo(uint16_t userPk_);
    std::vector<EQUIPMENT> GetUpdatedEquipment(uint16_t userPk_);
    std::vector<CONSUMABLES> GetUpdatedConsumables(uint16_t userPk_);
    std::vector<MATERIALS> GetUpdatedMaterials(uint16_t userPk_);
    std::vector<UserPassDataForSync> GetUpdatedPassData(uint16_t userPk_);
    
    // ======================= CENTER SERVER =======================
    void UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void UserDisConnect(uint16_t connObjNum_);
    void SendServerUserCounts(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void SendShopDataToClient(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MoveServer(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void BuyItemFromShop(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void SendPassDataToClient(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void PassExpUp(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetPassItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // ======================= CASH SERVER =======================
    void CashServerConnectResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void CashChargeResultResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // ======================== LOGIN SERVER =======================
    void LoginServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    // ======================= CHANNEL SERVER =======================
    void ChannelServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ChannelDisConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void SyncEqipmentEnhace(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    // ======================= MATCHING SERVER =======================
    void MatchingServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchStartResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchingCancelResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
	void CheckMatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    

    // ======================= RAID GAME SERVER =======================
    void GameServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void SyncUserRaidScore(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    typedef void(RedisManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 136 bytes
    boost::lockfree::queue<DataPacket> procSktQueue{ MAX_CENTER_PACKET_SIZE };

    // 80 bytes
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;
    std::unordered_map<std::string, std::vector<uint16_t>> missionMap;

    // 32 bytes
    std::vector<std::thread> redisThreads;

    // 8 bytes
    std::unique_ptr<sw::redis::RedisCluster> redis;

    MySQLManager* mySQLManager;
    ConnUsersManager* connUsersManager;
    InGameUserManager* inGameUserManager;
    ChannelServersManager* channelServersManager;

    // 1 bytes
    bool redisRun = false;
};

