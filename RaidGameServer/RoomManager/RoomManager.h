#pragma once

#include <set>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

#include "Room.h"

constexpr uint16_t TICK_RATE = 5; // Tick interval
constexpr int UDP_PORT = 50001; // Gameserver1 Udp Port
constexpr uint16_t MAX_ROOM = 10;

struct EndTimeComp { // Timeout checker for created game rooms
	bool operator()(Room* r1, Room* r2) const {
		return r1->GetEndTime() > r2->GetEndTime();
	}
};

class RoomManager {
public:
	~RoomManager() {
		timeChekcRun = false;
		if (timeCheckThread.joinable()) {
			timeCheckThread.join();
		}

		tickRateRun = false;
		if (tickRateThread.joinable()) {
			tickRateThread.join();
		}
	}

	bool init();
	bool CreateTimeCheckThread();
	bool CreateTickRateThread();
	void TimeCheckThread();
	void TickRateThread();

	bool MakeRoom(uint16_t roomNum_, uint16_t mapNum_, uint16_t timer_, int mobHp_, RaidUserInfo* raidUserInfo1, RaidUserInfo* raidUserInfo2);
	Room* GetRoom(uint16_t roomNum_);
	void DeleteRoom(uint16_t roomNum_);

	void DeleteMob(Room* room_);

private:
	// 80 bytes
	std::mutex mDeleteRoom;
	std::mutex mDeleteMatch;

	std::unordered_map<uint16_t, Room*> roomMap;

	// 24 bytes
	std::set<Room*, EndTimeComp> endRoomCheckSet;

	// 16 bytes
	std::thread timeCheckThread;
	std::thread tickRateThread;

	// 8 bytes
	SOCKET rgsSkt;
	SOCKET udpSkt;
	HANDLE rgsIOCPHandle;

	// 1 bytes
	std::atomic<bool> timeChekcRun;
	std::atomic<bool> tickRateRun;
};