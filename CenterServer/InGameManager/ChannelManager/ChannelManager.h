#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <thread>
#include <sw/redis++/redis++.h>

#include "Room.h"

class InGameUser;

class ChannelManager {
public:
	~ChannelManager() {
		channelUserCountSyncThreadRun = false;
		if (channelUserCountSyncThread.joinable()) {
			channelUserCountSyncThread.join();
		}
	}

private:
	// 80 bytes
	std::unordered_map<uint16_t, Room*> channelMap;

	// 16 byte
	std::thread channelUserCountSyncThread;

	// 1 byte
	std::atomic<bool> channelUserCountSyncThreadRun = false;
};
