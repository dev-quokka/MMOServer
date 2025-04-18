#include "PacketManager.h"

thread_local std::mt19937 PacketManager::gen(std::random_device{}());

void PacketManager::init(const uint16_t packetThreadCnt_) {

    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(UINT16)PACKET_ID::RAID_SERVER_CONNECT_RESPONSE] = &PacketManager::CenterServerConnectResponse;
    packetIDTable[(UINT16)PACKET_ID::MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER] = &PacketManager::MatchingServerConnectResponse;

    packetIDTable[(UINT16)PACKET_ID::MATCHING_REQUEST_TO_GAME_SERVER] = &PacketManager::MakeRoom;

    packetIDTable[(UINT16)PACKET_ID::USER_CONNECT_GAME_REQUEST] = &PacketManager::UserConnect;

    packetIDTable[(UINT16)PACKET_ID::RAID_TEAMINFO_REQUEST] = &PacketManager::RaidTeamInfo;
    packetIDTable[(UINT16)PACKET_ID::RAID_HIT_REQUEST] = &PacketManager::RaidHit;

    PacketRun(packetThreadCnt_);
}

void PacketManager::SetManager(ConnUsersManager* connUsersManager_, RoomManager* roomManager_) {
    connUsersManager = connUsersManager_;
    roomManager = roomManager_;
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

}

bool PacketManager::CreatePacketThread(const uint16_t packetThreadCnt_) {
    packetRun = true;

    try {
        packetThreads.emplace_back(std::thread([this]() { PacketThread(); }));
    }
    catch (const std::system_error& e) {
        std::cerr << "Create Packet Thread Failed : " << e.what() << std::endl;
        return false;
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

void PacketManager::CenterServerConnectResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<RAID_SERVER_CONNECT_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Failed to Authenticate with Center Server" << std::endl;
        return;
    }

    std::cout << "Successfully Authenticated with Center Server" << std::endl;
}

void PacketManager::MatchingServerConnectResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchConn = reinterpret_cast<MATCHING_SERVER_CONNECT_RESPONSE_TO_RAID_SERVER*>(pPacket_);

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
            std::cout << (std::string)userConn->userId << " Authentication Successful" << std::endl;
        }
        else {
            ucRes.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
            std::cout << (std::string)userConn->userId << " Authentication Failed" << std::endl;
        }
    }
    catch (const sw::redis::Error& e) {
        ucRes.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
        std::cerr << "Redis error: " << e.what() << std::endl;
        return;
    }
}

void PacketManager::MakeRoom(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchReqPacket = reinterpret_cast<MATCHING_REQUEST_TO_GAME_SERVER*>(pPacket_);

    auto tempRoomNum = matchReqPacket->roomNum;
    auto tempUserPk = matchReqPacket->userPk;

    roomManager->MakeRoom(tempRoomNum, 30);

    auto tempRoom = roomManager->GetRoom(tempRoomNum);

    RaidUserInfo* tempUser = new RaidUserInfo;
    tempUser->userPk = tempUserPk;
    tempUser->userCenterObjNum = matchReqPacket->userCenterObjNum;
    tempRoom->UserSetCheck(tempUser);

    std::vector<std::string> fields = { "id", "level", "raidScore"};
    std::vector<sw::redis::OptionalString> values;

    try { // Get matched user info from Redis Cluster
        std::string tag = "{" + std::to_string(tempUserPk) + "}";
        std::string key = "userinfo:" + tag;

        redis->hmget(key, fields.begin(), fields.end(), std::back_inserter(values));

        tempUser->userId = *values[0];
        tempUser->userLevel = static_cast<uint16_t>(std::stoul(*values[1]));
        tempUser->userMaxScore = std::stoul(*values[2]);
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Failed to Get Matched UserInfo" << std::endl;
    }

    if (tempUser->userRaidServerObjNum == MAX_RAID_ROOM_PLAYERS) { // When all users have been set in the room
        std::discrete_distribution<int> dist(mapProbabilities.begin(), mapProbabilities.end()); // Randomly select map by probability

        tempRoom->SetMapNum(dist(gen));

        MATCHING_RESPONSE_FROM_GAME_SERVER matchResPacket;
        matchResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_GAME_SERVER;
        matchResPacket.PacketLength = sizeof(MATCHING_RESPONSE_FROM_GAME_SERVER);
        matchResPacket.serverNum = GAME_SERVER_NUM;

        for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) { // Send raid room setup completion message to the center server to notify matched users that the raid is ready
            matchResPacket.userCenterObjNum = tempRoom->GetUserInfoByObjNum(i)->userCenterObjNum;
            matchResPacket.roomNum = matchReqPacket->roomNum;

            connUsersManager->FindUser(centerServerObjNum)->PushSendMsg(sizeof(MATCHING_RESPONSE_FROM_GAME_SERVER), (char*)&matchResPacket);
        }
    }
}

