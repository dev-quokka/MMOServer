#pragma once
#pragma comment (lib, "libmysql.lib")

#include <iostream>
#include <string>
#include <mysql.h>
#include <vector>
#include <string>

#include "UserSyncData.h"

class MySQLManager {
public:
	~MySQLManager(){
		mysql_close(ConnPtr);
		std::cout << "MySQL End" << std::endl;
	}

	// ====================== INITIALIZATION =======================
	bool init();
	bool GetEventPassId(std::unordered_map<std::string, PASSINFO>& eventPassInfoMap_); // 현재 진행중인 패스 아이디 가져오기
	std::vector<RANKING> SetRankingInRedis();


	// =================== USER LOGIN MANAGEMENT ===================
	bool GetUserInfoById(std::string& userId_, std::pair<uint32_t, LOGIN_USERINFO>& ulp_);
	bool GetUserEquipByPk(std::string& userPk_, std::vector<EQUIPMENT>& eq_);
	bool GetUserConsumablesByPk(std::string& userPk_, std::vector<CONSUMABLES>& cs_);
	bool GetUserMaterialsByPk(std::string& userPk_, std::vector<MATERIALS>& mt_);
	bool GetPassInfoByPk(std::string& userPk_, std::unordered_map<std::string, PASSINFO>& eventPassInfoMap_, std::vector<USERPASSINFO>& pi_);
	bool GetPassDataByPk(std::string& userPk_, std::unordered_map<std::string, PASSINFO>& eventPassInfoMap_, std::vector<PASSREWARDNFO>& pri_);

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