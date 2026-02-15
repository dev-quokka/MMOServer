#include "RedisManager.h"

// ========================== INITIALIZATION =========================

void RedisManager::init(const uint16_t packetThreadCnt_) {

    // -------------------- SET PACKET HANDLERS ----------------------

    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // LOGIN
    packetIDTable[(UINT16)PACKET_ID::LOGIN_SERVER_CONNECT_RESPONSE] = &RedisManager::ImLoginResponse;
    packetIDTable[(UINT16)PACKET_ID::USERINFO_REQUEST] = &RedisManager::GetUserInfo;
    packetIDTable[(UINT16)PACKET_ID::EQUIPMENT_REQUEST] = &RedisManager::GetEquipment;
    packetIDTable[(UINT16)PACKET_ID::CONSUMABLES_REQUEST] = &RedisManager::GetConsumables;
    packetIDTable[(UINT16)PACKET_ID::MATERIALS_REQUEST] = &RedisManager::GetMaterials;
    packetIDTable[(UINT16)PACKET_ID::PASSINFO_REQUEST] = &RedisManager::GetPassInfos;
    packetIDTable[(UINT16)PACKET_ID::PASSREWARDINFO_REQUEST] = &RedisManager::GetPassRewordInfos;
    packetIDTable[(UINT16)PACKET_ID::USER_GAMESTART_REQUEST] = &RedisManager::GameStart;

    RedisRun(packetThreadCnt_);

    mySQLManager = new MySQLManager;
    mySQLManager->init();

    auto tempRanks = mySQLManager->SetRankingInRedis();
    if (tempRanks.size() == 0) {
        std::cout << "(MySQL) Failed to set user rankings from MySQL." << std::endl;
        return;
    }

    SetRanking(tempRanks);
    SetEventPassId();
}

void RedisManager::SetManager(ConnUsersManager* connUsersManager_) {
    connUsersManager = connUsersManager_;
}

void RedisManager::SetRanking(std::vector<RANKING> ranks_) {

    auto pipe = redis->pipeline("ranking");

    for (int i = 0; i < ranks_.size(); i++) {
        pipe.zadd("ranking", std::string(ranks_[i].userId), ranks_[i].score);
    }

    pipe.exec();
}

void RedisManager::SetEventPassId() {
    if (!mySQLManager->GetEventPassId(eventPassInfoMap)) return;
}


// ===================== PACKET MANAGEMENT =====================

void RedisManager::RedisRun(const uint16_t packetThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        redis = std::make_unique<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connected" << std::endl;

        CreateRedisThread(packetThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis Connect Error : " << err.what() << std::endl;
    }
}

void RedisManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}

bool RedisManager::CreateRedisThread(const uint16_t packetThreadCnt_) {
    redisRun = true;

    try {
        for (int i = 0; i < packetThreadCnt_; i++) {
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


// ======================================================= LOGIN SERVER =======================================================

void RedisManager::ImLoginResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<LOGIN_SERVER_CONNECT_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Failed to Authenticate with Center Server" << std::endl;
        return;
    }

    std::cout << "Successfully Authenticated with Center Server" << std::endl;
}

void RedisManager::GetUserInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto uiReq = reinterpret_cast<USERINFO_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    std::string tempUserId = std::string(uiReq->userId);
    std::pair<uint32_t, LOGIN_USERINFO> userInfoPk;

    USERINFO_RESPONSE uiRes;
    uiRes.PacketId = (UINT16)PACKET_ID::USERINFO_RESPONSE;
    uiRes.PacketLength = sizeof(USERINFO_RESPONSE);

    if (!mySQLManager->GetUserInfoById(tempUserId, userInfoPk)) {
        std::cout << "[GetUserInfo] GetUserInfo Fail" << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USERINFO_RESPONSE), (char*)&uiRes);
        return;
    }

    std::string tag = "{" + std::to_string(userInfoPk.first) + "}";
    std::string key = "userinfo:" + tag; // user:{pk}

    auto tempLoginUserInfo = userInfoPk.second;

    try {
        auto pipe = redis->pipeline(tag);

        pipe.hset(key, "userId", std::string(uiReq->userId))
            .hset(key, "level", std::to_string(tempLoginUserInfo.level))
            .hset(key, "exp", std::to_string(tempLoginUserInfo.exp))
            .hset(key, "gold", std::to_string(tempLoginUserInfo.gold))
            .hset(key, "cash", std::to_string(tempLoginUserInfo.cash))
            .hset(key, "mileage", std::to_string(tempLoginUserInfo.mileage))
            .hset(key, "lastLogin", tempLoginUserInfo.lastLogin)
            .hset(key, "raidScore", std::to_string(tempLoginUserInfo.raidScore))
            .hset(key, "status", "Online")
            .expire(key, 3600);

        pipe.exec();
    }
    catch (const sw::redis::Error& e) { // Redis Error
        std::cerr << "[GetUserInfo] Redis error : " << e.what() << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USERINFO_RESPONSE), (char*)&uiRes);
        return;
    }

    std::cout << "Successfully uploaded UserInfo to the redis cluster" << std::endl;

    tempUser->SetPk(userInfoPk.first);

    USERINFO tempUserInfo;
    tempUserInfo.level = tempLoginUserInfo.level;
    tempUserInfo.exp = tempLoginUserInfo.exp;
    tempUserInfo.raidScore = tempLoginUserInfo.raidScore;
    tempUserInfo.gold = tempLoginUserInfo.gold;
    tempUserInfo.cash = tempLoginUserInfo.cash;
    tempUserInfo.mileage = tempLoginUserInfo.mileage;

    uiRes.UserInfo = tempUserInfo;
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USERINFO_RESPONSE), (char*)&uiRes);
}

