#include "PacketManager.h"

thread_local std::mt19937 PacketManager::gen(std::random_device{}());

void PacketManager::init(const uint16_t packetThreadCnt_) {

    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(UINT16)PACKET_ID::IM_GAME_RESPONSE] = &PacketManager::ImGameResponse;
    packetIDTable[(UINT16)PACKET_ID::MATCHING_SERVER_CONNECT_RESPONSE] = &PacketManager::ImGameResponsefromMatchingServer;

    packetIDTable[(UINT16)PACKET_ID::MATCHING_REQUEST_TO_GAME_SERVER] = &PacketManager::MakeRoom;

    packetIDTable[(UINT16)PACKET_ID::USER_CONNECT_GAME_REQUEST] = &PacketManager::UserConnect;

    packetIDTable[(UINT16)PACKET_ID::RAID_TEAMINFO_REQUEST] = &PacketManager::RaidTeamInfo;
    packetIDTable[(UINT16)PACKET_ID::RAID_HIT_REQUEST] = &PacketManager::RaidHit;

    PacketRun(packetThreadCnt_);
}

void PacketManager::PacketRun(const uint16_t packetThreadCnt_) { // Connect Redis Server
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

void PacketManager::Disconnect(uint16_t connObjNum_) {
    UserDisConnect(connObjNum_);
}

void PacketManager::SetManager(ConnUsersManager* connUsersManager_, RoomManager* roomManager_) {
    connUsersManager = connUsersManager_;
    roomManager = roomManager_;
}

bool PacketManager::CreatePacketThread(const uint16_t packetThreadCnt_) {
    packetRun = true;

    for (int i = 0; i < packetThreadCnt_; i++) {
        packetThreads.emplace_back(std::thread([this]() {PacketThread(); }));
    }
    return true;
}

void PacketManager::PacketThread() {
    DataPacket tempD(0, 0);
    ConnUser* TempConnUser = nullptr;
    char tempData[1024] = { 0 };

    while (packetRun) {
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

void PacketManager::PushPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}

// ============================== PACKET ==============================

//  ---------------------------- SYSTEM  ----------------------------

void PacketManager::ImGameResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<IM_GAME_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Failed to Authenticate with Center Server" << std::endl;
        return;
    }

    std::cout << "Successfully Authenticated with Center Server" << std::endl;
}

void PacketManager::ImGameResponsefromMatchingServer(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchConn = reinterpret_cast<IM_GAME_RESPONSE*>(pPacket_);

    if (!matchConn->isSuccess) {
        std::cout << "Failed to Authenticate with Matching Server" << std::endl;
        return;
    }

    std::cout << "Successfully Authenticated with Matcing Server" << std::endl;
}

void PacketManager::UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_GAME_REQUEST*>(pPacket_);
    std::string key = "jwtcheck:{" + std::to_string(static_cast<uint16_t>(ServerType::RaidGameServer01)) + "}";

    USER_CONNECT_GAME_RESPONSE ucRes;
    ucRes.PacketId = (uint16_t)PACKET_ID::USER_CONNECT_GAME_RESPONSE;
    ucRes.PacketLength = sizeof(USER_CONNECT_GAME_RESPONSE);

    try { // JWT token check
        auto userRaidServerObjNum = static_cast<uint32_t>(std::stoul(*redis->hget(key, (std::string)userConn->userToken)));

        if (userRaidServerObjNum) {
            auto tempId = jwt::decode((std::string)userConn->userToken).get_payload_claim("user_id");

            std::string user_id = tempId.as_string();

            if (user_id != (std::string)userConn->userId) { // ID Check Fail
                ucRes.isSuccess = false;
                connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
                std::cout << (std::string)userConn->userId << " ID Check Fail" << std::endl;
            }

            auto tempRoomId = jwt::decode((std::string)userConn->userToken).get_payload_claim("room_id");
            auto tempRaidId = jwt::decode((std::string)userConn->userToken).get_payload_claim("raid_id");

            uint16_t room_id = static_cast<uint16_t>(std::stoi(tempRoomId.as_string()));
            uint16_t raid_id = static_cast<uint16_t>(std::stoi(tempRaidId.as_string()));

            roomManager->GetRoom(room_id)->SetUserConnObjNum(raid_id, connObjNum_);
            connUsersManager->FindUser(connObjNum_)->SetUserRoomInfo(room_id, raid_id);

            ucRes.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
            std::cout << (std::string)userConn->userId << " Connection Success" << std::endl;
        }
        else {
            ucRes.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
            std::cout << (std::string)userConn->userId << " JWT Check Fail" << std::endl;
        }
    }
    catch (const sw::redis::Error& e) {
        ucRes.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
        std::cerr << "Redis error: " << e.what() << std::endl;
        return;
    }
}

