#pragma once
#include <vector>

#include "Channel.h"

constexpr int MAX_CHANNEL = 4; // Maximum number of channels + 1 (0 is not used)
constexpr int MAX_CHANNEL_USERS = 10; // Maximum number of users per channel

class ChannelManager {
public:
	// ====================== INITIALIZATION ======================
	bool init();


	// ==================== CHANNEL MANAGEMENT ====================
	bool InsertChannel(uint16_t channelNum, uint16_t userObjNum_, InGameUser* user_);
	void LeaveChannel(uint16_t channelNum, uint16_t userObjNum_);
	std::vector<uint16_t> GetChannels();

private:
	std::vector<Channel*> channels; // IDX - 1 ~ : Channel Number
};