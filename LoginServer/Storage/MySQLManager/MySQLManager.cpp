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

bool MySQLManager::GetEventPassId(std::unordered_map<std::string, PASSINFO>& eventPassInfoMap_) {

	std::string query_s = "SELECT passId, eventStart, eventEnd, passMaxLevel FROM PassInfoData WHERE eventStart<= NOW() AND eventEnd > NOW()";

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "[GetEventPassId] Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}

	Result = mysql_store_result(ConnPtr);
	if (Result == nullptr) {
		std::cerr << "[GetEventPassId] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}

	try {
		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if (!Row[0] || !Row[1] || !Row[2] || !Row[3]) continue;

			PASSINFO passInfo;
			strncpy_s(passInfo.passId, Row[0], MAX_PASS_ID_LEN + 1);
			strncpy_s(passInfo.eventStart, Row[1], MAX_EVENT_LEN + 1);
			strncpy_s(passInfo.eventEnd, Row[2], MAX_EVENT_LEN + 1);
			passInfo.passMaxLevel = (uint16_t)Row[3];

			eventPassInfoMap_[Row[0]] = passInfo;
		}

		mysql_free_result(Result);
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "[GetEventPassId] PassId load failed" << std::endl;
		return false;
	}
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
	catch (const std::exception& e) {
		std::cerr << "(SetRankingInRedis) Ranking load failed" << std::endl;
		tempRanks.clear();
		return tempRanks;
	}

	return tempRanks;
}


// ======================= USER LOGIN MANAGEMENT =======================

bool MySQLManager::GetUserInfoById(std::string& userId_, std::pair<uint32_t, LOGIN_USERINFO>& ulp_) {

	MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);
	if (!stmt) {
		std::cerr << "(GetUserInfoById) Failed to init MySQL statement" << std::endl;
		return false;
	}

	std::string query =
		"SELECT u.id, u.level, u.exp, u.last_login, r.score, u.gold, u.cash, u.mileage "
		"FROM Users u LEFT JOIN Ranking r ON u.name = r.name "
		"WHERE u.name = ?";

	if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
		std::cerr << "(GetUserInfoById) Prepare failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return false;
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
		return false;
	}

	if (mysql_stmt_execute(stmt) != 0) {
		std::cerr << "(GetUserInfoById) Execution failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return false;
	}

	MYSQL_BIND bindResult[8];
	memset(bindResult, 0, sizeof(bindResult));

	unsigned long strLen = 0;
	char loginTimeBuffer[20] = { 0 }; // Timestamp string buffer (19 chars + 1)

	int tempPk = 0;
	int tempLevel = 0;
	unsigned int tempExp = 0;
	unsigned int tempScore = 0;
	int gold = 0;
	int cash = 0;
	int mileage = 0;

	bindResult[0].buffer_type = MYSQL_TYPE_LONG;
	bindResult[0].buffer = &tempPk;

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

	bindResult[5].buffer_type = MYSQL_TYPE_LONG;
	bindResult[5].buffer = &gold;

	bindResult[6].buffer_type = MYSQL_TYPE_LONG;
	bindResult[6].buffer = &cash;

	bindResult[7].buffer_type = MYSQL_TYPE_LONG;
	bindResult[7].buffer = &mileage;

	if (mysql_stmt_bind_result(stmt, bindResult) != 0) {
		std::cerr << "(GetUserInfoById) Result bind failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return false;
	}

	if (mysql_stmt_store_result(stmt) != 0) {
		std::cerr << "(GetUserInfoById) Store result failed : " << mysql_stmt_error(stmt) << std::endl;
		mysql_stmt_close(stmt);
		return false;
	}

	LOGIN_USERINFO userInfo;

	try {
		if (mysql_stmt_fetch(stmt) == 0) {
			userInfo.level = (uint16_t)tempLevel;
			userInfo.exp = (uint32_t)tempExp;
			userInfo.lastLogin = std::string(loginTimeBuffer, strLen);
			userInfo.raidScore = (uint32_t)tempScore;
			userInfo.gold = (int)gold;
			userInfo.cash = (int)cash;
			userInfo.mileage = (int)mileage;

			ulp_ = { (uint32_t)tempPk, userInfo };
		}
	}
	catch (const std::exception& e) {
		std::cerr << "(GetUserInfoById) Exception Error : " << e.what() << std::endl;
		mysql_stmt_close(stmt);
		return false;
	}

	mysql_stmt_free_result(stmt);
	mysql_stmt_close(stmt);

	return true;
}

