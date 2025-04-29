#include "RedisManager.h"

// ========================== INITIALIZATION =========================

void RedisManager::init(const uint16_t RedisThreadCnt_) {

    // -------------------- SET PACKET HANDLERS ----------------------
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // CENTER
    packetIDTable[(uint16_t)PACKET_ID::USER_CONNECT_REQUEST] = &RedisManager::UserConnect;
    packetIDTable[(uint16_t)PACKET_ID::SERVER_USER_COUNTS_REQUEST] = &RedisManager::SendServerUserCounts;
    packetIDTable[(uint16_t)PACKET_ID::MOVE_SERVER_REQUEST] = &RedisManager::MoveServer;

    // LOGIN
    packetIDTable[(uint16_t)PACKET_ID::LOGIN_SERVER_CONNECT_REQUEST] = &RedisManager::LoginServerConnectRequest;

    // CHANNEL
    packetIDTable[(uint16_t)PACKET_ID::CHANNEL_SERVER_CONNECT_REQUEST] = &RedisManager::ChannelServerConnectRequest;
    packetIDTable[(uint16_t)PACKET_ID::USER_DISCONNECT_AT_CHANNEL_REQUEST] = &RedisManager::ChannelDisConnect;
    packetIDTable[(uint16_t)PACKET_ID::SYNC_EQUIPMENT_ENHANCE_REQUEST] = &RedisManager::SyncEqipmentEnhace;

    // MATCHING
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_REQUEST] = &RedisManager::MatchingServerConnectRequest;
    packetIDTable[(uint16_t)PACKET_ID::RAID_MATCHING_REQUEST] = &RedisManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchStartResponse;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST] = &RedisManager::MatchingCancel;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchingCancelResponse;

    // RAID GAME
    packetIDTable[(uint16_t)PACKET_ID::RAID_SERVER_CONNECT_REQUEST] = &RedisManager::GameServerConnectRequest;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_GAME_SERVER] = &RedisManager::CheckMatchSuccess;
    packetIDTable[(uint16_t)PACKET_ID::SYNC_HIGHSCORE_REQUEST] = &RedisManager::SyncUserRaidScore;

    RedisRun(RedisThreadCnt_);

    channelServersManager = new ChannelServersManager;
    channelServersManager->init();
}

void RedisManager::SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_) {
    connUsersManager = connUsersManager_;
    inGameUserManager = inGameUserManager_;
}


// ===================== PACKET MANAGEMENT =====================

void RedisManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}


// ==================== CONNECTION INTERFACE ===================

void RedisManager::Disconnect(uint16_t connObjNum_) {
    if (connUsersManager->FindUser(connObjNum_)->GetPk() == 0) return; // Check the server closed
    UserDisConnect(connObjNum_);
}


// ====================== REDIS MANAGEMENT =====================

