#include "MySQLManager.h"

// ====================== INITIALIZATION =======================

bool MySQLManager::init() {
    mysql_init(&Conn);

    ConnPtr = mysql_real_connect(&Conn, "127.0.0.1", "quokka", "1234", "iocp", 3306, (char*)NULL, 0);
    if (ConnPtr == NULL) {
        std::cout << mysql_error(&Conn) << std::endl;
        std::cout << "Mysql Connect Fail" << std::endl;
        return false;
    }

    std::cout << "Mysql Connect Success" << std::endl;
    return true;
}


// ======================= SYNCRONIZATION =======================

bool MySQLManager::LogoutSync(uint16_t userPk_, USERINFO userInfo_, std::vector<EQUIPMENT> userEquip_, std::vector<CONSUMABLES> userConsum_, std::vector<MATERIALS> userMat_) {
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
            std::cerr << "MySQL UPDATE Error: " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (...) { // MySQL or Unknown Error
        std::cerr << "Failed to Sync userPk : " << userPk_ << " UserInfo Data (MySQL or Unknown Error)" << std::endl;
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
            std::cerr << "MySQL Batch UPDATE Error: " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (...) { // MySQL or Unknown Error
        std::cerr << "Failed to Sync userPk : " << userPk_ << " Equipments (MySQL or Unknown Error)" << std::endl;
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
            std::cerr << "CONSUMABLE UPDATE Error: " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (...) { // MySQL or Unknown Error
        std::cerr << "Failed to Sync userPk : " << userPk_ << " Consumables (MySQL or Unknown Error)" << std::endl;
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
            std::cerr << "MATERIALS UPDATE Error: " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (...) { // MySQL or Unknown Error
        std::cerr << "Failed to Sync userPk : " << userPk_ << " Materials (MySQL or Unknown Error)" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized Materials with MySQL" << std::endl;
    return true;
}

bool MySQLManager::SyncUserRaidScore(uint16_t userPk_, unsigned int userScore_, std::string userId_) {
    try {
        std::string query_s = "UPDATE Raking set score = "
            + std::to_string(userScore_) + ", where id = " + userId_;

        const char* Query = query_s.c_str();

        MysqlResult = mysql_query(ConnPtr, Query);
        if (MysqlResult != 0) {
            std::cerr << "MySQL UPDATE Error: " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

    }
    catch (...) { // MySQL or Unknown Error
        std::cerr << "Failed to Sync userPk : " << userPk_ << " Raid Score : " << userScore_ << "(MySQL or Unknown Error)" << std::endl;
        return false;
    }

    return true;
}