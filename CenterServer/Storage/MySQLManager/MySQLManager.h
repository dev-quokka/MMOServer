#pragma once
#pragma comment (lib, "libmysql.lib")

#include <iostream>
#include <cstdint>
#include <string>
#include <mysql.h>
#include <sstream>
#include <vector>

#include "UserSyncData.h"

class MySQLManager {
public:
	~MySQLManager() {
		mysql_close(ConnPtr);
		std::cout << "MySQL End" << std::endl;
	}

	// ====================== INITIALIZATION =======================
	bool init();


	// ======================= SYNCRONIZATION =======================
	bool LogoutSync(uint16_t userPk_, USERINFO userInfo_, std::vector<EQUIPMENT> userEquip_, std::vector<CONSUMABLES> userConsum_, std::vector<MATERIALS> userMat_);

	bool SyncUserInfo(uint16_t userPk_, USERINFO userInfo_);
	bool SyncEquipment(uint16_t userPk_, std::vector<EQUIPMENT> userEquip_);
	bool SyncConsumables(uint16_t userPk_, std::vector<CONSUMABLES> userConsum_);
	bool SyncMaterials(uint16_t userPk_, std::vector<MATERIALS> userMat_);
	bool SyncUserRaidScore(uint16_t userPk_, unsigned int userScore_, std::string userId_);

private:
	MYSQL Conn;
	MYSQL* ConnPtr = NULL;
	MYSQL_RES* Result;
	MYSQL_ROW Row;

	int MysqlResult;
};