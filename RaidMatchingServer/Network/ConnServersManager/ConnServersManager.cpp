#include "ConnServersManager.h"

void ConnServersManager::InsertUser(uint16_t connObjNum_, ConnServer* connServer_) {
	connServers[connObjNum_] = connServer_;
};

ConnServer* ConnServersManager::FindUser(uint16_t connObjNum_) {
	return connServers[connObjNum_];
};

bool ConnServersManager::CheckGameServerObjNum(uint16_t idx_) {
	if (gameServerObjNums[idx_] != 0) return false; // 이미 서버 설정 되어 있으면 false
	return true;
}

void ConnServersManager::SetGameServerObjNum(uint16_t idx_, uint16_t gameServerObjNums_) {
	gameServerObjNums[idx_] = gameServerObjNums_;
}

ConnServer* ConnServersManager::GetGameServerObjNum(uint16_t idx_) {
	return connServers[gameServerObjNums[idx_]];
}