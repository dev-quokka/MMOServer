#include "ChannelManager.h"

// ========================== INITIALIZATION ==========================

bool ChannelManager::init() {
	channels.resize(MAX_CHANNEL);
	channels[0] = nullptr;

	for (int i = 1; i < MAX_CHANNEL; ++i) {
		channels[i] = new Channel();

	}
	return true;
}


// ======================== CHANNEL MANAGEMENT ========================

bool ChannelManager::InsertChannel(uint16_t channelNum, uint16_t userObjNum_, InGameUser* user_) {
	if (channels[channelNum]->GetUserCount() > MAX_CHANNEL_USERS) { // Channel is full, return fail
		return false;
	}

	channels[channelNum]->InsertUser(userObjNum_, user_);
	return true;
}

void ChannelManager::LeaveChannel(uint16_t channelNum, uint16_t userObjNum_) { // Decrease the user count in the channel if the user is in a channel
	channels[channelNum]->RemoveUser(userObjNum_);
}

std::vector<uint16_t> ChannelManager::GetChannels() {
	std::vector<uint16_t> k(MAX_CHANNEL, 0);

	for (int i = 1; i < channels.size(); i++) {
		k[i] = channels[i]->GetUserCount();
	}

	return k;
}