void RedisManager::RedisRun(const uint16_t RedisThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        redis = std::make_unique<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connected" << std::endl;

        mySQLManager = new MySQLManager;
        mySQLManager->init();

        CreateRedisThread(RedisThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis Connect Error : " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
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


// ================== SYNCRONIZATION ==================

USERINFO RedisManager::GetUpdatedUserInfo(uint16_t userPk_) {
    std::string userInfokey = "userinfo:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> userData;

    USERINFO tempUser;

    try {
        redis->hgetall(userInfokey, std::inserter(userData, userData.begin()));

        tempUser.userId = userData["userId"];
        tempUser.raidScore = std::stoul(userData["raidScore"]);
        tempUser.exp = std::stoul(userData["exp"]);
        tempUser.level = static_cast<uint16_t>(std::stoi(userData["level"]));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Failed to Get UserInfo for UserPk: " << userPk_ << std::endl;
        return tempUser;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }

    return tempUser;
}

std::vector<EQUIPMENT> RedisManager::GetUpdatedEquipment(uint16_t userPk_) {
    std::string eqSlot = "equipment:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> equipments;

    std::vector<EQUIPMENT> tempEqv;

    try {
        redis->hgetall(eqSlot, std::inserter(equipments, equipments.begin()));

        for (auto& [key, value] : equipments) {
            size_t div = value.find(':');

            if (div == std::string::npos) {
                std::cout << "Invalid data format in Redis for position: " << key << std::endl;
                continue;
            }

            EQUIPMENT eq;
            eq.position = static_cast<uint16_t>(std::stoi(key));
            eq.itemCode = static_cast<uint16_t>(std::stoi(value.substr(0, div)));
            eq.enhance = static_cast<uint16_t>(std::stoi(value.substr(div + 1)));

            tempEqv.emplace_back(eq);
        }
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Failed to Get Equipment for UserPk: " << userPk_ << std::endl;
        return tempEqv;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }

    return tempEqv;
}

std::vector<CONSUMABLES> RedisManager::GetUpdatedConsumables(uint16_t userPk_) {
    std::string csSlot = "consumables:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> consumables;

    std::vector<CONSUMABLES> tempCsv;

    try {
        redis->hgetall(csSlot, std::inserter(consumables, consumables.begin()));

        for (auto& [key, value] : consumables) {
            size_t div = value.find(':');
            if (div == std::string::npos) {
                std::cerr << "Invalid data format in Redis for position: " << key << std::endl;
                continue;
            }

            CONSUMABLES cs;
            cs.position = static_cast<uint16_t>(std::stoi(key));
            cs.itemCode = static_cast<uint16_t>(std::stoi(value.substr(0, div)));
            cs.count = static_cast<uint16_t>(std::stoi(value.substr(div + 1)));

            tempCsv.emplace_back(cs);
        }
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Failed to Get Consumables for UserPk: " << userPk_ << std::endl;
        return tempCsv;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }

    return tempCsv;
}

std::vector<MATERIALS> RedisManager::GetUpdatedMaterials(uint16_t userPk_) {
    std::string mtSlot = "materials:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> materials;

    redis->hgetall(mtSlot, std::inserter(materials, materials.begin()));

    std::vector<MATERIALS> tempMtv;

    for (auto& [key, value] : materials) {
        MATERIALS cs;

        if (value == "") { // Skip if the inventory slot is empty
            tempMtv.emplace_back(cs);
            continue;
        }

        size_t div = value.find(':');
        if (div == std::string::npos) {
            std::cerr << "Invalid data format in Redis for position : " << key << std::endl;
            continue;
        }

        try {
            cs.position = static_cast<uint16_t>(std::stoi(key));
            cs.itemCode = static_cast<uint16_t>(std::stoi(value.substr(0, div)));
            cs.count = static_cast<uint16_t>(std::stoi(value.substr(div + 1)));

            tempMtv.emplace_back(cs);
        }
        catch (const sw::redis::Error& e) {
            std::cerr << "Redis error: " << e.what() << std::endl;
            std::cout << "Failed to Get Materials for UserPk: " << userPk_ << std::endl;
            continue;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception error : " << e.what() << std::endl;
        }
    }

    return tempMtv;
}


// ======================================================= CENTER SERVER =======================================================

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

            auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

            connUsersManager->FindUser(connObjNum_)->SetPk(pk);
            tempUser->Set(pk, std::stoul(userData["exp"]),
                static_cast<uint16_t>(std::stoul(userData["level"])), std::stoul(userData["raidScore"]), (std::string)userConn->userId);

            redis->hset(key, "userstate", "online"); // Set user status to "online" in Redis Cluster
            tempUser->SetUserState(UserState::online); // Set user status to "online" in InGameUser object

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
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
}

void RedisManager::UserDisConnect(uint16_t connObjNum_) {
    auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);
    auto tempPk = tempUser->GetPk();

    try {
        redis->hset("userinfo:{" + std::to_string(tempPk) + "}", "userstate", "offline"); // Set user status to "offline" in Redis Cluster
        mySQLManager->LogoutSync(tempPk, GetUpdatedUserInfo(tempPk),
            GetUpdatedEquipment(tempPk), GetUpdatedConsumables(tempPk), GetUpdatedMaterials(tempPk));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;
        return;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
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

    try {
        redis->hset("userinfo:{" + std::to_string(inGameUserManager->GetInGameUserByObjNum(connObjNum_)->GetPk()) + "}", "userstate", "serverSwitching"); // Set user status to "serverSwitching" in Redis Cluster
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&serverUserCountsResPacket);
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;
        return;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }

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

    auto moveServerNum = MoveCHReqPacket->serverNum + CHANNEL_SERVER_START_NUMBER;

    moveCHResPacket.PacketId = (uint16_t)PACKET_ID::MOVE_SERVER_RESPONSE;
    moveCHResPacket.PacketLength = sizeof(MOVE_SERVER_RESPONSE);
    moveCHResPacket.port = ServerAddressMap[static_cast<ServerType>(moveServerNum)].port;
    strncpy_s(moveCHResPacket.ip, ServerAddressMap[static_cast<ServerType>(moveServerNum)].ip.c_str(), 256);

    if (!channelServersManager->EnterChannelServer(moveServerNum)) { // Failed to move to channel server 1
        moveCHResPacket.port = 0;
        connUsersManager->FindUser(connObjNum_)->
            PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
        return;
    };

    auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    try { // Generate JWT token
        std::string token = jwt::create()
            .set_issuer("Center_Server")
            .set_subject("Move_Server")
            .set_payload_claim("user_id", jwt::claim(tempUser->GetId()))
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        tag = "{" + std::to_string(moveServerNum) + "}";
        std::string key = "jwtcheck:" + tag;

        auto pipe = redis->pipeline(tag);
        pipe.hset(key, token, std::to_string(tempUser->GetPk()))
            .expire(key, 300);
        pipe.exec();

        strncpy_s(moveCHResPacket.serverToken, token.c_str(), MAX_JWT_TOKEN_LEN + 1);
        connUsersManager->FindUser(connObjNum_)-> // Send Channel Server address and JWT token to the user
            PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;

        // Send 0 when JWT token generation fails
        memset(moveCHResPacket.serverToken, 0, sizeof(moveCHResPacket.serverToken));
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
        return;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;

        // Send 0 when JWT token generation fails
        memset(moveCHResPacket.serverToken, 0, sizeof(moveCHResPacket.serverToken));
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
    }
}


