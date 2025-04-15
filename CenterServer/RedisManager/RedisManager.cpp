#include "RedisManager.h"

void RedisManager::init(const uint16_t RedisThreadCnt_) {

    ServerAddressMap[ServerType::GatewayServer] = { "127.0.0.1", 9091 };
    ServerAddressMap[ServerType::MatchingServer] = { "127.0.0.1", 9131 };
    ServerAddressMap[ServerType::ChannelServer01] = { "127.0.0.1", 9211 };
    ServerAddressMap[ServerType::ChannelServer02] = { "127.0.0.1", 9221 };
    ServerAddressMap[ServerType::RaidGameServer01] = { "127.0.0.1", 9501 };


    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(uint16_t)PACKET_ID::USER_CONNECT_REQUEST] = &RedisManager::UserConnect;
    packetIDTable[(uint16_t)PACKET_ID::USER_LOGOUT_REQUEST] = &RedisManager::Logout;
    packetIDTable[(uint16_t)PACKET_ID::SERVER_USER_COUNTS_REQUEST] = &RedisManager::SendServerUserCounts;
    packetIDTable[(uint16_t)PACKET_ID::MOVE_SERVER_REQUEST] = &RedisManager::MoveServer;
    packetIDTable[(uint16_t)PACKET_ID::RAID_END_REQUEST_TO_CENTER_SERVER] = &RedisManager::RaidEnd;

    // SESSION
    packetIDTable[(uint16_t)PACKET_ID::IM_SESSION_REQUEST] = &RedisManager::ImSessionRequest;

    // CHANNEL
    packetIDTable[(uint16_t)PACKET_ID::IM_CHANNEL_REQUEST] = &RedisManager::ImChannelRequest;
    packetIDTable[(uint16_t)PACKET_ID::USER_DISCONNECT_AT_CHANNEL_REQUEST] = &RedisManager::ChannelDisConnect;

    // MATCHING
    packetIDTable[(uint16_t)PACKET_ID::IM_MATCHING_REQUEST] = &RedisManager::ImMatchingRequest;
    packetIDTable[(uint16_t)PACKET_ID::RAID_MATCHING_REQUEST] = &RedisManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchStartResponse;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST] = &RedisManager::MatchingCancel;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchingCancelResponse;


    // RAID
    packetIDTable[(uint16_t)PACKET_ID::IM_GAME_REQUEST] = &RedisManager::ImGameRequest;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_GAME_SERVER] = &RedisManager::CheckMatchSuccess;
    packetIDTable[(uint16_t)PACKET_ID::RAID_RANKING_REQUEST] = &RedisManager::GetRanking;


    channelServerObjNums.resize(3, 0); // Channel Server ID start from 1 (index 0 is not used)
    raidGameServerObjNums.resize(2, 0); // Raid Game Server ID start from 1 (index 0 is not used)

    RedisRun(RedisThreadCnt_);
    channelServersManager = new ChannelServersManager;
    channelServersManager->init();
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

void RedisManager::Disconnect(uint16_t connObjNum_) {
    if (connUsersManager->FindUser(connObjNum_)->GetPk() == 0) return; // Check the server closed
    UserDisConnect(connObjNum_);
}

void RedisManager::SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_) {
    connUsersManager = connUsersManager_;
    inGameUserManager = inGameUserManager_;
}

