#include "ConnUsersManager.h"

// ================== CONNECTION USER MANAGEMENT ==================

void ConnUsersManager::InsertUser(uint16_t connObjNum_, ConnUser* connUser_) {
	ConnUsers[connObjNum_] = connUser_;
};

ConnUser* ConnUsersManager::FindUser(uint16_t connObjNum_) {
	return ConnUsers[connObjNum_];
};