// ======================================================= LOGIN SERVER =======================================================

void RedisManager::LoginServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imMatchingReqPacket = reinterpret_cast<LOGIN_SERVER_CONNECT_REQUEST*>(pPacket_);

    LOGIN_SERVER_CONNECT_RESPONSE imLRes;
    imLRes.PacketId = (uint16_t)PACKET_ID::LOGIN_SERVER_CONNECT_RESPONSE;
    imLRes.PacketLength = sizeof(LOGIN_SERVER_CONNECT_RESPONSE);
    imLRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);// Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(LOGIN_SERVER_CONNECT_RESPONSE), (char*)&imLRes);

    std::cout << "Login Server Authentication Successful" << std::endl;
}


// ======================================================= CHANNEL SERVER =======================================================

void RedisManager::ChannelServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imChReqPacket = reinterpret_cast<CHANNEL_SERVER_CONNECT_REQUEST*>(pPacket_);
    ServerAddressMap[static_cast<ServerType>(imChReqPacket->channelServerNum)].serverObjNum = connObjNum_;

    CHANNEL_SERVER_CONNECT_RESPONSE imChRes;
    imChRes.PacketId = (uint16_t)PACKET_ID::CHANNEL_SERVER_CONNECT_RESPONSE;
    imChRes.PacketLength = sizeof(CHANNEL_SERVER_CONNECT_RESPONSE);
    imChRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);// Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&imChRes);

    std::cout << "Channel Server" << imChReqPacket->channelServerNum << " Authentication Successful" << std::endl;
}

void RedisManager::SyncEqipmentEnhace(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto syncEquipReqPacket = reinterpret_cast<SYNC_EQUIPMENT_ENHANCE_REQUEST*>(pPacket_);

    mySQLManager->MySQLSyncEqipmentEnhace(syncEquipReqPacket->userPk, syncEquipReqPacket->itemPosition, syncEquipReqPacket->enhancement);
}


// ======================================================= MATCHING SERVER =======================================================

void RedisManager::MatchingServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imMatchingReqPacket = reinterpret_cast<MATCHING_SERVER_CONNECT_REQUEST*>(pPacket_);

    ServerAddressMap[ServerType::MatchingServer].serverObjNum = connObjNum_;

    MATCHING_SERVER_CONNECT_RESPONSE imMRes;
    imMRes.PacketId = (uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_RESPONSE;
    imMRes.PacketLength = sizeof(MATCHING_SERVER_CONNECT_RESPONSE);
    imMRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0); // Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATCHING_SERVER_CONNECT_RESPONSE), (char*)&imMRes);

    std::cout << "Matching Server Authentication Successful" << std::endl;
}

