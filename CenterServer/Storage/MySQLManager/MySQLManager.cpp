#include "MySQLManager.h"

MYSQL* MySQLManager::GetConnection() {
    MYSQL* ConnPtr;
    {
        std::lock_guard<std::mutex> lock(dbPoolMutex);
        if (dbPool.empty()) {
            return nullptr;
        }
        ConnPtr = dbPool.front();
        dbPool.pop();
    }
    return ConnPtr;
};

bool MySQLManager::UpdatePassItem(char* passId_, uint32_t userPk_, uint16_t passLevel_, uint16_t passCurrencyType_, uint16_t itemCode, uint16_t daysOrCounts_, uint16_t itemType_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[CheckGetPassItem] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    mysql_autocommit(ConnPtr, false);

    MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);

    std::string query = "UPDATE PassUserRewardData SET rewardBits = rewardBits | ? WHERE userPk = ? AND passId = ? AND passCurrencyType = ? AND ( rewardBits & ?  ) = 0";
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        std::cerr << "[UpdatePassItem] Prepare Error : " << mysql_stmt_error(stmt) << std::endl;
        return false;
    }

    MYSQL_BIND passBitBind[5] = {};
    uint64_t tempBit = 1ULL << (passLevel_ - 1);
    unsigned long passIdLength = strlen(passId_);

    passBitBind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    passBitBind[0].buffer = &tempBit;

    passBitBind[1].buffer_type = MYSQL_TYPE_LONG;
    passBitBind[1].buffer = &userPk_;

    passBitBind[2].buffer_type = MYSQL_TYPE_STRING;
    passBitBind[2].buffer = (void*)passId_;
    passBitBind[2].buffer_length = passIdLength;
    passBitBind[2].length = &passIdLength;

    passBitBind[3].buffer_type = MYSQL_TYPE_LONG;
    passBitBind[3].buffer = &passCurrencyType_;

    passBitBind[4].buffer_type = MYSQL_TYPE_LONGLONG;
    passBitBind[4].buffer = &tempBit;

    if (mysql_stmt_bind_param(stmt, passBitBind) != 0 || mysql_stmt_execute(stmt) != 0) {
        std::cerr << "[UpdatePassItem] Bind or Exec Error : " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        mysql_rollback(ConnPtr);
        mysql_autocommit(ConnPtr, true);
        return false;
    }

    my_ulonglong affected = mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);

    if (affected == 1) { // UPDATE문 성공

        std::string invenQuery;

        if (itemType_ == 0) {
            invenQuery = "INSERT INTO Equipment(user_pk, item_code, daysOrCount) VALUES (?, ?, ?)";
        }
        else if (itemType_ == 1) {
            invenQuery = "INSERT INTO Consumables(user_pk, item_code, daysOrCount) VALUES (?, ?, ?)";
        }
        else if (itemType_ == 2) {
            invenQuery = "INSERT INTO Materials(user_pk, item_code, daysOrCount) VALUES (?, ?, ?)";
        }
        else {
            std::cerr << "[UpdatePassItem] Unknown itemType : " << itemType_ << '\n';
            mysql_rollback(ConnPtr);
            mysql_autocommit(ConnPtr, true);
            return false;
        }

        MYSQL_STMT* invenStmt = mysql_stmt_init(ConnPtr);
        if (mysql_stmt_prepare(invenStmt, invenQuery.c_str(), invenQuery.length()) != 0) {
            std::cerr << "[UpdatePassItem] Prepare invenStmt Error: " << mysql_stmt_error(invenStmt) << std::endl;
            mysql_stmt_close(invenStmt);
            mysql_rollback(ConnPtr);
            mysql_autocommit(ConnPtr, true);
            return false;
        }

        MYSQL_BIND invenBind[3] = {};
        invenBind[0].buffer_type = MYSQL_TYPE_LONG;
        invenBind[0].buffer = &userPk_;

        invenBind[1].buffer_type = MYSQL_TYPE_LONG;
        invenBind[1].buffer = &itemCode;

        invenBind[2].buffer_type = MYSQL_TYPE_LONG;
        invenBind[2].buffer = &daysOrCounts_;

        if (mysql_stmt_bind_param(invenStmt, invenBind) != 0 || mysql_stmt_execute(invenStmt) != 0 || mysql_stmt_affected_rows(invenStmt) == 0) {
            std::cerr << "[UpdatePassItem] Execute invenStmt Error" << std::endl;
            mysql_stmt_close(invenStmt);
            mysql_rollback(ConnPtr);
            mysql_autocommit(ConnPtr, true);
            return false;
        }

        mysql_stmt_close(invenStmt);
    }

    if (mysql_commit(ConnPtr) != 0) {
        std::cerr << "[UpdatePassItem] Commit failed" << std::endl;
        mysql_rollback(ConnPtr);
        mysql_autocommit(ConnPtr, true);
        return false;
    }

    mysql_autocommit(ConnPtr, true);
    return true;
}