bool RedisManager::CreateRedisThread(const uint16_t RedisThreadCnt_) {
    redisRun = true;
    try {
        for (int i = 0; i < RedisThreadCnt_; i++) {
            redisThreads.emplace_back(std::thread([this]() { RedisThread(); }));
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Create Redis Thread Failed : " << e.what() << std::endl;
        return false;
    }

    return true;
}

void RedisManager::RedisThread() {
    DataPacket tempD(0,0);
    ConnUser* TempConnUser = nullptr;
    char tempData[1024] = {0};

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

void RedisManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_,size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}

// ============================== PACKET ==============================

//  ---------------------------- SYSTEM  ----------------------------

void RedisManager::UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_REQUEST_PACKET*>(pPacket_);
    std::string key = "jwtcheck:{" + (std::string)userConn->userId + "}";

    USER_CONNECT_RESPONSE_PACKET ucReq;
    ucReq.PacketId = (uint16_t)PACKET_ID::USER_CONNECT_RESPONSE;
    ucReq.PacketLength = sizeof(USER_CONNECT_RESPONSE_PACKET);
    try {
        auto pk = static_cast<uint32_t>(std::stoul(*redis->hget(key, (std::string)userConn->userToken)));
        if (pk) {
            std::string key = "userinfo:{" + std::to_string(pk) + "}";
            std::unordered_map<std::string, std::string> userData;
            redis->hgetall(key, std::inserter(userData, userData.begin()));

            connUsersManager->FindUser(connObjNum_)->SetPk(pk);
            inGameUserManager->GetInGameUserByObjNum(connObjNum_)->Set((std::string)userConn->userId, pk, std::stoul(userData["exp"]),
            static_cast<uint16_t>(std::stoul(userData["level"])), std::stoul(userData["raidScore"]));

            redis->hset(key, "userstate", "online"); // Change user status to "offline"

            ucReq.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);
            std::cout << (std::string)userConn->userId << " Authentication Successful" << std::endl;
        }
        else {
            ucReq.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);
            std::cout << (std::string)userConn->userId << " Authentication Failed" << std::endl;
        }
    }
    catch (const sw::redis::Error& e) {
        ucReq.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << (std::string)userConn->userId << " Authentication Failed" << std::endl;
        return;
    }
}

void RedisManager::Logout(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) { // Normal Disconnect
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    {  // Send User PK to the Session Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (uint16_t)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.userPk = tempUser->GetPk();
        connUsersManager->FindUser(GatewayServerObjNum)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);
        
        redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "offline"); // Change user status to "offline"
    }
}

void RedisManager::UserDisConnect(uint16_t connObjNum_) { // Abnormal Disconnect
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    {  // Send User PK to the Session Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (uint16_t)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.userPk = tempUser->GetPk();
        connUsersManager->FindUser(GatewayServerObjNum)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);

        redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "offline"); // Change user status to "offline"
    }
}

void RedisManager::ImSessionRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<IM_SESSION_REQUEST*>(pPacket_);
    std::cout << "Session Server Connect Request : " << connObjNum_ << std::endl;

    IM_SESSION_RESPONSE imSessionResPacket;
    imSessionResPacket.PacketId = (uint16_t)PACKET_ID::IM_SESSION_RESPONSE;
    imSessionResPacket.PacketLength = sizeof(IM_SESSION_RESPONSE);

    std::string str(userConn->Token);

    try {
        auto pk = redis->hget("jwtcheck:{sessionserver}", str);

        if (pk) {
            connUsersManager->FindUser(connObjNum_)->SetPk(0);
            GatewayServerObjNum = connObjNum_;
            imSessionResPacket.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_SESSION_RESPONSE), (char*)&imSessionResPacket);
            std::cout << "Session Server Authentication Successful" << std::endl;
        }
        else {
            imSessionResPacket.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_SESSION_RESPONSE), (char*)&imSessionResPacket);
            std::cout << "Session Server Authentication Failed" << std::endl;
            return;
        }
    }
    catch (const sw::redis::Error& e) {
        imSessionResPacket.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_SESSION_RESPONSE), (char*)&imSessionResPacket);
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Session Server Authentication Failed" << std::endl;
        return;
    }
}

void RedisManager::ImChannelRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imChReqPacket = reinterpret_cast<IM_CHANNEL_REQUEST*>(pPacket_);
    channelServerObjNums[imChReqPacket->channelServerNum] = connObjNum_; // Initialize Channel Server's unique ID

    IM_CHANNEL_RESPONSE imChRes;
    imChRes.PacketId = (uint16_t)PACKET_ID::IM_CHANNEL_RESPONSE;
    imChRes.PacketLength = sizeof(IM_CHANNEL_RESPONSE);
    imChRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&imChRes);
    std::cout << "Channel Server" << imChReqPacket->channelServerNum << " Authentication Successful" << std::endl;
}

void RedisManager::ImMatchingRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imMatchingReqPacket = reinterpret_cast<IM_MATCHING_REQUEST*>(pPacket_);
    MatchingServerObjNum = connObjNum_;  // Initialize Matching Server unique ID

    IM_MATCHING_RESPONSE imMRes;
    imMRes.PacketId = (uint16_t)PACKET_ID::IM_MATCHING_RESPONSE;
    imMRes.PacketLength = sizeof(IM_MATCHING_RESPONSE);
    imMRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_MATCHING_RESPONSE), (char*)&imMRes);
    std::cout << "Matching Server Authentication Successful"<< std::endl;
}