bool MySQLManager::GetUserEquipByPk(std::string& userPk_, std::vector<EQUIPMENT>& eq_) {

	std::string query_s = "SELECT item_code, position, enhance "
		"FROM Equipment WHERE user_pk = " + userPk_;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "(GetUserEquipByPk) Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(GetUserEquipByPk) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return false;
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty

			EQUIPMENT equipment;
			equipment.itemCode = (uint16_t)std::stoi(Row[0]);
			equipment.position = (uint16_t)std::stoi(Row[1]);
			equipment.enhance = (uint16_t)std::stoi(Row[2]);

			eq_.emplace_back(equipment);
		}
		mysql_free_result(Result);
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "(GetUserEquipByPk) Exception Error : " << e.what() << std::endl;
		return false;
	}
}

bool MySQLManager::GetUserConsumablesByPk(std::string& userPk_, std::vector<CONSUMABLES>& cs_) {

	std::string query_s = "SELECT item_code, position, daysOrCount "
		"FROM Consumables WHERE user_pk = " + userPk_;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "(GetUserConsumablesByPk) Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(GetUserConsumablesByPk) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return false;
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty

			CONSUMABLES consumable;
			consumable.itemCode = (uint16_t)std::stoi(Row[0]);
			consumable.position = (uint16_t)std::stoi(Row[1]);
			consumable.count = (uint16_t)std::stoi(Row[2]);

			cs_.emplace_back(consumable);
		}

		mysql_free_result(Result);
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "(GetUserConsumablesByPk) Exception Error : " << e.what() << std::endl;
		return false;
	}
}

bool MySQLManager::GetUserMaterialsByPk(std::string& userPk_, std::vector<MATERIALS>& mt_) {

	std::string query_s = "SELECT item_code, position, daysOrCount "
		"FROM Materials WHERE user_pk = " + userPk_;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "[GetUserMaterialsByPk] Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "[GetUserMaterialsByPk] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return false;
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if ((uint16_t)std::stoi(Row[0]) == 0) continue; // Skip if the inventory slot is empty

			MATERIALS Material;
			Material.itemCode = (uint16_t)std::stoi(Row[0]);
			Material.position = (uint16_t)std::stoi(Row[1]);
			Material.count = (uint16_t)std::stoi(Row[2]);

			mt_.emplace_back(Material);
		}

		mysql_free_result(Result);
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "[GetUserMaterialsByPk] Exception Error : " << e.what() << std::endl;
		return false;
	}
}

bool MySQLManager::GetPassInfoByPk(std::string& userPk_, std::unordered_map<std::string, PASSINFO>& eventPassInfoMap_, std::vector<USERPASSINFO>& pi_) {

	std::string query_s = "SELECT passId, userPassLevel, userPassExp, passCurrencyType FROM PassUserData WHERE userPk = " + userPk_;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "[GetPassInfoByPk] Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "[GetPassInfoByPk] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return false;
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if (!Row[0] || !Row[1] || !Row[2] || !Row[3]) continue;

			USERPASSINFO userPassInfo;
			userPassInfo.passInfo = eventPassInfoMap_[Row[0]];

			userPassInfo.passLevel = (uint16_t)std::stoi(Row[1]);
			userPassInfo.passExp = (uint16_t)std::stoi(Row[2]);
			userPassInfo.passCurrencyType = (uint16_t)std::stoi(Row[3]);

			pi_.emplace_back(userPassInfo);
		}
		mysql_free_result(Result);
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "[GetPassInfoByPk] Exception Error : " << e.what() << std::endl;
		return false;
	}
}

bool MySQLManager::GetPassDataByPk(std::string& userPk_, std::unordered_map<std::string, PASSINFO>& eventPassInfoMap_, std::vector<PASSREWARDNFO>& pri_) {

	std::string query_s = "SELECT passId, rewardBits, passCurrencyType FROM PassUserRewardData WHERE userPk = " + userPk_;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "[GetPassDataByPk] Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return false;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "[GetPassDataByPk] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return false;
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			if (!Row[0] || !Row[1] || !Row[2]) continue;

			PASSREWARDNFO passRewordInfos;
			strncpy_s(passRewordInfos.passId, Row[0], MAX_PASS_ID_LEN + 1);
			passRewordInfos.passReqwardBits = (uint64_t)std::stoi(Row[1]);
			passRewordInfos.passCurrencyType = (uint16_t)std::stoi(Row[2]);

			pri_.emplace_back(passRewordInfos);
		}
		mysql_free_result(Result);
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "[GetPassDataByPk] Exception Error : " << e.what() << std::endl;
		return false;
	}
}