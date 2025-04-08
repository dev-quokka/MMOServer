#pragma once

#include <jwt-cpp/jwt.h>
#include <winsock2.h>
#include <windef.h>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "InGameUser.h"
#include "InGameUserManager.h"
#include "ChannelServersManager.h"
#include "ConnUsersManager.h"

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

    void init(const uint16_t RedisThreadCnt_, const uint16_t maxClientCount_, const HANDLE sIOCPHandle_);
    void SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_);
    void PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_); // Push Redis Packet
    void Disconnect(uint16_t connObjNum_);

private:
    bool CreateRedisThread(const uint16_t RedisThreadCnt_);
    bool EquipmentEnhance(uint16_t currentEnhanceCount_);
    void RedisRun(const uint16_t RedisThreadCnt_);
    void RedisThread();

    //SYSTEM
    void UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // User Connect
    void Logout(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // Normal Disconnect (Set Short Time TTL)
    void UserDisConnect(uint16_t connObjNum_); // Abnormal Disconnect (Set Long Time TTL)
    void ImSessionRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // Session Server Socket Check
    void SendServerUserCounts(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 서버당 유저 수 요청 (유저가 서버 이동 화면으로 오면 전송)
    void MoveServer(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 채널 서버 이동 요청

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

    // RAID
    void MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 매치 대기열 삽입
    void MatchFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // 레이드 매칭 실패시 전달 받는 패킷
    void MatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // Center Server to Matching Server
    void MatchStartFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // Matching Server to Center Server
    void GetRanking(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    typedef void(RedisManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

    // 5000 bytes
    thread_local static std::mt19937 gen;

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 136 bytes (병목현상 발생하면 lock_guard,mutex 사용 또는 lockfree::queue의 크기를 늘리는 방법으로 전환)
    boost::lockfree::queue<DataPacket> procSktQueue{ 512 };

    // 80 bytes
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;

    // 32 bytes
    std::vector<std::thread> redisThreads;
    std::vector<uint16_t> enhanceProbabilities = { 100,90,80,70,60,50,40,30,20,10 };
    std::vector<unsigned int> mobExp = { 0,1,2,3,4,5,6,7,8,9,10 };
    std::vector<std::string> itemType = { "equipment", "consumables", "materials" };

    // 16 bytes
    std::unique_ptr<sw::redis::RedisCluster> redis;
    std::thread redisThread;

    // 8 bytes
    std::uniform_int_distribution<int> dist;

    ConnUsersManager* connUsersManager;
    InGameUserManager* inGameUserManager;
    ChannelServersManager* channelServersManager;

    // 2 bytes
    uint16_t GatewayServerObjNum = 0;
    uint16_t MatchingServerObjNum = 0;
    uint16_t RaidGameServerObjNum = 0;

    // 1 bytes
    bool redisRun = false;
};

