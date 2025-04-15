#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <mysql.h>
#include <sw/redis++/redis++.h>

#include "LoginUserData.h"
#pragma comment (lib, "libmysql.lib") // mysql 연동

const uint16_t INVENTORY_SIZE = 11; // 10개면 +1해서 11개로 해두기

class MySQLManager {
public:
	~MySQLManager(){
		mysql_close(ConnPtr);
		std::cout << "MySQL End" << std::endl;
	}

	bool init(std::shared_ptr<sw::redis::RedisCluster> redis_);

	bool SyncUserInfo(uint16_t userPk_);
	bool SyncEquipment(uint16_t userPk_);
	bool SyncConsumables(uint16_t userPk_);
	bool SyncMaterials(uint16_t userPk_);

	std::pair<uint32_t, USERINFO> GetUserInfoById(std::string userId_);
	std::pair<uint16_t, char*> GetUserEquipByPk(std::string userPk_);
	std::pair<uint16_t, char*> GetUserConsumablesByPk(std::string userPk_);
	std::pair<uint16_t, char*> GetUserMaterialsByPk(std::string userPk_);

	bool SetRankingInRedis();

private:
	MYSQL Conn;
	MYSQL* ConnPtr = NULL;

	std::shared_ptr<sw::redis::RedisCluster> redis;

	MYSQL_RES* Result;
	MYSQL_ROW Row;

	int MysqlResult;
};