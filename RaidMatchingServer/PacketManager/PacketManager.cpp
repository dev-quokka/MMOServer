#include "PacketManager.h"

void PacketManager::init(const uint16_t RedisThreadCnt_) {
    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_REQUEST_TO_MATCHING_SERVER] = &PacketManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_REQUEST_TO_MATCHING_SERVER] = &PacketManager::MatchingCancel;

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

void PacketManager::SetManager(ConnServersManager* connServersManager_, MatchingManager* matchingManager_) {
    connServersManager = connServersManager_;
    matchingManager = matchingManager_;
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

void PacketManager::PushPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnServer* TempConnServer = connServersManager->FindUser(connObjNum_);
    TempConnServer->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}

// ============================== PACKET ==============================

//  ---------------------------- SYSTEM  ----------------------------

void PacketManager::ImMatchingRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<IM_MATCHING_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Connected Fail to the central server" << std::endl;
        return;
    }

    std::cout << "Connected to the central server" << std::endl;
}

void PacketManager::ServerDisConnect(uint16_t connObjNum_) { // Abnormal Disconnect

}

//  ---------------------------- RAID  ----------------------------

void PacketManager::MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchingReqPacket = reinterpret_cast<MATCHING_REQUEST_TO_MATCHING_SERVER*>(pPacket_);

    MATCHING_RESPONSE_FROM_MATCHING_SERVER matchResPacket;
    matchResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_MATCHING_SERVER;
    matchResPacket.PacketLength = sizeof(MATCHING_RESPONSE_FROM_MATCHING_SERVER);

    // 매칭 쓰레드 insert 성공 여부 전달
    matchResPacket.isSuccess = matchingManager->Insert(matchingReqPacket->userPk, matchingReqPacket->userCenterObjNum, matchingReqPacket->userGroupNum);

    connServersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATCHING_RESPONSE_FROM_MATCHING_SERVER), (char*)&matchResPacket);
}

void PacketManager::MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_){
    auto matchingReqPacket = reinterpret_cast<MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER*>(pPacket_);

    MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER matchCancelResPacket;
    matchCancelResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER;
    matchCancelResPacket.PacketLength = sizeof(MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER);

    // 매칭 취소 성공 여부 전달
    matchCancelResPacket.isSuccess = matchingManager->CancelMatching(matchingReqPacket->userCenterObjNum, matchingReqPacket->userGroupNum);

    connServersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER), (char*)&matchCancelResPacket);
}