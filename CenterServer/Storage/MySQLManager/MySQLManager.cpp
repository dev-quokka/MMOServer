#include "MySQLManager.h"

// ====================== INITIALIZATION =======================

bool MySQLManager::init() {
    mysql_init(&Conn);

    ConnPtr = mysql_real_connect(&Conn, "127.0.0.1", "quokka", "1234", "iocp", 3306, (char*)NULL, 0);
    if (ConnPtr == NULL) {
        std::cout << mysql_error(&Conn) << std::endl;
        std::cout << "Mysql Connection Fail" << std::endl;
        return false;
    }

    std::cout << "Mysql Connection Success" << std::endl;
    return true;
}


// ======================= SYNCRONIZATION =======================

bool MySQLManager::LogoutSync(uint16_t userPk_, USERINFO userInfo_, std::vector<EQUIPMENT> userEquip_,
    std::vector<CONSUMABLES> userConsum_, std::vector<MATERIALS> userMat_) {
    SyncUserInfo(userPk_, userInfo_);
    SyncEquipment(userPk_, userEquip_);
    SyncConsumables(userPk_, userConsum_);
    SyncMaterials(userPk_, userMat_);
    return true;
}

bool MySQLManager::SyncUserInfo(uint16_t userPk_, USERINFO userInfo_) {
    try {
        std::string query_s = "UPDATE USERS left join Ranking r on USERS.name = r.name SET USERS.name = '" +
            userInfo_.userId + "', USERS.exp = " + std::to_string(userInfo_.exp) +
            ", USERS.level = " + std::to_string(userInfo_.level) + ", USERS.last_login = current_timestamp" +
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

bool MySQLManager::SyncEquipment(uint16_t userPk_, std::vector<EQUIPMENT> userEquip_) {
    try {
        std::ostringstream query_s;
        query_s << "UPDATE Equipment SET ";

        std::ostringstream item_code_case, enhancement_case, where;
        item_code_case << "Item_code = CASE ";
        enhancement_case << "enhance = CASE ";

        where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

        bool first = true;

        for (auto& tempEquip : userEquip_) {

            item_code_case << "WHEN position = " << tempEquip.position << " THEN " << tempEquip.itemCode << " ";
            enhancement_case << "WHEN position = " << tempEquip.position << " THEN " << tempEquip.enhance << " ";

            if (!first) where << ", ";
            where << tempEquip.position;
            first = false;
        }

        item_code_case << "END, ";
        enhancement_case << "END ";
        where << ");";

        query_s << item_code_case.str() << enhancement_case.str() << where.str();
        if (mysql_query(ConnPtr, query_s.str().c_str()) != 0) {
            std::cerr << "(SyncEquipment) MySQL Batch UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "(SyncEquipment) Failed to Sync userPk : " << userPk_ << " Equipments" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized Equipment with MySQL" << std::endl;
    return true;
}

bool MySQLManager::SyncConsumables(uint16_t userPk_, std::vector<CONSUMABLES> userConsum_) {
    try {
        std::ostringstream query_s;
        query_s << "UPDATE Consumables SET ";

        std::ostringstream item_code_case, count_case, where;
        item_code_case << "Item_code = CASE ";
        count_case << "count = CASE ";

        where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

        bool first = true;
        for (auto& tempConum : userConsum_) { // key = Pos, value = (code, count)

            item_code_case << "WHEN position = " << tempConum.position << " THEN " << tempConum.itemCode << " ";
            count_case << "WHEN position = " << tempConum.position << " THEN " << tempConum.count << " ";

            if (!first) where << ", ";
            where << tempConum.position;
            first = false;
        }

        item_code_case << "END, ";
        count_case << "END ";
        where << ");";

        query_s << item_code_case.str() << count_case.str() << where.str();
        if (mysql_query(ConnPtr, query_s.str().c_str()) != 0) {
            std::cerr << "(SyncConsumables) CONSUMABLE UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "(SyncConsumables) Failed to Sync userPk : " << userPk_ << " Consumables (MySQL or Unknown Error)" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized Consumables with MySQL" << std::endl;
    return true;
}

bool MySQLManager::SyncMaterials(uint16_t userPk_, std::vector<MATERIALS> userMat_) {
    try {
        std::ostringstream query_s;
        query_s << "UPDATE Materials SET ";

        std::ostringstream item_code_case, count_case, where;
        item_code_case << "Item_code = CASE ";
        count_case << "count = CASE ";

        where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

        bool first = true;
        for (auto& tempMat : userMat_) {

            item_code_case << "WHEN position = " << tempMat.position << " THEN " << tempMat.itemCode << " ";
            count_case << "WHEN position = " << tempMat.position << " THEN " << tempMat.count << " ";

            if (!first) where << ", ";
            where << tempMat.position;
            first = false;
        }

        item_code_case << "END, ";
        count_case << "END ";
        where << ");";

        query_s << item_code_case.str() << count_case.str() << where.str();
        if (mysql_query(ConnPtr, query_s.str().c_str()) != 0) {
            std::cerr << "(SyncMaterials) MATERIALS UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "(SyncMaterials) Failed to Sync userPk : " << userPk_ << " Materials (MySQL or Unknown Error)" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized Materials with MySQL" << std::endl;
    return true;
}

bool MySQLManager::MySQLSyncEqipmentEnhace(uint16_t userPk_, uint16_t itemPosition_, uint16_t enhancement_) {
    try {
        MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);

        std::string query = "UPDATE Equipment SET position = ?, enhance = ? WHERE user_pk = ?;";
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
            std::cerr << "(MySQLSyncEquipmentEnhance) Equipment Enhance Sync Prepare Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        MYSQL_BIND bind[3];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = &itemPosition_;

        bind[1].buffer_type = MYSQL_TYPE_LONG;
        bind[1].buffer = &enhancement_;

        bind[2].buffer_type = MYSQL_TYPE_LONG;
        bind[2].buffer = &userPk_;

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

bool MySQLManager::MySQLSyncUserRaidScore(uint16_t userPk_, unsigned int userScore_, std::string userId_) {
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
