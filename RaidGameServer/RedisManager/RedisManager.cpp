#include "RedisManager.h"

// ========================== INITIALIZATION ===========================

thread_local std::mt19937 RedisManager::gen(std::random_device{}());

void RedisManager::init(const uint16_t packetThreadCnt_) {

    // -------------------- SET PACKET HANDLERS ----------------------
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(UINT16)PACKET_ID::RAID_SERVER_CONNECT_RESPONSE] = &RedisManager::CenterServerConnectResponse;
    packetIDTable[(UINT16)PACKET_ID::MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER] = &RedisManager::MatchingServerConnectResponse;

    packetIDTable[(UINT16)PACKET_ID::MATCHING_REQUEST_TO_GAME_SERVER] = &RedisManager::MakeRoom;

    packetIDTable[(UINT16)PACKET_ID::USER_CONNECT_GAME_REQUEST] = &RedisManager::UserConnect;

    packetIDTable[(UINT16)PACKET_ID::RAID_TEAMINFO_REQUEST] = &RedisManager::RaidTeamInfo;
    packetIDTable[(UINT16)PACKET_ID::RAID_HIT_REQUEST] = &RedisManager::RaidHit;

    RedisRun(packetThreadCnt_);
}

void RedisManager::SetManager(ConnUsersManager* connUsersManager_, RoomManager* roomManager_) {
    connUsersManager = connUsersManager_;
    roomManager = roomManager_;
}


// ========================= PACKET MANAGEMENT ========================

void RedisManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}


// ======================== CONNECTION INTERFACE ======================

void RedisManager::Disconnect(uint16_t connObjNum_) {

}


// ========================= REDIS MANAGEMENT =========================

void RedisManager::RedisRun(const uint16_t packetThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        redis = std::make_unique<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connected" << std::endl;

        CreatePacketThread(packetThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis Connect Error : " << err.what() << std::endl;
    }
}

bool RedisManager::CreatePacketThread(const uint16_t packetThreadCnt_) {
    redisRun = true;

    try {
        redisThreads.emplace_back(std::thread([this]() { RedisThread(); }));
    }
    catch (const std::system_error& e) {
        std::cerr << "Create Packet Thread Failed : " << e.what() << std::endl;
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


// ======================================================= CENTER SERVER =======================================================

void RedisManager::CenterServerConnectResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<RAID_SERVER_CONNECT_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Failed to Authenticate with Center Server" << std::endl;
        return;
    }

    std::cout << "Successfully Authenticated with Center Server" << std::endl;
}


// ======================================================= MATCHING SERVER =======================================================

void RedisManager::MatchingServerConnectResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchConn = reinterpret_cast<MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER*>(pPacket_);

    if (!matchConn->isSuccess) {
        std::cout << "Failed to Authenticate with Matching Server" << std::endl;
        return;
    }

    std::cout << "Successfully Authenticated with Matcing Server" << std::endl;
}


// ======================================================= RAID GAME SERVER =======================================================

void RedisManager::MakeRoom(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchReqPacket = reinterpret_cast<MATCHING_REQUEST_TO_GAME_SERVER*>(pPacket_);

    auto tempRoomNum = matchReqPacket->roomNum;
    auto tempUserPk = matchReqPacket->userPk;

    roomManager->MakeRoom(tempRoomNum);
    auto tempRoom = roomManager->GetRoom(tempRoomNum);

    RaidUserInfo* tempUser = new RaidUserInfo;
    tempUser->userCenterObjNum = matchReqPacket->userCenterObjNum;

    auto tempRaidNum = tempRoom->UserSetCheck(tempUser);

    try { // Get matched user info from Redis Cluster
        std::vector<std::string> fields = { "userId", "level", "raidScore" };
        std::vector<sw::redis::OptionalString> values;

        std::string tag = "{" + std::to_string(tempUserPk) + "}";
        std::string key = "userinfo:" + tag;

        redis->hmget(key, fields.begin(), fields.end(), std::back_inserter(values));

        if (values.size() != 3 || !values[0] || !values[1] || !values[2]) {
            std::cerr << "[Redis Error] Missing fields in userinfo for PK: " << tempUserPk << std::endl;
            return;
        }

        tempUser->userId = *values[0];
        tempUser->userLevel = static_cast<uint16_t>(std::stoul(*values[1]));
        tempUser->userMaxScore = std::stoul(*values[2]);
    }

    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Failed to Get Matched UserInfo" << std::endl;
    }

    if (tempRaidNum == MAX_RAID_ROOM_PLAYERS) { // When all users have been set in the room
        // Randomly select map by probability
        std::discrete_distribution<int> dist(mapProbabilities.begin(), mapProbabilities.end());

        tempRoom->Set(dist(gen), 30);

        MATCHING_RESPONSE_FROM_GAME_SERVER matchResPacket;
        matchResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_GAME_SERVER;
        matchResPacket.PacketLength = sizeof(MATCHING_RESPONSE_FROM_GAME_SERVER);
        matchResPacket.serverNum = GAME_SERVER_NUM;

        // Send raid room setup completion message to the center server to notify matched users that the raid is ready
        for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) {
            matchResPacket.userCenterObjNum = tempRoom->GetUserInfoByObjNum(i)->userCenterObjNum;
            matchResPacket.roomNum = matchReqPacket->roomNum;
            matchResPacket.userRaidServerObjNum = i;

            connUsersManager->FindUser(static_cast<uint16_t>(ServerType::CenterServer))->
                PushSendMsg(sizeof(MATCHING_RESPONSE_FROM_GAME_SERVER), (char*)&matchResPacket);
        }
    }
}