void RedisManager::MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_REQUEST_TO_MATCHING_SERVER matchReqPacket;
    matchReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_REQUEST_TO_MATCHING_SERVER;
    matchReqPacket.PacketLength = sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER);
    matchReqPacket.userPk = tempUser->GetPk();
    matchReqPacket.userCenterObjNum = connObjNum_;
    matchReqPacket.userGroupNum = tempUser->GetUserGroupNum();

    connUsersManager->FindUser(ServerAddressMap[ServerType::MatchingServer].serverObjNum)->PushSendMsg(sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER), (char*)&matchReqPacket);
}

void RedisManager::MatchStartResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(matchSuccessReqPacket->userCenterObjNum);

    RAID_MATCHING_RESPONSE matchResPacket;
    matchResPacket.PacketId = (uint16_t)PACKET_ID::RAID_MATCHING_RESPONSE;
    matchResPacket.PacketLength = sizeof(RAID_MATCHING_RESPONSE);

    if (matchSuccessReqPacket->isSuccess) {
        matchResPacket.insertSuccess = matchSuccessReqPacket->isSuccess;

        try {
            redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "raidMatching"); // Set user status to "raidMatching" in Redis Cluster

            tempUser->SetUserState(UserState::raidMatching);
            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResPacket);

            return;
        }
        catch (const sw::redis::Error& e) {
            matchResPacket.insertSuccess = false;
            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResPacket);

            std::cerr << "Redis error : " << e.what() << std::endl;
            return;
        }
    }
    else {
        matchResPacket.insertSuccess = matchSuccessReqPacket->isSuccess;
        connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResPacket);
        std::cout << tempUser->GetId() << " " << tempUser->GetUserGroupNum() << "group matching failed" << std::endl;
    }
}

void RedisManager::MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER matchCancelReqPacket;
    matchCancelReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER;
    matchCancelReqPacket.PacketLength = sizeof(MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER);
    matchCancelReqPacket.userCenterObjNum = connObjNum_;
    matchCancelReqPacket.userGroupNum = tempUser->GetLevel() / 3 + 1;

    connUsersManager->FindUser(ServerAddressMap[ServerType::MatchingServer].serverObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchCancelReqPacket);
}

void RedisManager::MatchingCancelResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchCancelResPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(matchCancelResPacket->userCenterObjNum);

    MATCHING_CANCEL_RESPONSE matchCanResPacket;
    matchCanResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE;
    matchCanResPacket.PacketLength = sizeof(MATCHING_CANCEL_RESPONSE);
    matchCanResPacket.isSuccess = matchCancelResPacket->isSuccess;

    try {
        redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "online"); // Set user status to "online" in Redis Cluster
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;
        return;
    }

    tempUser->SetUserState(UserState::online);

    connUsersManager->FindUser(matchCancelResPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchCanResPacket);

    std::cout << tempUser->GetId() << " Matching Cancel Success" << std::endl;
}

