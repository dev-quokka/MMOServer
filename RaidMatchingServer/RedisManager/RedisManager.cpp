#include "RedisManager.h"

// ========================== INITIALIZATION =========================

void RedisManager::init(const uint16_t RedisThreadCnt_) {

    // -------------------- SET PACKET HANDLERS ----------------------
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    packetIDTable[(uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_RESPONSE] = &RedisManager::ImMatchingResponse;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_REQUEST_TO_MATCHING_SERVER] = &RedisManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER] = &RedisManager::MatchingCancel;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_REQUEST_FROM_RAID_SERVER] = &RedisManager::ImGameRequest;

    RedisRun(RedisThreadCnt_);
}

void RedisManager::RedisRun(const uint16_t RedisThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        redis = std::make_unique<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connected" << std::endl;

        CreateRedisThread(RedisThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis Connect Error : " << err.what() << std::endl;
    }
}

void RedisManager::SetManager(ConnServersManager* connServersManager_, MatchingManager* matchingManager_) {
    connServersManager = connServersManager_;
    matchingManager = matchingManager_;
}


// ===================== PACKET MANAGEMENT =====================

void RedisManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnServer* TempConnServer = connServersManager->FindServer(connObjNum_);
    TempConnServer->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}


// ====================== REDIS MANAGEMENT =====================

bool RedisManager::CreateRedisThread(const uint16_t RedisThreadCnt_) {
    redisRun = true;

    try {
        for (int i = 0; i < RedisThreadCnt_; i++) {
            redisThreads.emplace_back(std::thread([this]() { RedisThread(); }));
        }
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Create Redis Thread Failed : " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }

    return true;
}

void RedisManager::RedisThread() {
    DataPacket tempD(0, 0);
    ConnServer* TempConnServer = nullptr;
    char tempData[1024] = { 0 };

    while (redisRun) {
        if (procSktQueue.pop(tempD)) {
            std::memset(tempData, 0, sizeof(tempData));
            TempConnServer = connServersManager->FindServer(tempD.connObjNum); // Find User
            PacketInfo packetInfo = TempConnServer->ReadRecvData(tempData, tempD.dataSize); // GetData
            (this->*packetIDTable[packetInfo.packetId])(packetInfo.connObjNum, packetInfo.dataSize, packetInfo.pData);
        }
        else { // Empty Queue
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}


// ======================================================= MATCHING SERVER =======================================================

void RedisManager::ImMatchingResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<MATCHING_SERVER_CONNECT_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Failed to Authenticate with Center Server" << std::endl;
        return;
    }

    std::cout << "Successfully Authenticated with Center Server" << std::endl;
}

void RedisManager::ImGameRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_){
    auto centerConn = reinterpret_cast<MATCHING_SERVER_CONNECT_REQUEST_FROM_RAID_SERVER*>(pPacket_);
    auto tempNum = centerConn->gameServerNum;

    MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER imResPacket;
    imResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER;
    imResPacket.PacketLength = sizeof(MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER);

    if (!connServersManager->CheckGameServerObjNum(tempNum)) { // 이미 번호 있으면 연결 실패 전송
        std::cout << "Failed to Authenticated with Game Server" << tempNum << std::endl;
        imResPacket.isSuccess = false;
    }
    else {
        connServersManager->SetGameServerObjNum(tempNum, connObjNum_);
        imResPacket.isSuccess = true;
        std::cout << "Successfully Authenticated with Game Server" << tempNum << std::endl;
    }

    connServersManager->FindServer(connObjNum_)->PushSendMsg(sizeof(MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER), (char*)&imResPacket);
}


// ======================================================= RAID GAME SERVER =======================================================

void RedisManager::MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchingReqPacket = reinterpret_cast<MATCHING_REQUEST_TO_MATCHING_SERVER*>(pPacket_);

    MATCHING_RESPONSE_FROM_MATCHING_SERVER matchResPacket;
    matchResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_MATCHING_SERVER;
    matchResPacket.PacketLength = sizeof(MATCHING_RESPONSE_FROM_MATCHING_SERVER);
    matchResPacket.userCenterObjNum = matchingManager->Insert(matchingReqPacket->userPk, matchingReqPacket->userCenterObjNum, matchingReqPacket->userGroupNum);
    
    if (matchResPacket.userCenterObjNum == 0) matchResPacket.isSuccess = false;
    else matchResPacket.isSuccess = true;

    connServersManager->FindServer(connObjNum_)->PushSendMsg(sizeof(MATCHING_RESPONSE_FROM_MATCHING_SERVER), (char*)&matchResPacket);
}

void RedisManager::MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_){
    auto matchingReqPacket = reinterpret_cast<MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER*>(pPacket_);

    MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER matchCancelResPacket;
    matchCancelResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER;
    matchCancelResPacket.PacketLength = sizeof(MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER);

    if (!matchingManager->CancelMatching(matchingReqPacket->userCenterObjNum, matchingReqPacket->userGroupNum)) {
        matchCancelResPacket.isSuccess = false;
    }
    else matchCancelResPacket.isSuccess = true;

    connServersManager->FindServer(connObjNum_)->PushSendMsg(sizeof(MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER), (char*)&matchCancelResPacket);
}