void RedisManager::UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_GAME_REQUEST*>(pPacket_);
    std::string key = "jwtcheck:{" + std::to_string(static_cast<uint16_t>(ServerType::RaidGameServer01)) + "}";

    USER_CONNECT_GAME_RESPONSE ucRes;
    ucRes.PacketId = (uint16_t)PACKET_ID::USER_CONNECT_GAME_RESPONSE;
    ucRes.PacketLength = sizeof(USER_CONNECT_GAME_RESPONSE);

    try { // JWT token check
        auto pk = static_cast<uint32_t>(std::stoul(*redis->hget(key, (std::string)userConn->userToken)));

        if (pk) {
            auto tempUserCenterId = jwt::decode((std::string)userConn->userToken).get_payload_claim("user_center_id");
            auto tempRoomId = jwt::decode((std::string)userConn->userToken).get_payload_claim("room_id");
            auto tempRaidId = jwt::decode((std::string)userConn->userToken).get_payload_claim("raid_id");

            uint16_t room_id = static_cast<uint16_t>(std::stoi(tempRoomId.as_string()));
            uint16_t raid_id = static_cast<uint16_t>(std::stoi(tempRaidId.as_string()));

            roomManager->GetRoom(room_id)->SetUserConnObjNum(raid_id, connObjNum_);
            connUsersManager->FindUser(connObjNum_)->SetUserRoomInfo(room_id, raid_id);

            ucRes.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);

            std::cout << (std::string)userConn->userId << " Authentication Successful" << std::endl;
            return;
        }
        else {
            ucRes.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
            std::cout << (std::string)userConn->userId << " Authentication Failed" << std::endl;
            return;
        }
    }
    catch (const sw::redis::Error& e) {
        ucRes.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
        std::cerr << "Redis error: " << e.what() << std::endl;
        return;
    }
}

void RedisManager::RaidTeamInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto raidTeamInfoReqPacket = reinterpret_cast<RAID_TEAMINFO_REQUEST*>(pPacket_);

    auto connUser = connUsersManager->FindUser(connObjNum_);
    auto myRaidNum = connUser->GetUserRaidServerObjNum();

    Room* tempRoom = roomManager->GetRoom(connUser->GetRoomNum());
    tempRoom->SetSockAddr(myRaidNum, raidTeamInfoReqPacket->userAddr); // Set User UDP Socket Info

    for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) {
        auto teamInfo = tempRoom->GetUserInfoByObjNum(i);

        RAID_TEAMINFO_RESPONSE raidTeamInfoResPacket;
        raidTeamInfoResPacket.PacketId = (uint16_t)PACKET_ID::RAID_TEAMINFO_RESPONSE;
        raidTeamInfoResPacket.PacketLength = sizeof(RAID_TEAMINFO_RESPONSE);
        raidTeamInfoResPacket.teamLevel = teamInfo->userLevel;
        raidTeamInfoResPacket.userRaidServerObjNum = i;
        strncpy_s(raidTeamInfoResPacket.teamId, teamInfo->userId.c_str(), MAX_USER_ID_LEN);

        connUsersManager->FindUser(connObjNum_)->
            PushSendMsg(sizeof(RAID_TEAMINFO_RESPONSE), (char*)&raidTeamInfoResPacket);
    }

    if (tempRoom->SendUserCheck()) {
        for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) {

            RAID_START raidStartReqPacket1;
            raidStartReqPacket1.PacketId = (uint16_t)PACKET_ID::RAID_START;
            raidStartReqPacket1.PacketLength = sizeof(RAID_START);
            raidStartReqPacket1.endTime = tempRoom->SetEndTime();
            raidStartReqPacket1.mapNum = tempRoom->GetMapNum();
            raidStartReqPacket1.mobHp = tempRoom->GetMobHp();

            connUsersManager->FindUser(tempRoom->GetUserInfoByObjNum(i)->userConnObjNum)->
                PushSendMsg(sizeof(RAID_START), (char*)&raidStartReqPacket1);
        }
        tempRoom->SetGameRunning(true); // Set game start flag
        roomManager->InsertEndCheckSet(tempRoom);
    }
}