void RedisManager::GetEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<EQUIPMENT_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    EQUIPMENT_RESPONSE eqSend;
    eqSend.PacketId = (UINT16)PACKET_ID::EQUIPMENT_RESPONSE;
    eqSend.PacketLength = sizeof(EQUIPMENT_RESPONSE);

    std::string tempPk = std::to_string(tempUser->GetPk());
    std::vector<EQUIPMENT> eq;

    if (!mySQLManager->GetUserEquipByPk(tempPk, eq)) {
        std::cout << "[GetEquipment] Failed to Get User Get User Equipment" << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(EQUIPMENT_RESPONSE), (char*)&eqSend);
        return;
    }

    char* tempC = new char[MAX_INVEN_SIZE + 1];
    char* tc = tempC;

    std::string tag = "{" + std::to_string(tempUser->GetPk()) + "}";
    std::string key = "equipment:" + tag; // user:{pk}

    try {
        auto pipe = redis->pipeline(tag);

        for (int i = 0; i < eq.size(); i++) {
            pipe.hset(key, std::to_string(eq[i].position), std::to_string(eq[i].itemCode) + ":" + std::to_string(eq[i].enhance));

            memcpy(tc, (char*)&eq[i], sizeof(EQUIPMENT));
            tc += sizeof(EQUIPMENT);
        }

        pipe.exec();
    }
    catch (const sw::redis::Error& e) { // Redis Error
        std::cerr << "[GetEquipment] Redis error : " << e.what() << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(EQUIPMENT_RESPONSE), (char*)&eqSend);
        delete[] tempC;
        return;
    }
    catch (const std::exception& e) {
        std::cerr << "[GetEquipment] Exception error : " << e.what() << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(EQUIPMENT_RESPONSE), (char*)&eqSend);
        delete[] tempC;
        return;
    }

    std::cout << "Successfully uploaded user's Equipment to the game server" << std::endl;

    eqSend.eqCount = eq.size();
    std::memcpy(eqSend.Equipments, tempC, MAX_INVEN_SIZE + 1);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(EQUIPMENT_RESPONSE), (char*)&eqSend);

    delete[] tempC;
}