void RedisManager::CheckMatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_GAME_SERVER*>(pPacket_);

    auto raidServerNum = matchSuccessReqPacket->serverNum + GAME_SERVER_START_NUMBER;
    uint16_t tempRoomNum = matchSuccessReqPacket->roomNum;

    RAID_READY_REQUEST raidReadyReqPacket;
    raidReadyReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
    raidReadyReqPacket.PacketLength = sizeof(RAID_READY_REQUEST);
    raidReadyReqPacket.roomNum = tempRoomNum;
    raidReadyReqPacket.port = ServerAddressMap[static_cast<ServerType>(raidServerNum)].port;
    strncpy_s(raidReadyReqPacket.ip, ServerAddressMap[static_cast<ServerType>(raidServerNum)].ip.c_str(), 256);

    if (matchSuccessReqPacket->roomNum == 0) { // Send game creation failure message to matched users
        raidReadyReqPacket.roomNum = 0;
        connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
        return;
    }

    auto tempUser = inGameUserManager->GetInGameUserByObjNum(matchSuccessReqPacket->userCenterObjNum);

    { // Generate JWT token
        try {
            std::string tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::RaidGameServer01)) + "}";
            std::string key = "jwtcheck:" + tag;

            std::string token = jwt::create()
                .set_issuer("Center_Server")
                .set_subject("Connect_GameServer")
                .set_payload_claim("user_center_id", jwt::claim(std::to_string(matchSuccessReqPacket->userCenterObjNum))) // User unique ID used in the Center Server
                .set_payload_claim("room_id", jwt::claim(std::to_string(tempRoomNum))) // Matched room number
                .set_payload_claim("raid_id", jwt::claim(std::to_string(matchSuccessReqPacket->userRaidServerObjNum))) // User unique ID used in the Raid Server
                .set_expires_at(std::chrono::system_clock::now() +
                    std::chrono::seconds{ 300 })
                .sign(jwt::algorithm::hs256{ JWT_SECRET });

            auto pipe = redis->pipeline(tag);

            pipe.hset(key, token, std::to_string(tempUser->GetPk())) // Save the assigned Game Server unique ID to Redis
                .expire(key, 60);

            pipe.hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "inRaid"); // Set user status to "inRaid" in Redis Cluster

            pipe.exec();

            strncpy_s(raidReadyReqPacket.serverToken, token.c_str(), MAX_JWT_TOKEN_LEN + 1);

            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
            std::cout << "¸ÅÄª ¼º°ø pk : " << connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->GetPk() << std::endl;
        }
        catch (const sw::redis::Error& e) {
            raidReadyReqPacket.roomNum = 0;
            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

            redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "online"); // Set user status to "online" in Redis Cluster

            { // Send failed raid ready userObjNum to the game server
                RAID_READY_FAIL rrF;
                rrF.PacketId = (uint16_t)PACKET_ID::RAID_READY_FAIL;
                rrF.PacketLength = sizeof(RAID_READY_FAIL);
                rrF.userCenterObjNum = matchSuccessReqPacket->userCenterObjNum;
                rrF.roomNum = matchSuccessReqPacket->roomNum;

                connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
            }

            std::cerr << "Redis error : " << e.what() << std::endl;
            return;
        }
        catch (const std::exception& e) {
            raidReadyReqPacket.roomNum = 0;
            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

            redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "online"); // Set user status to "online" in Redis Cluster

            { // Send failed raid ready userObjNum to the game server
                RAID_READY_FAIL rrF;
                rrF.PacketId = (uint16_t)PACKET_ID::RAID_READY_FAIL;
                rrF.PacketLength = sizeof(RAID_READY_FAIL);
                rrF.userCenterObjNum = matchSuccessReqPacket->userCenterObjNum;
                rrF.roomNum = matchSuccessReqPacket->roomNum;

                connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
            }

            std::cerr << "Exception error : " << e.what() << std::endl;
            return;
        }
    }
}


// ======================================================= RAID GAME SERVER =======================================================

void RedisManager::GameServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imGameReqPacket = reinterpret_cast<RAID_SERVER_CONNECT_REQUEST*>(pPacket_);
    ServerAddressMap[static_cast<ServerType>(imGameReqPacket->gameServerNum)].serverObjNum = connObjNum_;

    RAID_SERVER_CONNECT_RESPONSE imGameRes;
    imGameRes.PacketId = (uint16_t)PACKET_ID::RAID_SERVER_CONNECT_RESPONSE;
    imGameRes.PacketLength = sizeof(RAID_SERVER_CONNECT_RESPONSE);
    imGameRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0); // Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_SERVER_CONNECT_RESPONSE), (char*)&imGameRes);

    std::cout << "Game Server" << imGameReqPacket->gameServerNum - GAME_SERVER_START_NUMBER << " Authentication Successful" << std::endl;
}

void RedisManager::SyncUserRaidScore(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<SYNC_HIGHSCORE_REQUEST*>(pPacket_);

    try {
        mySQLManager->MySQLSyncUserRaidScore(delEquipReqPacket->userPk, delEquipReqPacket->userScore, std::string(delEquipReqPacket->userId));
    }
    catch (const std::exception& e) {
        std::cout << "(MySQL Sync Fail) UserPK : " << delEquipReqPacket->userPk << ", Score: " << delEquipReqPacket->userScore << std::endl;
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
}