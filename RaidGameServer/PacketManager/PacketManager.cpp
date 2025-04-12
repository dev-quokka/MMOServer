#include "PacketManager.h"

thread_local std::mt19937 PacketManager::gen(std::random_device{}());

void PacketManager::init(const uint16_t packetThreadCnt_) {

    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(UINT16)PACKET_ID::IM_GAME_RESPONSE] = &PacketManager::ImGameRequest;

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
        std::cout << "Redis Cluster Connect Success !" << std::endl;

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

void PacketManager::ImGameRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<IM_GAME_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Connected Fail to the central server" << std::endl;
        return;
    }

    std::cout << "Connected to the central server" << std::endl;
}

void PacketManager::UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_GAME_REQUEST*>(pPacket_);
    std::string key = "jwtcheck:{" + std::to_string(static_cast<uint16_t>(ServerType::RaidGameServer01)) + "}";

    USER_CONNECT_GAME_RESPONSE ucRes;
    ucRes.PacketId = (uint16_t)PACKET_ID::USER_CONNECT_GAME_RESPONSE;
    ucRes.PacketLength = sizeof(USER_CONNECT_GAME_RESPONSE);

    { // JWT 토큰 payload에 있는 아이디로 유저 체크
        auto tempToken = jwt::decode((std::string)userConn->userToken);
        auto tempId = tempToken.get_payload_claim("room_id");

        std::string user_id = tempId.as_string();

        if (user_id != (std::string)userConn->userId) {
            ucRes.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
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



            ucRes.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_GAME_RESPONSE), (char*)&ucRes);
            std::cout << (std::string)userConn->userId << " Connect" << std::endl;
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
    // 방에서 유저 상태 오프라인으로 변경

}

void PacketManager::MakeRoom(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchReqPacket = reinterpret_cast<MATCHING_REQUEST_TO_GAME_SERVER*>(pPacket_);

    RaidUserInfo* user1;
    RaidUserInfo* user2;

    user1->userObjNum = 1;
    user2->userObjNum = 2;

    user1->userPk = matchReqPacket->userPk1;
    user2->userPk = matchReqPacket->userPk2;

    std::vector<std::string> fields = { "id", "level" };
    std::vector<sw::redis::OptionalString> values;

    { // 매칭된 유저 데이터 레디스 클러스터에서 가져오기
        std::string tag1 = "{" + std::to_string(matchReqPacket->userPk1) + "}";
        std::string key1 = "userinfo:" + tag1;

        redis->hmget(key1, fields.begin(), fields.end(), std::back_inserter(values));

        if (values[0] && values[1] && values[2]) {
            user1->userId = *values[0];
            user1->userLevel = static_cast<uint16_t>(std::stoul(*values[1]));
        }
    }

    values.clear(); // 값 벡터 재사용전에 초기화 

    { // 매칭된 유저 데이터 레디스 클러스터에서 가져오기
        std::string tag2 = "{" + std::to_string(matchReqPacket->userPk2) + "}";
        std::string key2 = "userinfo:" + tag2;

        redis->hmget(key2, fields.begin(), fields.end(), std::back_inserter(values));

        if (values[0] && values[1] && values[2]) {
            user2->userId = *values[0];
            user2->userLevel = static_cast<uint16_t>(std::stoul(*values[1]));
        }
    }

    std::discrete_distribution<int> dist(mapProbabilities.begin(), mapProbabilities.end()); // 확률에 따른 맵 랜덤 선택

    MATCHING_REQUEST_TO_GAME_SERVER match

        if (!roomManager->MakeRoom(matchReqPacket->roomNum, dist(gen), 10, 30, user1, user2)) {

        }

}