void RedisManager::ImGameRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imGameReqPacket = reinterpret_cast<IM_GAME_REQUEST*>(pPacket_);
    channelServerObjNums[imGameReqPacket->gameServerNum] = connObjNum_; // Initialize Game Server's unique ID

    IM_GAME_RESPONSE imGameRes;
    imGameRes.PacketId = (uint16_t)PACKET_ID::IM_GAME_RESPONSE;
    imGameRes.PacketLength = sizeof(IM_GAME_RESPONSE);
    imGameRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_GAME_RESPONSE), (char*)&imGameRes);
    std::cout << "Game Server" << imGameReqPacket->gameServerNum << " Authentication Successful" << std::endl;
}

void RedisManager::SendServerUserCounts(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
	SERVER_USER_COUNTS_RESPONSE serverUserCountsResPacket;
    serverUserCountsResPacket.PacketId = (uint16_t)PACKET_ID::SERVER_USER_COUNTS_RESPONSE;
    serverUserCountsResPacket.PacketLength = sizeof(SERVER_USER_COUNTS_RESPONSE);
    auto tempV = channelServersManager->GetServerCounts();

    char* tempC = new char[MAX_SERVER_USERS + 1];
    char* tc = tempC;
    uint16_t cnt = tempV.size();

    for (int i = 1; i < cnt; i++) {
		uint16_t userCount = tempV[i];
        memcpy(tc, (char*)&userCount, sizeof(uint16_t));
        tc += sizeof(uint16_t);
    }

    serverUserCountsResPacket.serverCount = cnt;
    std::memcpy(serverUserCountsResPacket.serverUserCnt, tempC, MAX_SERVER_USERS + 1);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&serverUserCountsResPacket);
    redis->hset("userinfo:{" + std::to_string(inGameUserManager->GetInGameUserByObjNum(connObjNum_)->GetPk()) + "}", "userstate", "serverSwitching"); // Change user status to "serverSwitching"
    
    delete[] tempC;
}

void RedisManager::ChannelDisConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto MoveCHReqPacket = reinterpret_cast<USER_DISCONNECT_AT_CHANNEL_REQUEST*>(pPacket_);
    channelServersManager->LeaveChannelServer(MoveCHReqPacket->channelServerNum);
}

void RedisManager::MoveServer(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto MoveCHReqPacket = reinterpret_cast<MOVE_SERVER_REQUEST*>(pPacket_);
    MOVE_SERVER_RESPONSE moveCHResPacket;
    std::string tag;

    if (MoveCHReqPacket->serverNum == 1) {
        moveCHResPacket.PacketId = (uint16_t)PACKET_ID::MOVE_SERVER_RESPONSE;
        moveCHResPacket.PacketLength = sizeof(MOVE_SERVER_RESPONSE);
		moveCHResPacket.port = ServerAddressMap[ServerType::ChannelServer01].port;
        strncpy_s(moveCHResPacket.ip, ServerAddressMap[ServerType::ChannelServer01].ip.c_str(), 256);
        tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::ChannelServer01)) + "}";

        if (!channelServersManager->EnterChannelServer(static_cast<uint16_t>(ChannelServerType::CH_01))) {
            moveCHResPacket.port = 0;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
        };

	}
	else if (MoveCHReqPacket->serverNum == 2) {
        moveCHResPacket.PacketId = (uint16_t)PACKET_ID::MOVE_SERVER_RESPONSE;
        moveCHResPacket.PacketLength = sizeof(MOVE_SERVER_RESPONSE);
        moveCHResPacket.port = ServerAddressMap[ServerType::ChannelServer02].port;
        strncpy_s(moveCHResPacket.ip, ServerAddressMap[ServerType::ChannelServer02].ip.c_str(), 256);
        tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::ChannelServer02)) + "}";

        if (!channelServersManager->EnterChannelServer(static_cast<uint16_t>(ChannelServerType::CH_02))) {
            moveCHResPacket.port = 0;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
        };
	}

    auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    // Generate JWT token
    std::string token = jwt::create()
        .set_issuer("Center_Server")
        .set_subject("Move_Server")
        .set_payload_claim("user_id", jwt::claim(tempUser->GetId()))
        .set_expires_at(std::chrono::system_clock::now() +
            std::chrono::seconds{ 300 })
        .sign(jwt::algorithm::hs256{ JWT_SECRET });

    std::string key = "jwtcheck:" + tag;

    auto pipe = redis->pipeline(tag);

    pipe.hset(key, token, std::to_string(tempUser->GetPk()))
        .expire(key, 300);

    pipe.exec();
    strncpy_s(moveCHResPacket.serverToken, token.c_str(), 256);
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket); // Send Channel Server address and JWT token to the user
}


