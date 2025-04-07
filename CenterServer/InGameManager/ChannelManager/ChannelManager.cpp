#include "ChannelManager.h"

bool ChannelManager::init() {
	channelVector.resize(7); // 채널수 + 1
	for (auto& c : channelVector) c.store(0);
	return true;
}

void ChannelManager::EnterChannelServer(uint16_t channelNum_) {
	if (!channelVector[channelNum_].fetch_add(1) + 1 < 51) { // 1명 증가한게 서버 인원 초과면 -1
		channelVector[channelNum_].fetch_sub(1);
	}
}

void ChannelManager::LeaveChannelServer(uint16_t channelNum_) {
	channelVector[channelNum_].fetch_sub(1);
}

std::vector<std::atomic<uint16_t>> ChannelManager::getChannelVector() {
	return channelVector;
}