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
#include "InGameUser.h"
#include "ServerChannelEnum.h"
#include "InGameUserManager.h"
#include "ChannelServersManager.h"
#include "ConnUsersManager.h"

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

    void init(const uint16_t RedisThreadCnt_);
    void SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_);
    void PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_);
    void Disconnect(uint16_t connObjNum_);

private:
    bool CreateRedisThread(const uint16_t RedisThreadCnt_);
    void RedisRun(const uint16_t RedisThreadCnt_);
    void RedisThread();

    // SYSTEM
    void UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void Logout(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void UserDisConnect(uint16_t connObjNum_);
    void ImSessionRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); 
    void ImChannelRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ImMatchingRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ImGameRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void SendServerUserCounts(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MoveServer(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // CHANNEL
    void ChannelDisConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // MATCHING
    void MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchStartResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MatchingCancelResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
	void CheckMatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
	void MatchStartFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    
    // RAID
    void RaidEnd(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void GetRanking(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    typedef void(RedisManager::* RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);

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

    ConnUsersManager* connUsersManager;
    InGameUserManager* inGameUserManager;
    ChannelServersManager* channelServersManager;

    // 2 bytes
    uint16_t GatewayServerObjNum = 0;
    uint16_t MatchingServerObjNum = 0;

    // 1 bytes
    bool redisRun = false;
};

