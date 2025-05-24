#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <thread>

#include "ChannelServer.h"

class ChannelServersManager{
public:
	// =================== INITIALIZATION  ===================
	bool init();


	// ======== CHANNEL SERVER USER COUNT MANAGEMENT ========
	bool EnterChannelServer(uint16_t channelNum_);
	void LeaveChannelServer(uint16_t channelNum_);


	// ================ CHANNEL SERVER STATUS ================
	std::vector<uint16_t> GetServerCounts() const;

private:
	std::vector<ChannelServer*> servers;
};