void PacketManager::UserDisConnect(uint16_t connObjNum_) {

}

void PacketManager::MakeRoom(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchReqPacket = reinterpret_cast<MATCHING_REQUEST_TO_GAME_SERVER*>(pPacket_);

    RaidUserInfo* user1 = new RaidUserInfo;
    RaidUserInfo* user2 = new RaidUserInfo;;

    user1->userRaidServerObjNum = 1;
    user1->userRaidServerObjNum = 2;

    user1->userPk = matchReqPacket->userPk1;
    user2->userPk = matchReqPacket->userPk2;

    std::vector<std::string> fields = { "id", "level", "raidScore"};
    std::vector<sw::redis::OptionalString> values;

    { // Get matched user info from Redis Cluster
        std::string tag1 = "{" + std::to_string(matchReqPacket->userPk1) + "}";
        std::string key1 = "userinfo:" + tag1;

        redis->hmget(key1, fields.begin(), fields.end(), std::back_inserter(values));

        if (values[0] && values[1] && values[2]) {
            user1->userId = *values[0];
            user1->userLevel = static_cast<uint16_t>(std::stoul(*values[1]));
            user1->userMaxScore = std::stoul(*values[2]);
        }
    }

    values.clear();

    { // Get matched user info from Redis Cluster
        std::string tag2 = "{" + std::to_string(matchReqPacket->userPk2) + "}";
        std::string key2 = "userinfo:" + tag2;

        redis->hmget(key2, fields.begin(), fields.end(), std::back_inserter(values));

        if (values[0] && values[1] && values[2]) {
            user2->userId = *values[0];
            user2->userLevel = static_cast<uint16_t>(std::stoul(*values[1]));
            user2->userMaxScore = std::stoul(*values[2]);
        }
    }

    std::discrete_distribution<int> dist(mapProbabilities.begin(), mapProbabilities.end()); // Randomly select map by probability

    MATCHING_RESPONSE_FROM_GAME_SERVER matchResPacket;
    matchResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_GAME_SERVER;
    matchResPacket.PacketLength = sizeof(MATCHING_RESPONSE_FROM_GAME_SERVER);

    if (!roomManager->MakeRoom(matchReqPacket->roomNum, dist(gen), 10, 30, user1, user2)) { // Room creation failed
        matchResPacket.roomNum = 0;
        connUsersManager->FindUser(centerServerObjNum)->PushSendMsg(sizeof(MATCHING_RESPONSE_FROM_GAME_SERVER), (char*)&matchResPacket);
        return;
    }

    matchResPacket.userCenterObjNum1 = matchReqPacket->userCenterObjNum1;
    matchResPacket.userCenterObjNum2 = matchReqPacket->userCenterObjNum2;
    matchResPacket.roomNum = matchReqPacket->roomNum;

    connUsersManager->FindUser(centerServerObjNum)->PushSendMsg(sizeof(MATCHING_RESPONSE_FROM_GAME_SERVER), (char*)&matchResPacket);
}

