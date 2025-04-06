#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <thread>
#include <sw/redis++/redis++.h>

struct ChannelUser {

};

class ChannelManager {
public:
	~ChannelManager() {

	}

private:
	// 80 bytes
	std::unordered_map<ServerType, std::vector<uint16_t>> channelMap; // key : 서버명, value: 각 채널의 인원 수를 저장하는 vector

};