// ====================== INITIALIZATION =======================

bool MySQLManager::init() {
    for (int i = 0; i < dbConnectionCount; i++) {
        MYSQL* Conn = mysql_init(nullptr);
        if (!Conn) {
            std::cerr << "Mysql Init Fail" << std::endl;
            return false;
        }

        MYSQL* ConnPtr = mysql_real_connect(Conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_PORT, (char*)NULL, 0);
        if (!ConnPtr) {
            std::cerr << "Mysql Connection Fail : " << mysql_error(Conn) << '\n';
            return false;
        }

        dbPool.push(ConnPtr);
    }

    std::cout << "Mysql Connection Success" << std::endl;
    return true;
}

bool MySQLManager::GetEquipmentItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetEquipmentItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT item_code, itemName, attackPower FROM EquipmentData";
    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[GetEquipmentItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[GetEquipmentItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2]) continue;

            auto equipmentData = std::make_unique<EquipmentItemData>();
            equipmentData->itemCode = (uint16_t)std::stoi(Row[0]);
            equipmentData->itemName = Row[1];
            equipmentData->attackPower = (uint16_t)std::stoi(Row[2]);
            equipmentData->itemType = ItemType::EQUIPMENT;

            itemData_[{equipmentData->itemCode, static_cast<uint16_t>(equipmentData->itemType)}] = std::move(equipmentData);
        }

        mysql_free_result(Result);

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[GetEquipmentItemData] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetConsumableItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetConsumableItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT item_code, itemName FROM ConsumableData";
    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[ConsumableItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[ConsumableItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1]) continue;

            auto consumableData = std::make_unique<ConsumableItemData>();
            consumableData->itemCode = (uint16_t)std::stoi(Row[0]);
            consumableData->itemName = Row[1];
            consumableData->itemType = ItemType::CONSUMABLE;

            itemData_[{consumableData->itemCode, static_cast<uint16_t>(consumableData->itemType)}] = std::move(consumableData);
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[ConsumableItemData] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetMaterialItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetMaterialItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT item_code, itemName FROM MaterialData";
    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[MaterialItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[MaterialItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1]) continue;

            auto materialData = std::make_unique<MaterialItemData>();
            materialData->itemCode = (uint16_t)std::stoi(Row[0]);
            materialData->itemName = Row[1];
            materialData->itemType = ItemType::MATERIAL;

            itemData_[{materialData->itemCode, static_cast<uint16_t>(materialData->itemType)}] = std::move(materialData);
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[MaterialItemData] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetShopItemData(std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash>& shopItemData_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetShopItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT item_code, itemType, itemPrice, itemCount, daysOrCount, currencyType FROM ShopItemData";

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[ShopEquipmentItem] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[ShopEquipmentItem] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2] || !Row[3] || !Row[4] || !Row[5]) continue;

            ShopItem shopItemData;
            shopItemData.itemCode = (uint16_t)std::stoi(Row[0]);
            shopItemData.itemType = static_cast<ItemType>(std::stoi(Row[1]));
            shopItemData.itemPrice = static_cast<uint32_t>(std::stoul(Row[2]));
            shopItemData.itemCount = (uint16_t)std::stoi(Row[3]);
            shopItemData.daysOrCount = (uint16_t)std::stoi(Row[4]);
            shopItemData.currencyType = static_cast<CurrencyType>(std::stoi(Row[5]));

            shopItemData_[{shopItemData.itemCode, shopItemData.daysOrCount}] = shopItemData;
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[ShopEquipmentItem] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetPassInfo(std::vector<std::pair<std::string, PassInfo>>& passInfoVector_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetPassInfo] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT passId, passMaxLevel, eventStart, eventEnd FROM PassInfoData WHERE eventStart<= NOW() AND eventEnd > NOW()";

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[GetPassInfo] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[GetPassInfo] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2] || !Row[3]) continue;

            PassInfo tempInfo;
            tempInfo.passMaxLevel = static_cast<uint16_t>(std::stoi(Row[1]));
            tempInfo.eventStart = Row[2];
            tempInfo.eventEnd = Row[3];

            passInfoVector_.emplace_back(Row[0], tempInfo);
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[GetPassInfo] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetPassItemData(std::vector<std::pair<std::string, PassInfo>>& passInfoVector_, std::unordered_map<std::string, std::unordered_map<PassDataKey, PassItemForSend, PassDataKeyHash>>& passDataMap_) {

    if (passInfoVector_.empty()) {
        // std::cerr << "[GetPassItemData] passInfoVector is empty." << '\n';
        return false;
    }

    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetPassItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT passId, item_code, passLevel, itemCount, daysOrCount, itemType, passCurrencyType FROM PassItemData WHERE passId IN (";
    for (int i = 0; i < passInfoVector_.size(); i++) {
        if (i != passInfoVector_.size() - 1) query_s += "'" + passInfoVector_[i].first + "', ";
        else query_s += "'" + passInfoVector_[i].first + "')";
    }

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[GetPassItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[GetPassItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2] || !Row[3] || !Row[4] || !Row[5] || !Row[6]) continue;

            PassItemForSend passItemData;
            passItemData.itemCode = (uint16_t)std::stoi(Row[1]);
            passItemData.passLevel = (uint16_t)std::stoi(Row[2]);
            passItemData.itemCount = (uint16_t)std::stoi(Row[3]);
            passItemData.daysOrCount = (uint16_t)std::stoi(Row[4]);
            passItemData.itemType = std::stoi(Row[5]);
            passItemData.passCurrencyType = std::stoi(Row[6]);

            passDataMap_[Row[0]][{passItemData.passLevel, static_cast<uint16_t>(passItemData.passCurrencyType)}] = std::move(passItemData);
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[GetPassItemData] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetPassExpData(std::vector<uint16_t>& passExpLimit_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetPassExpData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT level, exp FROM PassExpData";

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[GetPassExpData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[GetPassExpData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        std::unordered_map<uint16_t, uint16_t> passLevelExpMap;
        uint16_t maxLevel = 0;

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1]) continue;

            uint16_t tempLevel = static_cast<uint16_t>(std::stoi(Row[0]));
            uint16_t tempExp = static_cast<uint16_t>(std::stoi(Row[1]));

            passLevelExpMap[tempLevel] = tempExp;
            if (tempLevel > maxLevel) maxLevel = tempLevel;
        }

        // maxLevel 기준으로 passExpLimit_ 크기를 조정하여 passLevelExpMap의 값을 passExpLimit_에 할당
        passExpLimit_.resize(maxLevel + 1);
        for (auto& [level, exp] : passLevelExpMap) {
            passExpLimit_[level] = exp;
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[GetPassExpData] Exception Error : " << e.what() << std::endl;
        return false;
    }
}


// ======================= SYNCRONIZATION =======================

bool MySQLManager::LogoutSync(uint32_t userPk_, USERINFO userInfo_, std::vector<EQUIPMENT> userEquip_,
    std::vector<CONSUMABLES> userConsum_, std::vector<MATERIALS> userMat_, std::vector<UserPassDataForSync> userPassDataForSync_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[LogoutSync] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    mysql_autocommit(ConnPtr, false); // Transaction start

    for (int i = 0; i < 3; i++) { // // Retry up to 3 times on failure
        if (!SyncUserInfo(userPk_, userInfo_)) { std::cout << "SyncUserInfo failed" << '\n'; }
        else if (!SyncEquipment(userPk_, userEquip_)) { std::cout << "SyncEquipment failed" << '\n'; }
        else if (!SyncConsumables(userPk_, userConsum_)) { std::cout << "SyncConsumables failed" << '\n'; }
        else if (!SyncMaterials(userPk_, userMat_)) { std::cout << "SyncMaterials failed" << '\n'; }
        else {
            if (mysql_commit(ConnPtr) == 0) { // If commit is successful, exit
                mysql_autocommit(ConnPtr, true);
                return true;
            }
            else { // If commit fails, rollback
                std::cerr << "mysql_commit failed" << '\n';
                mysql_rollback(ConnPtr);
            }
        }
        mysql_rollback(ConnPtr);
        std::cerr << "userPk : " << userPk_ << " LogoutSync attempt : " << i + 1 << '\n';
    }

    mysql_autocommit(ConnPtr, true);

    // 실패 시 처리 해주는 서버 생성하여 해당 서버로 정보 전달
    //

    std::cerr << "(LogoutSync Failed) userPk : " << userPk_ << '\n';
    return false;
}

bool MySQLManager::SyncUserInfo(uint32_t userPk_, USERINFO userInfo_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncUserInfo] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    try {
        std::string query_s = "UPDATE USERS left join Ranking r on USERS.name = r.name SET USERS.name = '" +
            userInfo_.userId + "', USERS.exp = " + std::to_string(userInfo_.exp) +
            ", USERS.level = " + std::to_string(userInfo_.level) +
            ", USERS.gold = " + std::to_string(userInfo_.gold) + ", USERS.cash = " + std::to_string(userInfo_.cash) +
            ", USERS.mileage = " + std::to_string(userInfo_.mileage) + ", USERS.last_login = current_timestamp" +
            ", USERS.server = " + std::to_string(0) + ", USERS.channel = " + std::to_string(0) +
            ",r.score = " + std::to_string(userInfo_.raidScore) +
            " WHERE USERS.id = " + std::to_string(userPk_);

        const char* Query = query_s.c_str();

        MysqlResult = mysql_query(ConnPtr, Query);
        if (MysqlResult != 0) {
            std::cerr << "(SyncUserInfo) MySQL UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "(SyncUserInfo) Failed to Sync userPk : " << userPk_ << " UserInfo Data" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized UserInfo with MySQL" << std::endl;
    return true;
}


// 인벤토리 동기화 방식 변경
// 기존: 유저 생성 시 모든 슬롯을 0으로 INSERT 후 배치 UPDATE 수행
// 변경: INSERT ... ON DUPLICATE KEY UPDATE (UPDATE + INSERT) 방식으로 통합

bool MySQLManager::SyncEquipment(uint32_t userPk_, std::vector<EQUIPMENT> userEquip_) {
    
    if (userEquip_.empty()) {
        return true;
    }

    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncEquipment] dbPool is empty. Failed to get DB connection.\n";
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    try {
        std::ostringstream q;
        q << "INSERT INTO equipment "<< "(user_pk, position, item_code, enhance) VALUES ";

        bool first = true;
        for (const auto& e : userEquip_) {

            if (!first) q << ", ";
            first = false;

            q << "("
                << userPk_ << ", "
                << e.position << ", "
                << e.itemCode << ", "
                << e.enhance
                << ")";
        }

        q << " ON DUPLICATE KEY UPDATE "
            << "item_code = VALUES(item_code), "
            << "enhance = VALUES(enhance);";

        const std::string query = q.str();

        if (mysql_query(ConnPtr, query.c_str()) != 0) {
            std::cerr << "[SyncEquipment] MySQL UPSERT Error: " << mysql_error(ConnPtr) << "\n";
            return false;
        }

        std::cout << "Successfully Synchronized Equipment with MySQL" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[SyncEquipment] Failed to Sync userPk : " << userPk_ << " Equipments" << std::endl;
        return false;
    }
}

bool MySQLManager::SyncConsumables(uint32_t userPk_, std::vector<CONSUMABLES> userConsum_) {

    if (userConsum_.empty()) {
        return true;
    }

    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncConsumables] dbPool is empty. Failed to get DB connection.\n";
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    try {
        std::ostringstream q;
        q << "INSERT INTO Consumables (user_pk, position, item_code, daysOrCount) VALUES ";

        bool first = true;
        for (const auto& c : userConsum_) {
            if (!first) q << ", ";
            first = false;

            q << "("
                << userPk_ << ", "
                << c.position << ", "
                << c.itemCode << ", "
                << c.count
                << ")";
        }

        q << " ON DUPLICATE KEY UPDATE "
            << "item_code = VALUES(item_code), "
            << "daysOrCount = VALUES(daysOrCount);";

        const std::string query = q.str();

        if (mysql_query(ConnPtr, query.c_str()) != 0) {
            std::cerr << "[SyncConsumables] MySQL UPSERT Error: " << mysql_error(ConnPtr) << "\n";
            return false;
        }

        std::cout << "Successfully Synchronized Consumables with MySQL\n";
        return true;
    }
    catch (const std::exception&) {
        std::cerr << "[SyncConsumables] Failed to Sync userPk: " << userPk_ << " Consumables\n";
        return false;
    }
}

