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

    // SESSION
    packetIDTable[(uint16_t)PACKET_ID::LOGIN_SERVER_CONNECT_REQUEST] = &RedisManager::LoginServerConnectRequest;

    // CHANNEL
    packetIDTable[(uint16_t)PACKET_ID::CHANNEL_SERVER_CONNECT_REQUEST] = &RedisManager::ChannelServerConnectRequest;
    packetIDTable[(uint16_t)PACKET_ID::USER_DISCONNECT_AT_CHANNEL_REQUEST] = &RedisManager::ChannelDisConnect;

    // MATCHING
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_REQUEST] = &RedisManager::MatchingServerConnectRequest;
    packetIDTable[(uint16_t)PACKET_ID::RAID_MATCHING_REQUEST] = &RedisManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchStartResponse;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST] = &RedisManager::MatchingCancel;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchingCancelResponse;


    // RAID
    packetIDTable[(uint16_t)PACKET_ID::RAID_SERVER_CONNECT_REQUEST] = &RedisManager::GameServerConnectRequest;
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

        redis = std::make_shared<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connected" << std::endl;

        mySQLManager = new MySQLManager;
        mySQLManager->init();

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

void RedisManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}


//  ---------------------------- SYNCRONIZATION  ----------------------------

USERINFO RedisManager::GetUpdatedUserInfo(uint16_t userPk_) {
    std::string userInfokey = "userinfo:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> userData;

    USERINFO tempUser;

    try {
        redis->hgetall(userInfokey, std::inserter(userData, userData.begin()));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Failed to Get userPk : " << userPk_ << " Infos" << std::endl;
        return tempUser;
    }

    tempUser.userId = userData["userId"];
    tempUser.raidScore = std::stoul(userData["raidScore"]);
    tempUser.exp = std::stoul(userData["exp"]);
    tempUser.level = static_cast<uint16_t>(std::stoi(userData["level"]));

    return tempUser;
}

std::vector<EQUIPMENT> RedisManager::GetUpdatedEquipment(uint16_t userPk_) {
    std::string eqSlot = "equipment:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> equipments;
    redis->hgetall(eqSlot, std::inserter(equipments, equipments.begin()));

    std::vector<EQUIPMENT> tempEqv;

    for (auto& [key, value] : equipments) {
        size_t div = value.find(':');
        if (div == std::string::npos) {
            std::cerr << "Invalid data format in Redis for position: " << key << std::endl;
            continue;
        }

        try {
            EQUIPMENT eq;
            eq.position = static_cast<uint16_t>(std::stoi(key));
            eq.itemCode = static_cast<uint16_t>(std::stoi(value.substr(0, div)));
            eq.enhance = static_cast<uint16_t>(std::stoi(value.substr(div + 1)));

            tempEqv.emplace_back(eq);
        }
        catch (...) {
            std::cerr << "(Equipment) Failed to parse position :" << key << ", code:enhance : " << value << std::endl;
            continue;
        }
    }

    return tempEqv;
}

std::vector<CONSUMABLES> RedisManager::GetUpdatedConsumables(uint16_t userPk_) {
    std::string csSlot = "consumables:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> consumables;
    redis->hgetall(csSlot, std::inserter(consumables, consumables.begin()));

    std::vector<CONSUMABLES> tempCsv;

    for (auto& [key, value] : consumables) {
        size_t div = value.find(':');
        if (div == std::string::npos) {
            std::cerr << "Invalid data format in Redis for position: " << key << std::endl;
            continue;
        }

        try {
            CONSUMABLES cs;
            cs.position = static_cast<uint16_t>(std::stoi(key));
            cs.itemCode = static_cast<uint16_t>(std::stoi(value.substr(0, div)));
            cs.count = static_cast<uint16_t>(std::stoi(value.substr(div + 1)));

            tempCsv.emplace_back(cs);
        }
        catch (...) {
            std::cerr << "(Consumable) Failed to parse position :" << key << ", code:enhance : " << value << std::endl;
            continue;
        }
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
        catch (...) {
            std::cerr << "(Material) Failed to parse position :" << key << ", code:enhance : " << value << std::endl;
            continue;
        }
    }

    return tempMtv;
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
    auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);
    auto tempPk = tempUser->GetPk();

    {  // Send User PK to the Session Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (uint16_t)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.userPk = tempPk;
        connUsersManager->FindUser(GatewayServerObjNum)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);

        mySQLManager->LogoutSync(tempPk, GetUpdatedUserInfo(tempPk), GetUpdatedEquipment(tempPk), GetUpdatedConsumables(tempPk), GetUpdatedMaterials(tempPk));
        redis->hset("userinfo:{" + std::to_string(tempPk) + "}", "userstate", "offline"); // Change user status to "offline"
    }
}

