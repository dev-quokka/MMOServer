#include "ChannelManager.h"

bool ChannelManager::init() {
	channels.resize(MAX_CHANNEL);
	channels[0] = nullptr;// IDX 0은 사용하지 않음

	for (int i = 1; i < MAX_CHANNEL; ++i) {
		channels[i] = new Channel();

	}
	return true;
}

void ChannelManager::InsertChannel(uint16_t channelNum, uint16_t userObjNum_, InGameUser* user_) {
	channels[channelNum]->InsertUser(userObjNum_, user_);
}

void ChannelManager::LeaveChannel(uint16_t channelNum, uint16_t userObjNum_) {
	channels[channelNum]->RemoveUser(userObjNum_);
}