bool MySQLManager::SyncMaterials(uint32_t userPk_, std::vector<MATERIALS> userMat_) {

    if (userMat_.empty()) {
        return true;
    }

    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncMaterials] dbPool is empty. Failed to get DB connection.\n";
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    try {
        std::ostringstream q;
        q << "INSERT INTO Materials (user_pk, position, item_code, daysOrCount) VALUES ";

        bool first = true;
        for (const auto& m : userMat_) {
            if (!first) q << ", ";
            first = false;

            q << "("
                << userPk_ << ", "
                << m.position << ", "
                << m.itemCode << ", "
                << m.count
                << ")";
        }

        q << " ON DUPLICATE KEY UPDATE "
            << "item_code = VALUES(item_code), "
            << "daysOrCount = VALUES(daysOrCount);";

        const std::string query = q.str();

        if (mysql_query(ConnPtr, query.c_str()) != 0) {
            std::cerr << "[SyncMaterials] MySQL UPSERT Error: " << mysql_error(ConnPtr) << "\n";
            return false;
        }

        std::cout << "Successfully Synchronized Materials with MySQL\n";
        return true;
    }
    catch (const std::exception&) {
        std::cerr << "[SyncMaterials] Failed to Sync userPk: " << userPk_ << " Materials\n";
        return false;
    }
}