void RedisManager::UserDisConnect(uint16_t connObjNum_) { // Abnormal Disconnect
    auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);
    auto tempPk = tempUser->GetPk();

    {  // Send User PK to the Session Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (uint16_t)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.userPk = tempPk;
        connUsersManager->FindUser(GatewayServerObjNum)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);

        mySQLManager->LogoutSync(tempPk, GetUpdatedUserInfo(tempPk), GetUpdatedEquipment(tempPk), GetUpdatedConsumables(tempPk), GetUpdatedMaterials(tempPk));
        redis->hset("userinfo:{" + std::to_string(tempPk) + "}", "userstate", "offline"); // Change user status to "offline"
    }
}

void RedisManager::LoginServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imMatchingReqPacket = reinterpret_cast<LOGIN_SERVER_CONNECT_REQUEST*>(pPacket_);
    MatchingServerObjNum = connObjNum_;  // Initialize Login Server unique ID

    LOGIN_SERVER_CONNECT_RESPONSE imLRes;
    imLRes.PacketId = (uint16_t)PACKET_ID::LOGIN_SERVER_CONNECT_RESPONSE;
    imLRes.PacketLength = sizeof(LOGIN_SERVER_CONNECT_RESPONSE);
    imLRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(LOGIN_SERVER_CONNECT_RESPONSE), (char*)&imLRes);
    std::cout << "Login Server Authentication Successful" << std::endl;
}

void RedisManager::ChannelServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imChReqPacket = reinterpret_cast<CHANNEL_SERVER_CONNECT_REQUEST*>(pPacket_);
    channelServerObjNums[imChReqPacket->channelServerNum] = connObjNum_; // Initialize Channel Server's unique ID

    CHANNEL_SERVER_CONNECT_RESPONSE imChRes;
    imChRes.PacketId = (uint16_t)PACKET_ID::CHANNEL_SERVER_CONNECT_RESPONSE;
    imChRes.PacketLength = sizeof(CHANNEL_SERVER_CONNECT_RESPONSE);
    imChRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&imChRes);
    std::cout << "Channel Server" << imChReqPacket->channelServerNum << " Authentication Successful" << std::endl;
}

void RedisManager::MatchingServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imMatchingReqPacket = reinterpret_cast<MATCHING_SERVER_CONNECT_REQUEST*>(pPacket_);
    MatchingServerObjNum = connObjNum_;  // Initialize Matching Server unique ID

    MATCHING_SERVER_CONNECT_RESPONSE imMRes;
    imMRes.PacketId = (uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_RESPONSE;
    imMRes.PacketLength = sizeof(MATCHING_SERVER_CONNECT_RESPONSE);
    imMRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATCHING_SERVER_CONNECT_RESPONSE), (char*)&imMRes);
    std::cout << "Matching Server Authentication Successful" << std::endl;
}

void RedisManager::GameServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imGameReqPacket = reinterpret_cast<RAID_SERVER_CONNECT_REQUEST*>(pPacket_);
    channelServerObjNums[imGameReqPacket->gameServerNum] = connObjNum_; // Initialize Game Server's unique ID

    RAID_SERVER_CONNECT_RESPONSE imGameRes;
    imGameRes.PacketId = (uint16_t)PACKET_ID::RAID_SERVER_CONNECT_RESPONSE;
    imGameRes.PacketLength = sizeof(RAID_SERVER_CONNECT_RESPONSE);
    imGameRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_SERVER_CONNECT_RESPONSE), (char*)&imGameRes);
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
    matchReqPacket.userGroupNum = tempUser->GetLevel() / 3 + 1;

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
        std::cout << tempUser->GetId() << " " << tempUser->GetLevel() / 3 + 1 << "group matching successful" << std::endl;
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

void RedisManager::SyncUserRaidScore(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<SYNC_HIGHSCORE_REQUEST*>(pPacket_);

    mySQLManager->SyncUserRaidScore(delEquipReqPacket->userPk, delEquipReqPacket->userScore, std::string(delEquipReqPacket->userId));
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