//  ---------------------------- RAID  ----------------------------

void RedisManager::MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) { 
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_REQUEST_TO_MATCHING_SERVER matchReqPacket;
    matchReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_REQUEST_TO_MATCHING_SERVER;
    matchReqPacket.PacketLength = sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER);
    matchReqPacket.userPk = tempUser->GetPk();
	matchReqPacket.userCenterObjNum = connObjNum_;
    matchReqPacket.userGroupNum = tempUser->GetLevel()/3 + 1;

    connUsersManager->FindUser(MatchingServerObjNum)->PushSendMsg(sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER), (char*)&matchReqPacket);
}

void RedisManager::MatchStartResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(matchSuccessReqPacket->userCenterObjNum);

    RAID_MATCHING_RESPONSE matchResPacket;
    matchResPacket.PacketId = (uint16_t)PACKET_ID::RAID_MATCHING_RESPONSE;
    matchResPacket.PacketLength = sizeof(RAID_MATCHING_RESPONSE);

    if (matchSuccessReqPacket->isSuccess) {
        matchResPacket.insertSuccess = matchSuccessReqPacket->isSuccess;

        redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "matching"); // // Change user status to "matching"
        std::cout << tempUser->GetId() << " " << tempUser->GetLevel() / 3 + 1 <<  "group matching successful" << std::endl;
    }
    else {
        matchResPacket.insertSuccess = matchSuccessReqPacket->isSuccess;
        std::cout << tempUser->GetId() << " " << tempUser->GetLevel() / 3 + 1 << "group matching failed" << std::endl;
    }

    connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResPacket);
}

void RedisManager::MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER matchCancelReqPacket;
    matchCancelReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER;
    matchCancelReqPacket.PacketLength = sizeof(MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER);
    matchCancelReqPacket.userCenterObjNum = connObjNum_;
    matchCancelReqPacket.userGroupNum = tempUser->GetLevel() / 3 + 1;

    connUsersManager->FindUser(MatchingServerObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchCancelReqPacket);
}

void RedisManager::MatchingCancelResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchCancelResPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(matchCancelResPacket->userCenterObjNum);

    MATCHING_CANCEL_RESPONSE matchCanResPacket;
    matchCanResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE;
    matchCanResPacket.PacketLength = sizeof(MATCHING_CANCEL_RESPONSE);
    matchCanResPacket.isSuccess = matchCancelResPacket->isSuccess;

    redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "online"); // Change user status to "online"

    connUsersManager->FindUser(matchCancelResPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchCanResPacket);
    std::cout << tempUser->GetId() << " Matching Cancel Success" << std::endl;
}


