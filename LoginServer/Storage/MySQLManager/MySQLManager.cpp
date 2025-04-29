#include "MySQLManager.h"

// ========================== INITIALIZATION ===========================

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

std::vector<RANKING> MySQLManager::SetRankingInRedis() {
	std::string query_s = "SELECT score,name From Ranking";
	std::vector<RANKING> tempRanks;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "(SetRankingInRedis) Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return tempRanks;
	}

	Result = mysql_store_result(ConnPtr);
	if (Result == nullptr) {
		std::cerr << "(SetRankingInRedis) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
		return tempRanks;
	}

	RANKING tempRank;

	try {
		while ((Row = mysql_fetch_row(Result)) != NULL) {
			strncpy_s(tempRank.userId, sizeof(tempRank.userId), Row[1], MAX_USER_ID_LEN + 1);
			tempRank.score = std::stod(Row[0]);

			tempRanks.emplace_back(tempRank);
		}
		mysql_free_result(Result);
	}
	catch (...) {
		std::cerr << "(SetRankingInRedis) Ranking load failed" << std::endl;
		tempRanks.clear();
		return tempRanks;
	}

	return tempRanks;
}


// ======================= USER LOGIN MANAGEMENT =======================

std::pair<uint32_t, LOGIN_USERINFO> MySQLManager::GetUserInfoById(std::string userId_) {
	uint32_t pk_ = 0;
	LOGIN_USERINFO userInfo;

	MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);
	if (!stmt) {
		std::cerr << "(GetUserInfoById) Failed to init MySQL statement" << std::endl;
		return { 0, userInfo };
	}

	std::string query =
		"SELECT u.id, u.level, u.exp, u.last_login, r.score "
		"FROM Users u LEFT JOIN Ranking r ON u.name = r.name "
		"WHERE u.name = ?";

	if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
		std::cerr << "(GetUserInfoById) Prepare failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return { 0, userInfo };
	}

	MYSQL_BIND bindParam[1];
	memset(bindParam, 0, sizeof(bindParam));

	unsigned long nameLength = userId_.length();

	bindParam[0].buffer_type = MYSQL_TYPE_STRING;
	bindParam[0].buffer = (void*)userId_.c_str();
	bindParam[0].buffer_length = nameLength;
	bindParam[0].length = &nameLength;

	if (mysql_stmt_bind_param(stmt, bindParam) != 0) {
		std::cerr << "(GetUserInfoById) Bind failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return { 0, userInfo };
	}

	if (mysql_stmt_execute(stmt) != 0) {
		std::cerr << "(GetUserInfoById) Execution failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return { 0, userInfo };
	}

	MYSQL_BIND bindResult[5];
	memset(bindResult, 0, sizeof(bindResult));

	unsigned long strLen = 0;
	char loginTimeBuffer[20] = { 0 }; // Timestamp string buffer (19 chars + 1)

	int tempId = 0;
	int tempLevel = 0;
	int tempExp = 0;
	int tempScore = 0;

	bindResult[0].buffer_type = MYSQL_TYPE_LONG;
	bindResult[0].buffer = &tempId;

	bindResult[1].buffer_type = MYSQL_TYPE_LONG;
	bindResult[1].buffer = &tempLevel;

	bindResult[2].buffer_type = MYSQL_TYPE_LONG;
	bindResult[2].buffer = &tempExp;

	bindResult[3].buffer_type = MYSQL_TYPE_STRING;
	bindResult[3].buffer = loginTimeBuffer;
	bindResult[3].buffer_length = sizeof(loginTimeBuffer);
	bindResult[3].length = &strLen;

	bindResult[4].buffer_type = MYSQL_TYPE_LONG;
	bindResult[4].buffer = &tempScore;

	if (mysql_stmt_bind_result(stmt, bindResult) != 0) {
		std::cerr << "(GetUserInfoById) Result bind failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return { 0, userInfo };
	}

	if (mysql_stmt_store_result(stmt) != 0) {
		std::cerr << "(GetUserInfoById) Store result failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return { 0, userInfo };
	}

	try {
		if (mysql_stmt_fetch(stmt) == 0) {
			pk_ = (uint32_t)tempId;
			userInfo.level = (uint16_t)tempLevel;
			userInfo.exp = (uint32_t)tempExp;
			userInfo.lastLogin = std::string(loginTimeBuffer, strLen);
			userInfo.raidScore = (uint32_t)tempScore;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "(GetUserInfoById) Exception Error : " << e.what() << std::endl;
		mysql_stmt_close(stmt);
		return { 0, userInfo };
	}

	mysql_stmt_free_result(stmt);
	mysql_stmt_close(stmt);

	return { pk_, userInfo };
}


std::vector<EQUIPMENT> MySQLManager::GetUserEquipByPk(std::string userPk_) {

	std::string query_s = "SELECT item_code, position, enhance "
		"FROM Equipment WHERE user_pk = " + userPk_;

	std::vector<EQUIPMENT> tempEq;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "(GetUserEquipByPk) Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return tempEq;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(GetUserEquipByPk) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return tempEq;
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty

			EQUIPMENT equipment;
			equipment.itemCode = (uint16_t)std::stoi(Row[0]);
			equipment.position = (uint16_t)std::stoi(Row[1]);
			equipment.enhance = (uint16_t)std::stoi(Row[2]);

			tempEq.emplace_back(equipment);
		}
		mysql_free_result(Result);
		return tempEq;
	}
	catch (const std::exception& e) {
		std::cerr << "(GetUserEquipByPk) Exception Error : " << e.what() << std::endl;
		tempEq.clear();
		return tempEq;
	}
}

std::vector<CONSUMABLES> MySQLManager::GetUserConsumablesByPk(std::string userPk_) {

	std::string query_s = "SELECT item_code, position, count "
		"FROM Consumables WHERE user_pk = " + userPk_;

	std::vector<CONSUMABLES> tempCs;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "(GetUserConsumablesByPk) Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return tempCs;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(GetUserConsumablesByPk) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return tempCs;
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty

			CONSUMABLES consumable;
			consumable.itemCode = (uint16_t)std::stoi(Row[0]);
			consumable.position = (uint16_t)std::stoi(Row[1]);
			consumable.count = (uint16_t)std::stoi(Row[2]);

			tempCs.emplace_back(consumable);
		}

		mysql_free_result(Result);
		return tempCs;
	}
	catch (const std::exception& e) {
		std::cerr << "(GetUserConsumablesByPk) Exception Error : " << e.what() << std::endl;
		tempCs.clear();
		return tempCs;
	}
}

std::vector<MATERIALS> MySQLManager::GetUserMaterialsByPk(std::string userPk_) {

	std::string query_s = "SELECT item_code, position, count "
		"FROM Materials WHERE user_pk = " + userPk_;

	std::vector<MATERIALS> tempMt;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "(GetUserMaterialsByPk) Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return tempMt;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(GetUserMaterialsByPk) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return tempMt;
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty

			MATERIALS Material;
			Material.itemCode = (uint16_t)std::stoi(Row[0]);
			Material.position = (uint16_t)std::stoi(Row[1]);
			Material.count = (uint16_t)std::stoi(Row[2]);

			tempMt.emplace_back(Material);
		}

		mysql_free_result(Result);
		return tempMt;
	}
	catch (const std::exception& e) {
		std::cerr << "(GetUserMaterialsByPk) Exception Error : " << e.what() << std::endl;
		tempMt.clear();
		return tempMt;
	}
}
