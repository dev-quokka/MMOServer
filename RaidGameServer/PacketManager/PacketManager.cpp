#include "PacketManager.h"

void PacketManger::init(const uint16_t RedisThreadCnt_) {

    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(UINT16)PACKET_ID::IM_GAME_RESPONSE] = &PacketManger::ImGameRequest;

    RedisRun(RedisThreadCnt_);
}

void PacketManger::RedisRun(const uint16_t RedisThreadCnt_) { // Connect Redis Server
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

void PacketManger::Disconnect(uint16_t connObjNum_) {
    UserDisConnect(connObjNum_);
}

void PacketManger::SetManager(ConnUsersManager* connUsersManager_) {
    connUsersManager = connUsersManager_;
}

bool PacketManger::CreateRedisThread(const uint16_t RedisThreadCnt_) {
    redisRun = true;
    for (int i = 0; i < RedisThreadCnt_; i++) {
        redisThreads.emplace_back(std::thread([this]() {RedisThread(); }));
    }
    return true;
}

void PacketManger::RedisThread() {
    DataPacket tempD(0, 0);
    ConnUser* TempConnUser = nullptr;
    char tempData[1024] = { 0 };

    while (redisRun) {
        if (procSktQueue.pop(tempD)) {
            std::memset(tempData, 0, sizeof(tempData));
            TempConnUser = connUsersManager->FindUser(tempD.connObjNum); // Find User
            PacketInfo packetInfo = TempConnUser->ReadRecvData(tempData, tempD.dataSize); // GetData
            (this->*packetIDTable[packetInfo.packetId])(packetInfo.connObjNum, packetInfo.dataSize, packetInfo.pData); // Proccess Packet
        }
        else { // Empty Queue
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void PacketManger::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}

// ============================== PACKET ==============================

//  ---------------------------- SYSTEM  ----------------------------

void PacketManger::ImGameRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<IM_GAME_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Connected Fail to the central server" << std::endl;
        return;
    }

    std::cout << "Connected to the central server" << std::endl;
}

void PacketManger::UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_CHANNEL_REQUEST*>(pPacket_);
    std::string key = "jwtcheck:{" + std::to_string(static_cast<uint16_t>(ServerType::ChannelServer01)) + "}";

    USER_CONNECT_CHANNEL_RESPONSE ucReq;
    ucReq.PacketId = (uint16_t)PACKET_ID::USER_CONNECT_CHANNEL_RESPONSE;
    ucReq.PacketLength = sizeof(USER_CONNECT_CHANNEL_RESPONSE);

    { // JWT 토큰 payload에 있는 아이디로 유저 체크
        auto tempToken = jwt::decode((std::string)userConn->userToken);
        auto tempId = tempToken.get_payload_claim("user_id");

        std::string user_id = tempId.as_string();

        if (user_id != (std::string)userConn->userId) {
            ucReq.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_CHANNEL_RESPONSE), (char*)&ucReq);
            std::cout << (std::string)userConn->userId << " JWT Check Fail" << std::endl;
            return;
        }
    }

    try {
        auto pk = static_cast<uint32_t>(std::stoul(*redis->hget(key, (std::string)userConn->userToken)));
        if (pk) {
            std::string userInfokey = "userinfo:{" + std::to_string(pk) + "}";
            std::unordered_map<std::string, std::string> userData;
            redis->hgetall(userInfokey, std::inserter(userData, userData.begin()));

            connUsersManager->FindUser(connObjNum_)->SetPk(pk);
            inGameUserManager->Set(connObjNum_, (std::string)userConn->userId, pk, std::stoul(userData["exp"]),
                static_cast<uint16_t>(std::stoul(userData["level"])), std::stoul(userData["raidScore"]));

            ucReq.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_CHANNEL_RESPONSE), (char*)&ucReq);
            std::cout << (std::string)userConn->userId << " Connect" << std::endl;
        }
        else {
            ucReq.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_CHANNEL_RESPONSE), (char*)&ucReq);
            std::cout << (std::string)userConn->userId << " JWT Check Fail" << std::endl;
        }
    }
    catch (const sw::redis::Error& e) {
        ucReq.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_CHANNEL_RESPONSE), (char*)&ucReq);
        std::cerr << "Redis error: " << e.what() << std::endl;
        return;
    }
}

void PacketManger::UserDisConnect(uint16_t connObjNum_) {
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    channelManager->LeaveChannel(tempUser->GetChannel(), connObjNum_); // 해당 채널 인원 감소

    USER_DISCONNECT_AT_CHANNEL_REQUEST userDisconnReqPacket;
    userDisconnReqPacket.PacketId = (uint16_t)PACKET_ID::USER_DISCONNECT_AT_CHANNEL_REQUEST;
    userDisconnReqPacket.PacketLength = sizeof(USER_DISCONNECT_AT_CHANNEL_REQUEST);
    userDisconnReqPacket.channelServerNum = CHANNEL_NUM;

    connUsersManager->FindUser(centerServerObjNum)->PushSendMsg(sizeof(USER_DISCONNECT_AT_CHANNEL_REQUEST), (char*)&userDisconnReqPacket);
}