void RedisManager::GetConsumables(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<CONSUMABLES_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    CONSUMABLES_RESPONSE csSend;
    csSend.PacketId = (UINT16)PACKET_ID::CONSUMABLES_RESPONSE;
    csSend.PacketLength = sizeof(CONSUMABLES_RESPONSE);

    std::string tempPk = std::to_string(tempUser->GetPk());
    std::vector<CONSUMABLES> cs;

    if (!mySQLManager->GetUserConsumablesByPk(tempPk, cs)) {
        std::cout << "[GetConsumables] Failed to Get User Consumables" << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(CONSUMABLES_RESPONSE), (char*)&csSend);
        return;
    }

    char* tempC = new char[MAX_INVEN_SIZE + 1];
    char* tc = tempC;

    std::string tag = "{" + std::to_string(tempUser->GetPk()) + "}";
    std::string key = "consumables:" + tag; // user:{pk}

    try {
        auto pipe = redis->pipeline(tag);

        for (int i = 0; i < cs.size(); i++) {
            pipe.hset(key, std::to_string(cs[i].position), std::to_string(cs[i].itemCode) + ":" + std::to_string(cs[i].count));

            memcpy(tc, (char*)&cs[i], sizeof(CONSUMABLES));
            tc += sizeof(CONSUMABLES);
        }

        pipe.exec();
    }
    catch (const sw::redis::Error& e) { // Redis Error
        std::cerr << "[GetConsumables] Redis error : " << e.what() << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(CONSUMABLES_RESPONSE), (char*)&csSend);
        delete[] tempC;
        return;
    }
    catch (const std::exception& e) {
        std::cerr << "[GetConsumables] Exception error : " << e.what() << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(CONSUMABLES_RESPONSE), (char*)&csSend);
        delete[] tempC;
        return;
    }

    std::cout << "Successfully uploaded user's Consumable to the game server" << std::endl;

    csSend.csCount = cs.size();
    std::memcpy(csSend.Consumables, tempC, MAX_INVEN_SIZE + 1);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(CONSUMABLES_RESPONSE), (char*)&csSend);

    delete[] tempC;
}

void RedisManager::GetMaterials(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<MATERIALS_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    MATERIALS_RESPONSE mtSend;
    mtSend.PacketId = (UINT16)PACKET_ID::MATERIALS_RESPONSE;
    mtSend.PacketLength = sizeof(MATERIALS_RESPONSE);

    std::string tempPk = std::to_string(tempUser->GetPk());
    std::vector<MATERIALS> mt;

    if (!mySQLManager->GetUserMaterialsByPk(tempPk, mt)) {
        std::cout << "[GetMaterials] Failed to Get User Materials" << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATERIALS_RESPONSE), (char*)&mtSend);
        return;
    }

    char* tempC = new char[MAX_INVEN_SIZE + 1];
    char* tc = tempC;

    std::string tag = "{" + std::to_string(tempUser->GetPk()) + "}";
    std::string key = "materials:" + tag; // user:{pk}

    try {
        auto pipe = redis->pipeline(tag);

        for (int i = 0; i < mt.size(); i++) {
            pipe.hset(key, std::to_string(mt[i].position), std::to_string(mt[i].itemCode) + ":" + std::to_string(mt[i].count));

            memcpy(tc, (char*)&mt[i], sizeof(MATERIALS));
            tc += sizeof(MATERIALS);
        }

        pipe.exec();
    }
    catch (const sw::redis::Error& e) { // Redis Error
        std::cerr << "[GetMaterials] Redis error : " << e.what() << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATERIALS_RESPONSE), (char*)&mtSend);
        delete[] tempC;
        return;
    }

    std::cout << "Successfully uploaded user's Material to the game server" << std::endl;

    mtSend.mtCount = mt.size();
    std::memcpy(mtSend.Materials, tempC, MAX_INVEN_SIZE + 1);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATERIALS_RESPONSE), (char*)&mtSend);

    delete[] tempC;
}