void PacketManager::RaidTeamInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto raidTeamInfoReqPacket = reinterpret_cast<RAID_TEAMINFO_REQUEST*>(pPacket_);

    Room* tempRoom = roomManager->GetRoom(raidTeamInfoReqPacket->roomNum);
    tempRoom->setSockAddr(raidTeamInfoReqPacket->myNum, raidTeamInfoReqPacket->userAddr); // Set User UDP Socket Info

    InGameUser* teamUser = tempRoom->GetTeamUser(raidTeamInfoReqPacket->myNum);

    RAID_TEAMINFO_RESPONSE raidTeamInfoResPacket;
    raidTeamInfoResPacket.PacketId = (uint16_t)PACKET_ID::RAID_TEAMINFO_RESPONSE;
    raidTeamInfoResPacket.PacketLength = sizeof(RAID_TEAMINFO_RESPONSE);
    raidTeamInfoResPacket.teamLevel = teamUser->GetLevel();
    strncpy_s(raidTeamInfoResPacket.teamId, teamUser->GetId().c_str(), MAX_USER_ID_LEN);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_TEAMINFO_RESPONSE), (char*)&raidTeamInfoResPacket);

    if (tempRoom->StartCheck()) { // 두 명의 유저에게 팀의 정보를 전달하고 둘 다 받음 확인하면 게임 시작 정보 보내주기
        RAID_START_REQUEST raidStartReqPacket1;
        raidStartReqPacket1.PacketId = (uint16_t)PACKET_ID::RAID_START_REQUEST;
        raidStartReqPacket1.PacketLength = sizeof(RAID_START_REQUEST);
        raidStartReqPacket1.endTime = tempRoom->SetEndTime();

        RAID_START_REQUEST raidStartReqPacket2;
        raidStartReqPacket2.PacketId = (uint16_t)PACKET_ID::RAID_START_REQUEST;
        raidStartReqPacket2.PacketLength = sizeof(RAID_START_REQUEST);
        raidStartReqPacket2.endTime = tempRoom->SetEndTime();

        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_START_REQUEST), (char*)&raidStartReqPacket1);
        connUsersManager->FindUser(tempRoom->GetTeamObjNum(raidTeamInfoReqPacket->myNum))->PushSendMsg(sizeof(RAID_START_REQUEST), (char*)&raidStartReqPacket2);
    }
}

void PacketManager::RaidHit(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto raidHitReqPacket = reinterpret_cast<RAID_HIT_REQUEST*>(pPacket_);
    InGameUser* user = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    RAID_HIT_RESPONSE raidHitResPacket;
    raidHitResPacket.PacketId = (uint16_t)PACKET_ID::RAID_HIT_RESPONSE;
    raidHitResPacket.PacketLength = sizeof(RAID_HIT_RESPONSE);

    auto room = roomManager->GetRoom(raidHitReqPacket->roomNum);
    auto hit = room->Hit(raidHitReqPacket->myNum, raidHitReqPacket->damage);

    if (hit.first <= 0) { // Mob Dead
        if (room->EndCheck()) { // SendEndMsg
            raidHitResPacket.currentMobHp = 0;
            raidHitResPacket.yourScore = hit.second;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);

            InGameUser* inGameUser;
            try {
                auto pipe = redis->pipeline("ranking");
                for (int i = 0; i < room->GetRoomUserCnt(); i++) {  // 레이드 종료 메시지
                    inGameUser = room->GetUser(i);

                    RAID_END_REQUEST raidEndReqPacket;
                    raidEndReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST;
                    raidEndReqPacket.PacketLength = sizeof(RAID_END_REQUEST);
                    raidEndReqPacket.userScore = room->GetScore(i);
                    raidEndReqPacket.teamScore = room->GetTeamScore(i);
                    connUsersManager->FindUser(room->GetUserObjNum(i))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket);

                    if (room->GetScore(i) > room->GetUser(i)->GetScore()) {
                        pipe.zadd("ranking", inGameUser->GetId(), (double)(room->GetScore(i))); // 점수 레디스에 동기화
                    }

                }
                pipe.exec(); // 유저들 랭킹 동기화
                matchingManager->DeleteMob(room); // 방 종료 처리
            }
            catch (const sw::redis::Error& e) {
                std::cerr << "Redis error: " << e.what() << std::endl;
                for (int i = 0; i < room->GetRoomUserCnt(); i++) {
                    std::cout << room->GetUser(i)->GetId() << " 유저 점수 : " << room->GetScore(i) << std::endl;
                }
                std::cout << "동기화 실패" << std::endl;
                matchingManager->DeleteMob(room); // 방 종료 처리
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

    if (hit.second != 0) { // Score이 0이 아니면 웹 서버에 동기화 메시지 전송 
        connUsersManager->FindUser(GatewayServerObjNum)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);
    }
}