#include "PacketManager.h"

void PacketManager::init(const uint16_t RedisThreadCnt_) {
    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(uint16_t)PACKET_ID::USER_CONNECT_REQUEST] = &RedisManager::UserConnect;
    packetIDTable[(uint16_t)PACKET_ID::USER_LOGOUT_REQUEST] = &RedisManager::Logout;
    packetIDTable[(uint16_t)PACKET_ID::SERVER_USER_COUNTS_REQUEST] = &RedisManager::SendServerUserCounts;
    packetIDTable[(uint16_t)PACKET_ID::MOVE_SERVER_REQUEST] = &RedisManager::MoveServer;

    // SESSION
    packetIDTable[(uint16_t)PACKET_ID::IM_SESSION_REQUEST] = &RedisManager::ImSessionRequest;

    // CHANNEL
    packetIDTable[(uint16_t)PACKET_ID::IM_CHANNEL_REQUEST] = &RedisManager::ImChannelRequest;
    packetIDTable[(uint16_t)PACKET_ID::USER_DISCONNECT_AT_CHANNEL_REQUEST] = &RedisManager::ChannelDisConnect;

    // RAID
    packetIDTable[(uint16_t)PACKET_ID::RAID_MATCHING_REQUEST] = &RedisManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::RAID_RANKING_REQUEST] = &RedisManager::GetRanking;

    RedisRun(RedisThreadCnt_);
}

void PacketManager::RedisRun(const uint16_t RedisThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        redis = std::make_unique<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connect Success !" << std::endl;

        CreateRedisThread(RedisThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis Connect Error : " << err.what() << std::endl;
    }
}

void PacketManager::SetManager(ConnServersManager* connServersManager_) {
    connServersManager = connServersManager_;
}

bool PacketManager::CreateRedisThread(const uint16_t RedisThreadCnt_) {
    redisRun = true;
    for (int i = 0; i < RedisThreadCnt_; i++) {
        redisThreads.emplace_back(std::thread([this]() {RedisThread(); }));
    }
    return true;
}

void PacketManager::RedisThread() {
    DataPacket tempD(0, 0);
    ConnServer* TempConnServer = nullptr;
    char tempData[1024] = { 0 };

    while (redisRun) {
        if (procSktQueue.pop(tempD)) {
            std::memset(tempData, 0, sizeof(tempData));
            TempConnServer = connServersManager->FindUser(tempD.connObjNum); // Find User
            PacketInfo packetInfo = TempConnServer->ReadRecvData(tempData, tempD.dataSize); // GetData
            (this->*packetIDTable[packetInfo.packetId])(packetInfo.connObjNum, packetInfo.dataSize, packetInfo.pData); // Proccess Packet
        }
        else { // Empty Queue
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void PacketManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnServer* TempConnServer = connServersManager->FindUser(connObjNum_);
    TempConnServer->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}

// ============================== PACKET ==============================

//  ---------------------------- SYSTEM  ----------------------------

void PacketManager::ServerDisConnect(uint16_t connObjNum_) { // Abnormal Disconnect

}

//  ---------------------------- RAID  ----------------------------

void PacketManager::MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_REQUEST_TO_MATCHING_SERVER matchReqPacket;
    matchReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_REQUEST_TO_MATCHING_SERVER;
    matchReqPacket.PacketLength = sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER);
    matchReqPacket.userObjNum = connObjNum_;
    matchReqPacket.userGroupNum = tempUser->GetLevel() / 3 + 1; // 설정해둔 그룹 번호 만들어서 전달

    connUsersManager->FindUser(MatchingServerObjNum)->PushSendMsg(sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER), (char*)&matchReqPacket);
}

void PacketManager::MatchFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchResPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);

    RAID_MATCHING_RESPONSE matchResToUserPacket;
    matchResToUserPacket.PacketId = (uint16_t)PACKET_ID::RAID_MATCHING_RESPONSE;
    matchResToUserPacket.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
    matchResToUserPacket.insertSuccess = matchResPacket->isSuccess;

    connUsersManager->FindUser(matchResPacket->userObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResToUserPacket);
}

void PacketManager::MatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER*>(pPacket_);

    uint16_t tempRoomNum = matchSuccessReqPacket->roomNum;

    RAID_READY_REQUEST raidReadyReqPacket;
    raidReadyReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
    raidReadyReqPacket.PacketLength = sizeof(RAID_READY_REQUEST);
    raidReadyReqPacket.roomNum = tempRoomNum;
    raidReadyReqPacket.udpPort = 50001; // 나중에 게임 서버가 늘어나면 해당 서버로 부터 udp 포트 직접 받기
    raidReadyReqPacket.port = ServerAddressMap[ServerType::RaidGameServer01].port;
    strncpy_s(raidReadyReqPacket.ip, ServerAddressMap[ServerType::RaidGameServer01].ip.c_str(), 256);

    { // 매칭된 유저들에게 선택된 게임 서버의 ip, port와 채널 이동 간 보안을 위한 JWT Token 생성 (유저가 많아지면 vector 이용 고려)
        std::string token1 = jwt::create()
            .set_issuer("Center_Server")
            .set_subject("Connect_GameServer")
            .set_payload_claim("user_id", jwt::claim(std::to_string(matchSuccessReqPacket->userObjNum1)))  // 유저 고유번호
            .set_payload_claim("room_id", jwt::claim(std::to_string(tempRoomNum)))  // 방 번호
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        std::string tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::RaidGameServer01)) + "}";
        std::string key = "jwtcheck:" + tag;

        auto pipe = redis->pipeline(tag);

        pipe.hset(key, token1, std::to_string(matchSuccessReqPacket->userObjNum1))
            .expire(key, 300);

        connUsersManager->FindUser(matchSuccessReqPacket->userObjNum1)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

        std::string token2 = jwt::create()
            .set_issuer("Center_Server")
            .set_subject("Connect_GameServer")
            .set_payload_claim("user_id", jwt::claim(std::to_string(matchSuccessReqPacket->userObjNum2)))  // 유저 고유번호
            .set_payload_claim("room_id", jwt::claim(std::to_string(tempRoomNum)))  // 방 번호
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        pipe.hset(key, token2, std::to_string(matchSuccessReqPacket->userObjNum2))
            .expire(key, 150);

        connUsersManager->FindUser(matchSuccessReqPacket->userObjNum2)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

        pipe.exec();
    }
}

void PacketManager::MatchStartFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {

}