void RedisManager::CheckMatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_GAME_SERVER*>(pPacket_);

    uint16_t tempRoomNum = matchSuccessReqPacket->roomNum;

    RAID_READY_REQUEST raidReadyReqPacket;
    raidReadyReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
    raidReadyReqPacket.PacketLength = sizeof(RAID_READY_REQUEST);
    raidReadyReqPacket.roomNum = tempRoomNum;
    raidReadyReqPacket.port = ServerAddressMap[ServerType::RaidGameServer01].port;
    strncpy_s(raidReadyReqPacket.ip, ServerAddressMap[ServerType::RaidGameServer01].ip.c_str(), 256);

    if (matchSuccessReqPacket->roomNum == 0) { // Send game creation failure message to matched users
        raidReadyReqPacket.roomNum = 0;
        connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum1)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
        connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum2)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
    }

    { // Generate JWT token (Consider using vector if the number exceeds 4 users)
        std::string token1 = jwt::create()
            .set_issuer("Center_Server")
            .set_subject("Connect_GameServer")
            .set_payload_claim("user_id", jwt::claim(std::to_string(matchSuccessReqPacket->userCenterObjNum1))) // User unique ID used in the Center Server
            .set_payload_claim("room_id", jwt::claim(std::to_string(tempRoomNum))) // Matched room number
            .set_payload_claim("raid_id", jwt::claim(std::to_string(matchSuccessReqPacket->userRaidServerObjNum1))) // User unique ID used in the Raid Server
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        std::string tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::RaidGameServer01)) + "}";
        std::string key = "jwtcheck:" + tag;

        auto pipe = redis->pipeline(tag);

        pipe.hset(key, token1, std::to_string(matchSuccessReqPacket->userRaidServerObjNum1)) // Save the assigned Game Server unique ID to Redis
            .expire(key, 300);

        connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum1)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

        std::string token2 = jwt::create()
            .set_issuer("Center_Server")
            .set_subject("Connect_GameServer")
            .set_payload_claim("user_id", jwt::claim(std::to_string(matchSuccessReqPacket->userCenterObjNum2))) // User unique ID used in the Center Server
            .set_payload_claim("room_id", jwt::claim(std::to_string(tempRoomNum))) // Matched room number
            .set_payload_claim("raid_id", jwt::claim(std::to_string(matchSuccessReqPacket->userRaidServerObjNum2))) // User unique ID used in the Raid Server
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        pipe.hset(key, token2, std::to_string(matchSuccessReqPacket->userRaidServerObjNum2)) // Save the assigned Game Server unique ID to Redis
            .expire(key, 150);

        connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum2)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

        pipe.exec();

        redis->hset("userinfo:{" + std::to_string(inGameUserManager->GetInGameUserByObjNum(matchSuccessReqPacket->userCenterObjNum1)->GetPk()) + "}", "userstate", "inRaid"); // Change user status to "inRaid"
        redis->hset("userinfo:{" + std::to_string(inGameUserManager->GetInGameUserByObjNum(matchSuccessReqPacket->userCenterObjNum2)->GetPk()) + "}", "userstate", "inRaid"); // Change user status to "inRaid"
    }
}

void RedisManager::MatchStartFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {

}

void RedisManager::RaidEnd(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto RaidEndPacket = reinterpret_cast<RAID_END_REQUEST_TO_CENTER_SERVER*>(pPacket_);

    RAID_END_REQUEST_TO_GAME_SERVER raidEndReqPacket;
    raidEndReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST_TO_GAME_SERVER;
    raidEndReqPacket.PacketLength = sizeof(RAID_END_REQUEST_TO_GAME_SERVER);
    raidEndReqPacket.roomNum = RaidEndPacket->roomNum;
    raidEndReqPacket.gameServerNum = RaidEndPacket->gameServerNum;

    connUsersManager->FindUser(MatchingServerObjNum)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&raidEndReqPacket); // Send the terminated room number to the matching server
}

void RedisManager::GetRanking(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<RAID_RANKING_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    RAID_RANKING_RESPONSE raidRankResPacket;
    raidRankResPacket.PacketId = (uint16_t)PACKET_ID::RAID_RANKING_RESPONSE;
    raidRankResPacket.PacketLength = sizeof(RAID_RANKING_RESPONSE);

    std::vector<std::pair<std::string, double>> scores;
    try {

        redis->zrevrange("ranking", delEquipReqPacket->startRank, 
            delEquipReqPacket->startRank + RANKING_USER_COUNT, std::back_inserter(scores));

        char* tempC = new char[MAX_SCORE_SIZE + 1];
        char* tc = tempC;
        uint16_t cnt = scores.size();

        for (int i = 0; i < cnt; i++) {
            RANKING ranking;
            strncpy_s(ranking.userId, scores[i].first.c_str(), MAX_USER_ID_LEN);
            ranking.score = scores[i].second;
            memcpy(tc, (char*)&ranking, sizeof(RANKING));
            tc += sizeof(RANKING);
        }

        raidRankResPacket.rkCount = cnt;
        std::memcpy(raidRankResPacket.reqScore, tempC, MAX_SCORE_SIZE + 1);

        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&raidRankResPacket);

        delete[] tempC;
    }
    catch (const sw::redis::Error& e) {
        raidRankResPacket.rkCount = 0;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&raidRankResPacket);
        std::cerr << "Redis error: " << e.what() << std::endl;
        return;
    }
}