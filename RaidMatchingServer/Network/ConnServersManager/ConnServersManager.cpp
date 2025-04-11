#include "ConnServersManager.h"

void ConnServersManager::InsertUser(uint16_t connObjNum_, ConnServer* connServer_) {
	connServers[connObjNum_] = connServer_;
};

ConnServer* ConnServersManager::FindUser(uint16_t connObjNum_) {
	return connServers[connObjNum_];
};