void RedisManager::RaidHit(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto raidHitReqPacket = reinterpret_cast<RAID_HIT_REQUEST*>(pPacket_);
    auto connUser = connUsersManager->FindUser(connObjNum_);
    auto tempRoom = roomManager->GetRoom(connUser->GetRoomNum());

    RAID_HIT_RESPONSE raidHitResPacket;
    raidHitResPacket.PacketId = (uint16_t)PACKET_ID::RAID_HIT_RESPONSE;
    raidHitResPacket.PacketLength = sizeof(RAID_HIT_RESPONSE);

    auto hit = tempRoom->Hit(connUser->GetUserRaidServerObjNum(), raidHitReqPacket->damage);

    if (hit.first <= 0) { // Mob defeated
        if (tempRoom->EndCheck()) {
            raidHitResPacket.currentMobHp = 0;
            raidHitResPacket.yourScore = hit.second;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);

            for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) { // Send raid end message to all users
                auto tempUser = tempRoom->GetUserInfoByObjNum(i);

                RAID_END raidEndReqPacket;
                raidEndReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_END;
                raidEndReqPacket.PacketLength = sizeof(RAID_END);
                connUsersManager->FindUser(tempUser->userConnObjNum)->PushSendMsg(sizeof(RAID_END), (char*)&raidEndReqPacket);
            }

            try {
                auto pipe = redis->pipeline("ranking");

                for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) { // Send scores of users who participated in the raid
                    auto tempUser1 = tempRoom->GetUserInfoByObjNum(i);

                    for (int j = 1; j <= MAX_RAID_ROOM_PLAYERS; j++) {
                        auto tempUser2 = tempRoom->GetUserInfoByObjNum(j);

                        SEND_RAID_SCORE sendScoreReqPacket;
                        sendScoreReqPacket.PacketId = (uint16_t)PACKET_ID::SEND_RAID_SCORE;
                        sendScoreReqPacket.PacketLength = sizeof(SEND_RAID_SCORE);
                        sendScoreReqPacket.userRaidServerObjNum = j;
                        sendScoreReqPacket.userScore = tempUser2->userScore.load();

                        connUsersManager->FindUser(tempUser1->userConnObjNum)->PushSendMsg(sizeof(SEND_RAID_SCORE), (char*)&sendScoreReqPacket);
                    }

                    if (tempUser1->userScore.load() > tempUser1->userMaxScore) { // Update Redis if new score exceeds previous best score
                        pipe.zadd("ranking", tempUser1->userId, (double)(tempUser1->userScore.load()));

                        { // Send sync message to Center Server if new score exceeds previous best score
                            SYNC_HIGHSCORE_REQUEST shreq;
                            shreq.PacketId = (uint16_t)PACKET_ID::SYNC_HIGHSCORE_REQUEST;
                            shreq.PacketLength = sizeof(SYNC_HIGHSCORE_REQUEST);
                            shreq.userScore = tempUser1->userScore.load();
                            strncpy_s(shreq.userId, tempUser1->userId.c_str(), MAX_USER_ID_LEN);

                            connUsersManager->FindUser(static_cast<uint16_t>(ServerType::CenterServer))->PushSendMsg(sizeof(SYNC_HIGHSCORE_REQUEST), (char*)&shreq);
                        }
                    }
                }

                pipe.exec(); // Synchronize user rankings

                RAID_END_REQUEST_TO_MATCHING_SERVER raidEndReq;
                raidEndReq.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST_TO_MATCHING_SERVER;
                raidEndReq.PacketLength = sizeof(RAID_END_REQUEST_TO_MATCHING_SERVER);
                raidEndReq.gameServerNum = GAME_SERVER_NUM;
                raidEndReq.roomNum = tempRoom->GetRoomNum();

                connUsersManager->FindUser(static_cast<uint16_t>(ServerType::MatchingServer))->PushSendMsg(sizeof(RAID_END_REQUEST_TO_MATCHING_SERVER), (char*)&raidEndReq);

                tempRoom->SetGameRunning(false); // Set game end flag
                roomManager->DeleteMob(tempRoom);
                return;
            }
            catch (const sw::redis::Error& e) {
                std::cerr << "Redis error: " << e.what() << std::endl;

                for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) {
                    auto tempUser = tempRoom->GetUserInfoByObjNum(i);
                    std::cout << tempUser->userId << " Score : " << tempUser->userScore << std::endl;
                }

                std::cout << "Fail to Synchronize" << std::endl;
                roomManager->DeleteMob(tempRoom);
                return;
            }
        }
        else { // if get 0, waitting End message
            raidHitResPacket.currentMobHp = 0;
            raidHitResPacket.yourScore = hit.second;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);
            return;
        }
    }
    else {
        raidHitResPacket.currentMobHp = hit.first;
        raidHitResPacket.yourScore = hit.second;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);
        return;
    }
}