bool MySQLManager::SyncPassInfo(uint32_t userPk_, std::vector<UserPassDataForSync>& userPassDataForSync_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncPassInfo] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    try {
        std::ostringstream query_s;
        query_s << "UPDATE PassUserData SET ";

        std::ostringstream level_case, exp_case, where;
        level_case << "userPassLevel = CASE ";
        exp_case << "userPassExp = CASE ";

        where << "WHERE userPk = " << std::to_string(userPk_) << " AND passId IN (";

        bool first = true;

        for (const auto& passData : userPassDataForSync_) {
            level_case << "WHEN passId = '" << passData.passId << "' THEN " << passData.passLevel << " ";
            exp_case << "WHEN passId = '" << passData.passId << "' THEN " << passData.passExp << " ";

            if (!first) where << ", ";
            where << "'" << passData.passId << "'";
            first = false;
        }

        level_case << "END, ";
        exp_case << "END ";
        where << ");";

        query_s << level_case.str() << exp_case.str() << where.str();

        if (mysql_query(ConnPtr, query_s.str().c_str()) != 0) {
            std::cerr << "[SyncPassInfo] MySQL Batch UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[SyncPassInfo] Exception during sync : " << e.what() << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized PassInfo with MySQL" << std::endl;
    return true;
}


bool MySQLManager::MySQLSyncEqipmentEnhace(uint32_t userPk_, uint16_t itemPosition_, uint16_t enhancement_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[MySQLSyncEqipmentEnhace] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    try {
        MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);

        std::string query = "UPDATE Equipment SET enhance = ? WHERE user_pk = ?;";
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
            std::cerr << "(MySQLSyncEquipmentEnhance) Equipment Enhance Sync Prepare Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = &enhancement_;

        bind[1].buffer_type = MYSQL_TYPE_LONG;
        bind[1].buffer = &userPk_;

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "(MySQLSyncEquipmentEnhance) Equipment Enhance Sync Bind Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "(MySQLSyncEquipmentEnhance) Equipment Enhance Sync Execute Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "(MySQLSyncEquipmentEnhance) Failed to sync equipment enhancement. userPk : " << userPk_
            << ", position : " << itemPosition_ << ", enhancement : " << enhancement_ << std::endl;
        return false;
    }
}

