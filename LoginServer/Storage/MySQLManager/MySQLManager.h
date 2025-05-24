#pragma once
#pragma comment (lib, "libmysql.lib")

#include <iostream>
#include <string>
#include <mysql.h>
#include <vector>

#include "UserSyncData.h"

class MySQLManager {
public:
	~MySQLManager(){
		mysql_close(ConnPtr);
		std::cout << "MySQL End" << std::endl;
	}

	// ====================== INITIALIZATION =======================
	bool init();
	std::vector<RANKING> SetRankingInRedis();


	// =================== USER LOGIN MANAGEMENT ===================
	std::pair<uint32_t, LOGIN_USERINFO> GetUserInfoById(std::string userId_);
	std::vector<EQUIPMENT> GetUserEquipByPk(std::string userPk_);
	std::vector<CONSUMABLES> GetUserConsumablesByPk(std::string userPk_);
	std::vector<MATERIALS> GetUserMaterialsByPk(std::string userPk_);

private:
	// 1096 bytes
	MYSQL Conn;

	// 8 bytes
	MYSQL_ROW Row;

	MYSQL* ConnPtr = NULL;
	MYSQL_RES* Result;

	// 4 bytes
	int MysqlResult;
};