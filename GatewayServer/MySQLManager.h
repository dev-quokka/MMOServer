#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <mysql.h>

#pragma comment (lib, "libmysql.lib") // mysql 연동

const uint16_t INVENTORY_SIZE = 11; // 10개면 +1해서 11개로 해두기

class MySQLManager {
public:
	~MySQLManager(){
		mysql_close(ConnPtr);
		std::cout << "MySQL End" << std::endl;
	}

	bool SetRankingInRedis() {
		std::string query_s = "SELECT score,name From Ranking";

		const char* Query = query_s.c_str();

		MysqlResult = mysql_query(ConnPtr, Query);

		auto pipe = redis->pipeline("ranking");

		if (MysqlResult == 0) {
			Result = mysql_store_result(ConnPtr);
			while ((Row = mysql_fetch_row(Result)) != NULL) {
				pipe.zadd("ranking", std::string(Row[1]), std::stod(Row[0]));
				pipe.exec();
			}
			mysql_free_result(Result);
		}

		pipe.exec();

		return true;
	}

	bool Run(std::shared_ptr<sw::redis::RedisCluster> redis_) {
		redis = redis_;

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

	bool SyncUserInfo(uint16_t userPk_) {
		std::string userInfokey = "userinfo:{" + std::to_string(userPk_) + "}";
		std::unordered_map<std::string, std::string> userData;
		redis->hgetall(userInfokey, std::inserter(userData, userData.begin()));

			std::string query_s = "UPDATE USERS left join Ranking r on USERS.name = r.name SET USERS.name = '"+ 
				userData["userId"] +"', USERS.exp = " + userData["exp"] +
			", USERS.level = " + userData["level"] +
			", USERS.last_login = current_timestamp, r.score =" + userData["raidScore"] +
			" WHERE USERS.id = " + std::to_string(userPk_);

		const char* Query = query_s.c_str();

		MysqlResult = mysql_query(ConnPtr, Query);

		if (MysqlResult != 0) {
			std::cerr << "MySQL UPDATE Error: " << mysql_error(ConnPtr) << std::endl;
			return false;
		}

		return true; 
	}

	bool SyncEquipment(uint16_t userPk_) {

		std::string userInfokey = "equipment:{" + std::to_string(userPk_) + "}";
		std::unordered_map<std::string, std::string> equipments;
		redis->hgetall(userInfokey, std::inserter(equipments, equipments.begin()));

		std::ostringstream query_s;
		query_s << "UPDATE Equipment SET ";

		std::ostringstream item_code_case, enhancement_case, where;
		item_code_case << "Item_code = CASE ";
		enhancement_case << "enhance = CASE ";

		where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

		bool first = true;
		for (auto& [key, value] : equipments) { // key = Pos, value = (code, enhance)
			if (value.empty()) {
				std::cerr << "Empty value in Redis for position: " << key << std::endl;
				continue;
			}
			size_t div = value.find(':');

			if (div == std::string::npos) {
				std::cerr << "Invalid data format in Redis for position: " << key << std::endl;
				continue;
			}

			int item_code = std::stoi(value.substr(0, div));
			int enhance = std::stoi(value.substr(div + 1));

			item_code_case << "WHEN position = " << key << " THEN " << item_code << " ";
			enhancement_case << "WHEN position = " << key << " THEN " << enhance << " ";

			if (!first) where << ", ";
			where << key;
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

		return true;
	}

	bool SyncConsumables(uint16_t userPk_) {

		std::string userInfokey = "consumables:{" + std::to_string(userPk_) + "}";
		std::unordered_map<std::string, std::string> consumables;
		redis->hgetall(userInfokey, std::inserter(consumables, consumables.begin()));

		std::ostringstream query_s;
		query_s << "UPDATE Consumables SET ";

		std::ostringstream item_code_case, count_case, where;
		item_code_case << "Item_code = CASE ";
		count_case << "count = CASE ";

		where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

		bool first = true;
		for (auto& [key, value] : consumables) { // key = Pos, value = (code, count)

			size_t div = value.find(':');

			int item_code = std::stoi(value.substr(0, div));
			int count = std::stoi(value.substr(div + 1));

			item_code_case << "WHEN position = " << key << " THEN " << item_code << " ";
			count_case << "WHEN position = " << key << " THEN " << count << " ";

			if (!first) where << ", ";
			where << key;
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

		return true;
	}

	bool SyncMaterials(uint16_t userPk_) {

		std::string userInfokey = "materials:{" + std::to_string(userPk_) + "}";
		std::unordered_map<std::string, std::string> Materials;
		redis->hgetall(userInfokey, std::inserter(Materials, Materials.begin()));

		std::ostringstream query_s;
		query_s << "UPDATE Materials SET ";

		std::ostringstream item_code_case, count_case, where;
		item_code_case << "Item_code = CASE ";
		count_case << "count = CASE ";

		where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

		bool first = true;
		for (auto& [key, value] : Materials) { // key = Pos, value = (code, count)

			size_t div = value.find(':');

			int item_code = std::stoi(value.substr(0, div));
			int count = std::stoi(value.substr(div + 1));

			item_code_case << "WHEN position = " << key << " THEN " << item_code << " ";
			count_case << "WHEN position = " << key << " THEN " << count << " ";

			if (!first) where << ", ";
			where << key;
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

		return true;
	}


	std::pair<uint32_t, USERINFO> GetUserInfoById(std::string userId_) {
		std::string query_s = 
		"SELECT u.id, u.level, u.exp, u.last_login, r.score From Users u "
		"left join Ranking r on u.name = r.name WHERE u.name = '" + userId_+"'";

		const char* Query = query_s.c_str();

		MysqlResult = mysql_query(ConnPtr, Query);

		USERINFO userInfo;
		uint32_t pk_;

		if (MysqlResult == 0) {
			Result = mysql_store_result(ConnPtr);
			while ((Row = mysql_fetch_row(Result)) != NULL) {
				pk_ = (uint32_t)std::stoi(Row[0]);
				userInfo.level = (uint16_t)std::stoi(Row[1]);
				userInfo.exp = (unsigned int)std::stoi(Row[2]);
				userInfo.raidScore = (unsigned int)std::stoi(Row[4]);

				std::string tag = "{" + std::to_string(pk_) + "}";
				std::string key = "userinfo:" + tag; // user:{pk}

				auto pipe = redis->pipeline(tag);

				pipe.hset(key, "level", Row[1])
					.hset(key, "exp", Row[2])
					.hset(key, "userId", userId_)
					.hset(key, "lastlogin", Row[3])
					.hset(key, "raidScore", Row[4])
					.expire(key, 3600);

				pipe.exec();
			}
			mysql_free_result(Result);
		}

		return { pk_, userInfo };
	}
	
	std::pair<uint16_t, char*> GetUserEquipByPk(std::string userPk_) {

		std::string query_s = "SELECT item_code, position, enhance "
		"FROM Equipment WHERE user_pk = " + userPk_;

		const char* Query = query_s.c_str();
		MysqlResult = mysql_query(ConnPtr, Query);

		// std::vector<EQUIPMENT> Equipments;

		char* tempC = new char[MAX_INVEN_SIZE+1];
		char* tc = tempC;
		uint16_t cnt = 0;

		std::string tag = "{" + userPk_ + "}";
		std::string key = "equipment:" + tag; // user:{pk}

		auto pipe = redis->pipeline(tag);

		if (MysqlResult == 0) {
			Result = mysql_store_result(ConnPtr);
			while ((Row = mysql_fetch_row(Result)) != NULL) {
				if ((uint16_t)std::stoi(Row[0]) == 0) continue; // 빈칸이면 넘어가기
				pipe.hset(key, Row[1], std::string(Row[0]) + ":" + std::string(Row[2]));

				EQUIPMENT equipment;
				equipment.itemCode = (uint16_t)std::stoi(Row[0]);
				equipment.position = (uint16_t)std::stoi(Row[1]);
				equipment.enhance = (uint16_t)std::stoi(Row[2]);

				memcpy(tc, (char*)&equipment, sizeof(EQUIPMENT));
				tc += sizeof(EQUIPMENT);
				cnt++;
				//Equipments.emplace_back(equipment);
			}
			pipe.exec();

			mysql_free_result(Result);
		}

		//return Equipments;

		return { cnt, tempC };
	}

	std::pair<uint16_t, char*> GetUserConsumablesByPk(std::string userPk_) {
		std::string query_s = "SELECT item_code, position, count "
		"FROM Consumables WHERE user_pk = " + userPk_;

		const char* Query = query_s.c_str();
		MysqlResult = mysql_query(ConnPtr, Query);

		std::string tag = "{" + userPk_ + "}";
		std::string key = "consumables:" + tag; // user:{pk}

		char* tempC = new char[MAX_INVEN_SIZE + 1];
		char* tc = tempC;
		uint16_t cnt = 0;

		auto pipe = redis->pipeline(tag);

		if (MysqlResult == 0) {
			Result = mysql_store_result(ConnPtr);
			while ((Row = mysql_fetch_row(Result)) != NULL) {
				if ((uint16_t)std::stoi(Row[0]) == 0) continue; // 빈칸이면 넘어가기
				pipe.hset(key, Row[1], std::string(Row[0]) + ":" + std::string(Row[2]));

				CONSUMABLES consumable;
				consumable.itemCode = (uint16_t)std::stoi(Row[0]);
				consumable.position = (uint16_t)std::stoi(Row[1]);
				consumable.count = (uint16_t)std::stoi(Row[2]);
				memcpy(tc, (char*)&consumable, sizeof(CONSUMABLES));
				tc += sizeof(CONSUMABLES);
				cnt++;
			}

			pipe.exec();
			mysql_free_result(Result);
		}

		return { cnt, tempC };
	}


	std::pair<uint16_t, char*> GetUserMaterialsByPk(std::string userPk_) {

		std::string query_s = "SELECT item_code, position, count "
		"FROM Materials WHERE user_pk = " + userPk_;

		const char* Query = query_s.c_str();
		MysqlResult = mysql_query(ConnPtr, Query);

		std::string tag = "{" + userPk_ + "}";
		std::string key = "materials:" + tag; // user:{pk}

		char* tempC = new char[MAX_INVEN_SIZE + 1];
		char* tc = tempC;
		uint16_t cnt = 0;

		auto pipe = redis->pipeline(tag);

		if (MysqlResult == 0) {
			Result = mysql_store_result(ConnPtr);
			while ((Row = mysql_fetch_row(Result)) != NULL) {
				if ((uint16_t)std::stoi(Row[0]) == 0) continue; // 빈칸이면 넘어가기
				pipe.hset(key, Row[1], std::string(Row[0]) + ":" + std::string(Row[2]));

				MATERIALS Material;
				Material.itemCode = (uint16_t)std::stoi(Row[0]);
				Material.position = (uint16_t)std::stoi(Row[1]);
				Material.count = (uint16_t)std::stoi(Row[2]);

				memcpy(tc, (char*)&Material, sizeof(MATERIALS));
				tc += sizeof(MATERIALS);
				cnt++;
			}

			pipe.exec();
			mysql_free_result(Result);
		}

		return { cnt, tempC };
	}

private:
	MYSQL Conn;
	MYSQL* ConnPtr = NULL;
	MYSQL_RES* Result;
	MYSQL_ROW Row;
	int MysqlResult;
	std::shared_ptr<sw::redis::RedisCluster> redis;
};