void RedisManager::GetPassInfos(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<PASSINFO_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    PASSINFO_RESPONSE piRes;
    piRes.PacketId = (UINT16)PACKET_ID::PASSINFO_RESPONSE;
    piRes.PacketLength = sizeof(PASSINFO_RESPONSE);

    std::string tempPk = std::to_string(tempUser->GetPk());
    std::vector<USERPASSINFO> pi;

    if (!mySQLManager->GetPassInfoByPk(tempPk, eventPassInfoMap, pi)) {
        std::cout << "[GetPassInfos] Failed to Get User PassRewordInfos" << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(PASSINFO_RESPONSE), (char*)&piRes);
        return;
    }

    auto passInfoSize = pi.size();

    size_t packetSize = sizeof(PASSINFO_RESPONSE) + sizeof(USERPASSINFO) * passInfoSize;
    char* packetBuffer = new char[packetSize];

    piRes.passCount = passInfoSize;
    memcpy(packetBuffer, &piRes, sizeof(PASSINFO_RESPONSE));

    USERPASSINFO* passInfoVector = reinterpret_cast<USERPASSINFO*>(packetBuffer + sizeof(PASSINFO_RESPONSE));

    std::string tempPassId;
    std::string tag = "{" + std::to_string(tempUser->GetPk()) + "}:";
    std::string passKey;

    try {
        auto pipe = redis->pipeline(tag);

        for (int i = 0; i < passInfoSize; i++) {
            tempPassId = pi[i].passInfo.passId;
            passKey = "pass:" + tag + tempPassId;
            redis->hset(passKey, "passLevel", std::to_string(pi[i].passLevel));
            redis->hset(passKey, "passExp", std::to_string(pi[i].passExp));
            redis->hset(passKey, "passCurrencyType", std::to_string(pi[i].passCurrencyType));

            passInfoVector[i] = pi[i];
        }

        pipe.exec();
    }
    catch (const sw::redis::Error& e) { // Redis Error
        std::cerr << "[GetPassInfos] Redis error : " << e.what() << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(PASSINFO_RESPONSE), (char*)&piRes);
        return;
    }

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(packetSize, packetBuffer);
}

void RedisManager::GetPassRewordInfos(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<PASSREWARDINFO_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    PASSREWARDINFO_RESPONSE priRes;
    priRes.PacketId = (UINT16)PACKET_ID::PASSREWARDINFO_RESPONSE;
    priRes.PacketLength = sizeof(PASSREWARDINFO_RESPONSE);

    std::string tempPk = std::to_string(tempUser->GetPk());
    std::vector<PASSREWARDNFO> pri;

    if (!mySQLManager->GetPassDataByPk(tempPk, eventPassInfoMap, pri)) {
        std::cout << "[GetPassRewordInfos] Failed to Get User PassRewordInfos" << std::endl;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(PASSREWARDINFO_RESPONSE), (char*)&priRes);
        return;
    }

    auto passRewardSize = pri.size();

    size_t packetSize = sizeof(PASSREWARDINFO_RESPONSE) + sizeof(PASSREWARDNFO) * passRewardSize;
    char* packetBuffer = new char[packetSize];

    priRes.passRewordCount = passRewardSize;
    memcpy(packetBuffer, &priRes, sizeof(PASSREWARDINFO_RESPONSE));

    PASSREWARDNFO* passRewawrdVector = reinterpret_cast<PASSREWARDNFO*>(packetBuffer + sizeof(PASSREWARDINFO_RESPONSE));

    for (int i = 0; i < passRewardSize; i++) {
        passRewawrdVector[i] = pri[i];
    }

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(packetSize, packetBuffer);
}

void RedisManager::GameStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<USER_GAMESTART_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    USER_GAMESTART_RESPONSE ugRes;
    ugRes.PacketId = (UINT16)PACKET_ID::USER_GAMESTART_RESPONSE;
    ugRes.PacketLength = sizeof(USER_GAMESTART_RESPONSE);

    try {
        std::string token = jwt::create()
            .set_issuer("session_server")
            .set_subject("Login_check")
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        std::string tag = "{" + std::string(ugReq->userId) + "}";
        std::string key = "jwtcheck:" + tag;

        auto pipe = redis->pipeline(tag);
        pipe.hset(key, token, std::to_string(tempUser->GetPk()))
            .expire(key, 300); // set ttl
        pipe.exec();

        strncpy_s(ugRes.Token, token.c_str(), 256);
        connUsersManager->FindUser(connObjNum_)->
            PushSendMsg(sizeof(USER_GAMESTART_RESPONSE), (char*)&ugRes);
    }

    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;

        // Send 0 when JWT token generation fails
        memset(ugRes.Token, 0, sizeof(ugRes.Token));
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_GAMESTART_RESPONSE), (char*)&ugRes);
        return;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;

        // Send 0 when JWT token generation fails
        memset(ugRes.Token, 0, sizeof(ugRes.Token));
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_GAMESTART_RESPONSE), (char*)&ugRes);
    }
}