bool MySQLManager::MySQLSyncUserRaidScore(uint32_t userPk_, unsigned int userScore_, std::string userId_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[MySQLSyncUserRaidScore] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    try {
        MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);

        std::string query = "UPDATE Ranking SET score = ? WHERE id = ?;";
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
            std::cerr << "(MySQLSyncUserRaidScore) Raid Score Sync Prepare Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        unsigned long idLength = userId_.length();

        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = &userScore_;

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)userId_.c_str();
        bind[1].buffer_length = idLength;
        bind[1].length = &idLength;

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "(MySQLSyncUserRaidScore) Raid Score Sync Bind Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "(MySQLSyncUserRaidScore) Raid Score Sync Execute Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "(MySQLSyncUserRaidScore) Failed to sync raid score. userId : " << userId_
            << ", score : " << userScore_ << std::endl;
        return false;
    }
}

bool MySQLManager::CashCharge(uint32_t userPk_, uint32_t chargedAmount) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[CashCharge] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    try {
        MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);

        std::string query = "UPDATE USERS SET cash = cash + ? WHERE id = ?";
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
            std::cerr << "[CashCharge] Statement prepare error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        MYSQL_BIND bind[2] = {};
        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = (char*)&chargedAmount;

        bind[1].buffer_type = MYSQL_TYPE_LONG;
        bind[1].buffer = (char*)&userPk_;

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "[CashCharge] Bind error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "[CashCharge] Execute error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[CashCharge] Exception : " << e.what() << " (UserPk: " << userPk_ << ")" << '\n';
        return false;
    }
}