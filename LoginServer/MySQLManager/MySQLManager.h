#pragma once
#pragma comment (lib, "libmysql.lib")

#include <iostream>
#include <cstdint>
#include <string>
#include <mysql.h>
#include <sw/redis++/redis++.h>

#include "LoginUserData.h"

class MySQLManager {
public:
	~MySQLManager(){
		mysql_close(ConnPtr);
		std::cout << "MySQL End" << std::endl;
	}

	bool init(std::shared_ptr<sw::redis::RedisCluster> redis_);
	bool SetRankingInRedis();

	std::pair<uint32_t, USERINFO> GetUserInfoById(std::string userId_);
	std::pair<uint16_t, char*> GetUserEquipByPk(std::string userPk_);
	std::pair<uint16_t, char*> GetUserConsumablesByPk(std::string userPk_);
	std::pair<uint16_t, char*> GetUserMaterialsByPk(std::string userPk_);

private:
	MYSQL Conn;
	MYSQL* ConnPtr = NULL;

	std::shared_ptr<sw::redis::RedisCluster> redis;

	MYSQL_RES* Result;
	MYSQL_ROW Row;

	int MysqlResult;
};