void PacketManager::RaidTeamInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto raidTeamInfoReqPacket = reinterpret_cast<RAID_TEAMINFO_REQUEST*>(pPacket_);
    auto connUser = connUsersManager->FindUser(connObjNum_);
    
    Room* tempRoom = roomManager->GetRoom(connUser->GetRoomNum());
    tempRoom->SetSockAddr(connUser->GetUserRaidServerObjNum(), raidTeamInfoReqPacket->userAddr); // Set User UDP Socket Info

    auto teamInfo = tempRoom->GetTeamInfo(connUser->GetUserRaidServerObjNum());

    RAID_TEAMINFO_RESPONSE raidTeamInfoResPacket;
    raidTeamInfoResPacket.PacketId = (uint16_t)PACKET_ID::RAID_TEAMINFO_RESPONSE;
    raidTeamInfoResPacket.PacketLength = sizeof(RAID_TEAMINFO_RESPONSE);
    raidTeamInfoResPacket.teamLevel = teamInfo->userLevel;
    strncpy_s(raidTeamInfoResPacket.teamId, teamInfo->userId.c_str(), MAX_USER_ID_LEN);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_TEAMINFO_RESPONSE), (char*)&raidTeamInfoResPacket);

    if (tempRoom->StartCheck()) { // Send game start ready message to matched users
        RAID_START raidStartReqPacket1;
        raidStartReqPacket1.PacketId = (uint16_t)PACKET_ID::RAID_START;
        raidStartReqPacket1.PacketLength = sizeof(RAID_START);
        raidStartReqPacket1.endTime = tempRoom->SetEndTime();
        raidStartReqPacket1.mapNum = tempRoom->GetMapNum();
        raidStartReqPacket1.mobHp = tempRoom->GetMobHp();

        RAID_START raidStartReqPacket2;
        raidStartReqPacket2.PacketId = (uint16_t)PACKET_ID::RAID_START;
        raidStartReqPacket2.PacketLength = sizeof(RAID_START);
        raidStartReqPacket2.endTime = tempRoom->SetEndTime();
        raidStartReqPacket1.mapNum = tempRoom->GetMapNum();
        raidStartReqPacket1.mobHp = tempRoom->GetMobHp();

        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_START), (char*)&raidStartReqPacket1);
        connUsersManager->FindUser(teamInfo->userConnObjNum)->PushSendMsg(sizeof(RAID_START), (char*)&raidStartReqPacket2);
    }
}

void PacketManager::RaidHit(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
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

            try {
                auto pipe = redis->pipeline("ranking");
                for (int i = 1; i < tempRoom->GetRoomUserCnt(); i++) { // Send raid end message to all users
                    auto tempUser = tempRoom->GetMyInfo(i);

                    RAID_END_REQUEST raidEndReqPacket;
                    raidEndReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST;
                    raidEndReqPacket.PacketLength = sizeof(RAID_END_REQUEST);
                    raidEndReqPacket.userScore = tempRoom->GetMyScore(i);
                    raidEndReqPacket.teamScore = tempRoom->GetTeamScore(i);
                    connUsersManager->FindUser(tempUser->userConnObjNum)->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket);

                    if (tempUser->userScore.load() > tempUser->userMaxScore) { // Update Redis if new score exceeds previous best score
                        pipe.zadd("ranking", tempUser->userId, (double)(tempUser->userScore.load()));
                    }
                }

                pipe.exec(); // Synchronize user rankings
                roomManager->DeleteMob(tempRoom);
            }
            catch (const sw::redis::Error& e) {
                std::cerr << "Redis error: " << e.what() << std::endl;

                for (int i = 1; i < tempRoom->GetRoomUserCnt(); i++) {
                    auto tempUser = tempRoom->GetMyInfo(i);
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
        }
    }
    else {
        raidHitResPacket.currentMobHp = hit.first;
        raidHitResPacket.yourScore = hit.second;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);
    }
}