void PacketManager::RaidTeamInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto raidTeamInfoReqPacket = reinterpret_cast<RAID_TEAMINFO_REQUEST*>(pPacket_);
    auto connUser = connUsersManager->FindUser(connObjNum_);
    auto myRaidNum = connUser->GetUserRaidServerObjNum();

    Room* tempRoom = roomManager->GetRoom(connUser->GetRoomNum());
    tempRoom->SetSockAddr(connUser->GetUserRaidServerObjNum(), raidTeamInfoReqPacket->userAddr); // Set User UDP Socket Info

    for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) {
        if (i == myRaidNum) continue; // Skip if it's the current user's number

        auto teamInfo = tempRoom->GetUserInfoByObjNum(i);

        RAID_TEAMINFO_RESPONSE raidTeamInfoResPacket;
        raidTeamInfoResPacket.PacketId = (uint16_t)PACKET_ID::RAID_TEAMINFO_RESPONSE;
        raidTeamInfoResPacket.PacketLength = sizeof(RAID_TEAMINFO_RESPONSE);
        raidTeamInfoResPacket.teamLevel = teamInfo->userLevel;
        strncpy_s(raidTeamInfoResPacket.teamId, teamInfo->userId.c_str(), MAX_USER_ID_LEN);

        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_TEAMINFO_RESPONSE), (char*)&raidTeamInfoResPacket);
    }

    if (tempRoom->StartCheck()) { // Send game start ready message to matched users
        RAID_START raidStartReqPacket1;
        raidStartReqPacket1.PacketId = (uint16_t)PACKET_ID::RAID_START;
        raidStartReqPacket1.PacketLength = sizeof(RAID_START);
        raidStartReqPacket1.endTime = tempRoom->SetEndTime();
        raidStartReqPacket1.mapNum = tempRoom->GetMapNum();
        raidStartReqPacket1.mobHp = tempRoom->GetMobHp();

        for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) {
            connUsersManager->FindUser(tempRoom->GetUserInfoByObjNum(i)->userConnObjNum)->PushSendMsg(sizeof(RAID_START), (char*)&raidStartReqPacket1);
        }
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

            std::vector<unsigned int> tempUserScores;
            tempUserScores.resize(MAX_RAID_ROOM_PLAYERS, 0);

            for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) { // Send raid end message to all users
                auto tempUser = tempRoom->GetUserInfoByObjNum(i);

                RAID_END raidEndReqPacket;
                raidEndReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_END;
                raidEndReqPacket.PacketLength = sizeof(RAID_END);
                connUsersManager->FindUser(tempUser->userConnObjNum)->PushSendMsg(sizeof(RAID_END), (char*)&raidEndReqPacket);

                tempUserScores[i] = tempUser->userScore.load();
            }

            try {
                auto pipe = redis->pipeline("ranking");

                for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) {
                    auto tempUser = tempRoom->GetUserInfoByObjNum(i);

                    if (tempUser->userScore.load() > tempUser->userMaxScore) {
                        pipe.zadd("ranking", tempUser->userId, (double)(tempUser->userScore.load())); // Update Redis if new score exceeds previous best score

                        { // Send sync message to Center Server if new score exceeds previous best score
                            SYNC_HIGHSCORE_REQUEST shreq;
                            shreq.PacketId = (uint16_t)PACKET_ID::SYNC_HIGHSCORE_REQUEST;
                            shreq.PacketLength = sizeof(SYNC_HIGHSCORE_REQUEST);
                            shreq.userScore = tempUser->userScore.load();
                            strncpy_s(shreq.userId, tempUser->userId.c_str(), MAX_USER_ID_LEN);

                            connUsersManager->FindUser(centerServerObjNum)->PushSendMsg(sizeof(SYNC_HIGHSCORE_REQUEST), (char*)&shreq);
                        }
                    }

                    for (int i = 1; i <= MAX_RAID_ROOM_PLAYERS; i++) { // Send scores of users who participated in the raid
                        SEND_RAID_SCORE sendScoreReqPacket;
                        sendScoreReqPacket.PacketId = (uint16_t)PACKET_ID::SEND_RAID_SCORE;
                        sendScoreReqPacket.PacketLength = sizeof(SEND_RAID_SCORE);
                        sendScoreReqPacket.userRaidServerObjNum = i;
                        sendScoreReqPacket.userScore = tempUserScores[i];

                        connUsersManager->FindUser(tempUser->userConnObjNum)->PushSendMsg(sizeof(SEND_RAID_SCORE), (char*)&sendScoreReqPacket);
                    }
                }

                roomManager->DeleteMob(tempRoom);
                pipe.exec(); // Synchronize user rankings
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
        }
    }
    else {
        raidHitResPacket.currentMobHp = hit.first;
        raidHitResPacket.yourScore = hit.second;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);
    }
}