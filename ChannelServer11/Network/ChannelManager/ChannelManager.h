#pragma once
#include <vector>

#include "Channel.h"

constexpr int MAX_CHANNEL = 4; // 최대 채널 수 + 1

class ChannelManager {
public:
	bool init();
	void InsertChannel(uint16_t channelNum, uint16_t userObjNum_, InGameUser* user_);
	void LeaveChannel(uint16_t channelNum, uint16_t userObjNum_);

private:
	std::vector<Channel*> channels; // IDX 1부터 1채널
};