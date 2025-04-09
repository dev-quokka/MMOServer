#include "ChannelServersManager.h"

bool ChannelServersManager::init() {
	servers.resize(3); // Ã¤³Î¼ö + 1
	servers[0] = nullptr;

	for (int i = 1; i < 3; i++) {
		servers[i] = new ChannelServer;
	}

	return true;
}

bool ChannelServersManager::EnterChannelServer(uint16_t channelNum_) {
	return servers[channelNum_]->InsertUser();
}

void ChannelServersManager::LeaveChannelServer(uint16_t channelNum_) {
	servers[channelNum_]->RemoveUser();
}

std::vector<uint16_t> ChannelServersManager::GetServerCounts() {
	std::vector<uint16_t> k;
	for (int i = 1; i < servers.size(); i++) {
		k.emplace_back(servers[i]->GetUserCount());
	}
	return k;
}