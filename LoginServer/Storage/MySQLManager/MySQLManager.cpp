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
		std::cerr << "(MySQL) Query Failed : " << mysql_error(ConnPtr) << std::endl;
		return tempRanks;
	}

	Result = mysql_store_result(ConnPtr);
	if (Result == nullptr) {
		std::cerr << "(MySQL) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
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
	catch (...) { // MySQL or Unknown Error
		std::cerr << "(MySQL or Unknown Error) Ranking load failed" << std::endl;
		tempRanks.clear();
		return tempRanks;
	}

	return tempRanks;
}


// ======================= USER LOGIN MANAGEMENT =======================

std::pair<uint32_t, LOGIN_USERINFO> MySQLManager::GetUserInfoById(std::string userId_) {
	
	std::string query_s =
		"SELECT u.id, u.level, u.exp, u.last_login, r.score From Users u "
		"left join Ranking r on u.name = r.name WHERE u.name = '" + userId_ + "'";

	uint32_t pk_;
	LOGIN_USERINFO userInfo;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "(MySQL) Query Failed: " << mysql_error(ConnPtr) << std::endl;
		return { 0, userInfo };
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(MySQL) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
			return { 0, userInfo };
		}

		while ((Row = mysql_fetch_row(Result)) != NULL) {
			pk_ = (uint32_t)std::stoi(Row[0]);
			userInfo.level = (uint16_t)std::stoi(Row[1]);
			userInfo.exp = (unsigned int)std::stoi(Row[2]);
			userInfo.lastLogin = std::string(Row[3]);
			userInfo.raidScore = (unsigned int)std::stoi(Row[4]);
		}

		mysql_free_result(Result);
		return { pk_, userInfo };
	}
	catch (...) { // MySQL or Unknown Error
		std::cerr << "(MySQL or Unknown Error) Failed to Load UserInfo" << std::endl;

		return { 0, userInfo };
	}
}

std::vector<EQUIPMENT> MySQLManager::GetUserEquipByPk(std::string userPk_) {
	
	std::string query_s = "SELECT item_code, position, enhance "
		"FROM Equipment WHERE user_pk = " + userPk_;

	std::vector<EQUIPMENT> tempEq;

	const char* Query = query_s.c_str();

	if (mysql_query(ConnPtr, Query) != 0) {
		std::cerr << "(MySQL) Query Failed: " << mysql_error(ConnPtr) << std::endl;
		return tempEq ;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(MySQL) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
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
	catch (...) { // MySQL or Unknown Error
		std::cerr << "(MySQL or Unknown Error) Failed to Load Equipment" << std::endl;
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
		std::cerr << "(MySQL) Query Failed: " << mysql_error(ConnPtr) << std::endl;
		return tempCs;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(MySQL) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
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
	catch (...) { // MySQL or Unknown Error
		std::cerr << "(MySQL or Unknown Error) Failed to Load Consumables" << std::endl;
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
		std::cerr << "(MySQL) Query Failed: " << mysql_error(ConnPtr) << std::endl;
		return tempMt;
	}

	try {
		Result = mysql_store_result(ConnPtr);
		if (Result == nullptr) {
			std::cerr << "(MySQL) Failed to store result : " << mysql_error(ConnPtr) << std::endl;
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
	catch (...) { // MySQL or Unknown Error
		std::cerr << "(MySQL or Unknown Error) Failed to Load Materials" << std::endl;
		tempMt.clear();
		return tempMt;
	}
}
