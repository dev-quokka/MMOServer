#include "MySQLManager.h"

bool MySQLManager::init(std::shared_ptr<sw::redis::RedisCluster> redis_) {
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

bool MySQLManager::SetRankingInRedis() {
	std::string query_s = "SELECT score,name From Ranking";

	const char* Query = query_s.c_str();

	MysqlResult = mysql_query(ConnPtr, Query);
	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "Query Failed(MySQL) : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}

	Result = mysql_store_result(ConnPtr);
	if (Result == nullptr) {
		std::cerr << "Failed to store result(MySQL) : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}
	auto pipe = redis->pipeline("ranking");

	if (MysqlResult == 0) {
		try {
			Result = mysql_store_result(ConnPtr);
			while ((Row = mysql_fetch_row(Result)) != NULL) {
				pipe.zadd("ranking", std::string(Row[1]), std::stod(Row[0]));
				pipe.exec();
			}
			mysql_free_result(Result);

			pipe.exec();
		}
		catch (const sw::redis::Error& e) { // Redis Error
			std::cerr << "Redis error : " << e.what() << std::endl;
			return false;
		}
		catch (...) { // MySQL or Unknown Error
			std::cerr << "Ranking load failed(MySQL or Unknown Error)" << std::endl;
			return false;
		}

		return true;
	}

	return false;
}


std::pair<uint32_t, USERINFO> MySQLManager::GetUserInfoById(std::string userId_) {
	std::string query_s =
		"SELECT u.id, u.level, u.exp, u.last_login, u.server, u.channel, r.score From Users u "
		"left join Ranking r on u.name = r.name WHERE u.name = '" + userId_ + "'";

	const char* Query = query_s.c_str();

	USERINFO userInfo;
	uint32_t pk_;

	MysqlResult = mysql_query(ConnPtr, Query);
	if (MysqlResult != 0) {
		std::cerr << "[MySQL ERROR] Query Failed: " << mysql_error(ConnPtr) << std::endl;
		return { 100, userInfo };
	}

	if (MysqlResult == 0) {
		try {
			Result = mysql_store_result(ConnPtr);
			if (Result == nullptr) {
				std::cerr << "Failed to store result(MySQL) : " << mysql_error(ConnPtr) << std::endl;
				return { 100, userInfo };
			}

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
					.hset(key, "server", Row[4])
					.hset(key, "channel", Row[5])
					.hset(key, "raidScore", Row[6])
					.hset(key, "status", "Online")
					.expire(key, 3600);

				mysql_free_result(Result);
				pipe.exec();
			}
		}
		catch (const sw::redis::Error& e) { // Redis Error
			std::cerr << "Redis error : " << e.what() << std::endl;
			return { 100, userInfo };
		}
		catch (...) { // MySQL or Unknown Error
			std::cerr << "Failed to Load Equipment(MySQL or Unknown Error)" << std::endl;
			return { 100, userInfo };
		}

		return { pk_, userInfo };
	}

	return { 100, userInfo };
}

std::pair<uint16_t, char*> MySQLManager::GetUserEquipByPk(std::string userPk_) {

	std::string query_s = "SELECT item_code, position, enhance "
		"FROM Equipment WHERE user_pk = " + userPk_;

	const char* Query = query_s.c_str();

	MysqlResult = mysql_query(ConnPtr, Query);
	if (MysqlResult != 0) {
		std::cerr << "[MySQL ERROR] Query Failed: " << mysql_error(ConnPtr) << std::endl;
		return { 100, nullptr };
	}

	char* tempC = new char[MAX_INVEN_SIZE + 1];
	char* tc = tempC;
	uint16_t cnt = 0;

	std::string tag = "{" + userPk_ + "}";
	std::string key = "equipment:" + tag; // user:{pk}

	auto pipe = redis->pipeline(tag);

	if (MysqlResult == 0) {
		try {
			Result = mysql_store_result(ConnPtr);
			if (Result == nullptr) {
				std::cerr << "Failed to store result(MySQL) : " << mysql_error(ConnPtr) << std::endl;
				return { 100, nullptr };
			}

			while ((Row = mysql_fetch_row(Result)) != NULL) {
				if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty
				pipe.hset(key, Row[1], std::string(Row[0]) + ":" + std::string(Row[2]));

				EQUIPMENT equipment;
				equipment.itemCode = (uint16_t)std::stoi(Row[0]);
				equipment.position = (uint16_t)std::stoi(Row[1]);
				equipment.enhance = (uint16_t)std::stoi(Row[2]);

				memcpy(tc, (char*)&equipment, sizeof(EQUIPMENT));
				tc += sizeof(EQUIPMENT);
				cnt++;
			}

			pipe.exec();
			mysql_free_result(Result);
		}
		catch (const sw::redis::Error& e) { // Redis Error
			std::cerr << "Redis error : " << e.what() << std::endl;
			delete[] tempC;
			return { 100, nullptr };
		}
		catch (...) { // MySQL or Unknown Error
			std::cerr << "Failed to Load Equipment(MySQL or Unknown Error)" << std::endl;
			delete[] tempC;
			return { 100, nullptr };
		}

		delete[] tempC;
		return { cnt, tempC };
	}

	return { 100, nullptr };
}

std::pair<uint16_t, char*> MySQLManager::GetUserConsumablesByPk(std::string userPk_) {
	std::string query_s = "SELECT item_code, position, count "
		"FROM Consumables WHERE user_pk = " + userPk_;

	const char* Query = query_s.c_str();

	MysqlResult = mysql_query(ConnPtr, Query);
	if (MysqlResult != 0) {
		std::cerr << "[MySQL ERROR] Query Failed: " << mysql_error(ConnPtr) << std::endl;
		return { 100, nullptr };
	}

	std::string tag = "{" + userPk_ + "}";
	std::string key = "consumables:" + tag; // user:{pk}

	char* tempC = new char[MAX_INVEN_SIZE + 1];
	char* tc = tempC;
	uint16_t cnt = 0;

	auto pipe = redis->pipeline(tag);

	if (MysqlResult == 0) {
		try {
			Result = mysql_store_result(ConnPtr);
			if (Result == nullptr) {
				std::cerr << "Failed to store result(MySQL) : " << mysql_error(ConnPtr) << std::endl;
				return { 100, nullptr };
			}

			while ((Row = mysql_fetch_row(Result)) != NULL) {
				if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty
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
		catch (const sw::redis::Error& e) { // Redis Error
			std::cerr << "Redis error : " << e.what() << std::endl;
			delete[] tempC;
			return { 100, nullptr };
		}
		catch (...) { // MySQL or Unknown Error
			std::cerr << "Failed to Load Equipment(MySQL or Unknown Error)" << std::endl;
			delete[] tempC;
			return { 100, nullptr };
		}

		delete[] tempC;
		return { cnt, tempC };
	}

	return { 100, nullptr };
}


std::pair<uint16_t, char*> MySQLManager::GetUserMaterialsByPk(std::string userPk_) {

	std::string query_s = "SELECT item_code, position, count "
		"FROM Materials WHERE user_pk = " + userPk_;

	const char* Query = query_s.c_str();

	MysqlResult = mysql_query(ConnPtr, Query);
	if (MysqlResult != 0) {
		std::cerr << "[MySQL ERROR] Query Failed: " << mysql_error(ConnPtr) << std::endl;
		return { 100, nullptr };
	}

	std::string tag = "{" + userPk_ + "}";
	std::string key = "materials:" + tag; // user:{pk}

	char* tempC = new char[MAX_INVEN_SIZE + 1];
	char* tc = tempC;
	uint16_t cnt = 0;

	auto pipe = redis->pipeline(tag);

	if (MysqlResult == 0) {

		try {
			Result = mysql_store_result(ConnPtr);
			if (Result == nullptr) {
				std::cerr << "Failed to store result(MySQL) : " << mysql_error(ConnPtr) << std::endl;
				return { 100, nullptr };
			}

			while ((Row = mysql_fetch_row(Result)) != NULL) {
				if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty
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
		catch (const sw::redis::Error& e) { // Redis Error
			std::cerr << "Redis error : " << e.what() << std::endl;
			delete[] tempC;
			return { 100, nullptr };
		}
		catch (...) { // MySQL or Unknown Error
			std::cerr << "Failed to Load Equipment(MySQL or Unknown Error)" << std::endl;
			delete[] tempC;
			return { 100, nullptr };
		}

		delete[] tempC;
		return { cnt, tempC };
	}

	return { 100, nullptr };
}
