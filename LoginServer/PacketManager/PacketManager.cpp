#include "PacketManager.h"

thread_local std::mt19937 PacketManager::gen(std::random_device{}());

void PacketManager::init(const uint16_t packetThreadCnt_) {

    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(UINT16)PACKET_ID::LOGIN_SERVER_CONNECT_RESPONSE] = &PacketManager::ImLoginResponse;

    packetIDTable[(UINT16)PACKET_ID::USERINFO_REQUEST] = &PacketManager::GetUserInfo;
    packetIDTable[(UINT16)PACKET_ID::EQUIPMENT_REQUEST] = &PacketManager::GetEquipment;
    packetIDTable[(UINT16)PACKET_ID::CONSUMABLES_REQUEST] = &PacketManager::GetConsumables;
    packetIDTable[(UINT16)PACKET_ID::MATERIALS_REQUEST] = &PacketManager::GetMaterials;

    packetIDTable[(UINT16)PACKET_ID::USER_GAMESTART_REQUEST] = &PacketManager::GameStart;

    PacketRun(packetThreadCnt_);
}

void PacketManager::PacketRun(const uint16_t packetThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        redis = std::make_shared<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connected" << std::endl;

        mySQLManager = new MySQLManager;
        mySQLManager->init(redis);

        CreatePacketThread(packetThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis Connect Error : " << err.what() << std::endl;
    }
}

void PacketManager::SetManager(ConnUsersManager* connUsersManager_) {
    connUsersManager = connUsersManager_;
}

bool PacketManager::CreatePacketThread(const uint16_t packetThreadCnt_) {
    packetRun = true;

    try {
        for (int i = 0; i < packetThreadCnt_; i++) {
            packetThreads.emplace_back(std::thread([this]() { PacketThread(); }));
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Create Redis Thread Failed : " << e.what() << std::endl;
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

void PacketManager::ImLoginResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto centerConn = reinterpret_cast<LOGIN_SERVER_CONNECT_RESPONSE*>(pPacket_);

    if (!centerConn->isSuccess) {
        std::cout << "Failed to Authenticate with Center Server" << std::endl;
        return;
    }

    std::cout << "Successfully Authenticated with Center Server" << std::endl;
}

void PacketManager::GetUserInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto uiReq = reinterpret_cast<USERINFO_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    std::pair<uint32_t, USERINFO> userInfoPk = mySQLManager->GetUserInfoById(uiReq->userId);

    if (userInfoPk.first == 100) { // Return value 100 indicates failure
        std::cout << "GetUserInfo Fail" << std::endl;
        return;
    }

    tempUser->SetPk(userInfoPk.first);

    USERINFO_RESPONSE uiRes;
    uiRes.PacketId = (UINT16)PACKET_ID::USERINFO_RESPONSE;
    uiRes.PacketLength = sizeof(USERINFO_RESPONSE);
    uiRes.UserInfo = userInfoPk.second;

    std::cout << "Successfully uploaded user data to the redis cluster" << std::endl;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USERINFO_RESPONSE), (char*)&uiRes);
}

void PacketManager::GetEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<EQUIPMENT_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    std::pair<uint16_t, char*> eq = mySQLManager->GetUserEquipByPk(std::to_string(tempUser->GetPk()));

    if (eq.first == 100) {  // Return value 100 indicates failure
        std::cout << "Fail to GetUserEquip()" << std::endl;
        return;
    }

    EQUIPMENT_RESPONSE eqSend;
    eqSend.PacketId = (UINT16)PACKET_ID::EQUIPMENT_RESPONSE;
    eqSend.PacketLength = sizeof(EQUIPMENT_RESPONSE);
    eqSend.eqCount = eq.first;
    std::memcpy(eqSend.Equipments, eq.second, MAX_INVEN_SIZE + 1);

    std::cout << "Successfully uploaded user's equipment to the game server" << std::endl;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(EQUIPMENT_RESPONSE), (char*)&eqSend);
}

void PacketManager::GetConsumables(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<CONSUMABLES_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    std::pair<uint16_t, char*> es = mySQLManager->GetUserConsumablesByPk(std::to_string(tempUser->GetPk()));

    if (es.first == 100) {
        std::cout << "GetUserConsumables Fail" << std::endl;
        return;
    }

    CONSUMABLES_RESPONSE csSend;
    csSend.PacketId = (UINT16)PACKET_ID::CONSUMABLES_RESPONSE;
    csSend.PacketLength = sizeof(CONSUMABLES_RESPONSE);
    csSend.csCount = es.first;
    std::memcpy(csSend.Consumables, es.second, MAX_INVEN_SIZE + 1);

    std::cout << "Successfully uploaded user's consumable to the game server" << std::endl;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(CONSUMABLES_RESPONSE), (char*)&csSend);
}

void PacketManager::GetMaterials(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<MATERIALS_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    std::pair<uint16_t, char*> em = mySQLManager->GetUserMaterialsByPk(std::to_string(tempUser->GetPk()));

    if (em.first == 100) {
        std::cout << "GetUserMaterials Fail" << std::endl;
        return;
    }

    MATERIALS_RESPONSE mtSend;
    mtSend.PacketId = (UINT16)PACKET_ID::MATERIALS_RESPONSE;
    mtSend.PacketLength = sizeof(MATERIALS_RESPONSE);
    mtSend.mtCount = em.first;
    std::memcpy(mtSend.Materials, em.second, MAX_INVEN_SIZE + 1);

    std::cout << "Successfully uploaded user's material to the game server" << std::endl;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATERIALS_RESPONSE), (char*)&mtSend);
}

void PacketManager::GameStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto ugReq = reinterpret_cast<USER_GAMESTART_REQUEST*>(pPacket_);
    auto tempUser = connUsersManager->FindUser(connObjNum_);

    std::string token = jwt::create()
        .set_issuer("session_server")
        .set_subject("Login_check")
        .set_expires_at(std::chrono::system_clock::now() +
            std::chrono::seconds{ 600 })
        .sign(jwt::algorithm::hs256{ JWT_SECRET });

    std::string tag = "{" + std::string(ugReq->userId) + "}";
    std::string key = "jwtcheck:" + tag;

    auto pipe = redis->pipeline(tag);
    pipe.hset(key, token, std::to_string(tempUser->GetPk()))
        .expire(key, 15); // set ttl 1 hour
    pipe.exec();

    USER_GAMESTART_RESPONSE ugRes;
    ugRes.PacketId = (UINT16)PACKET_ID::USER_GAMESTART_RESPONSE;
    ugRes.PacketLength = sizeof(USER_GAMESTART_RESPONSE);
    strncpy_s(ugRes.Token, token.c_str(), 256);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_GAMESTART_RESPONSE), (char*)&ugRes);
}