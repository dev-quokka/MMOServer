#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <thread>

#include "Room.h"

constexpr uint16_t TICK_RATE = 5; // 1초에 몇번씩 보낼건지

class InGameUser;
class UdpOverLappedManager;

class RoomManager {
public:
	RoomManager(SOCKET* udpSkt_, UdpOverLappedManager* udpOverLappedManager_) : udpSkt(udpSkt_), udpOverLappedManager(udpOverLappedManager_) {}
	~RoomManager() {
		for (auto& iter : roomMap) {
			delete iter.second;
		}
	}

	bool Init();
	void CreateTickRateThread();
	void TickRateThread();
	bool DeleteRoom(uint16_t roomNum);
	Room* MakeRoom(uint16_t roomNum_, uint16_t timer_, unsigned int mobHp_, uint16_t userObjNum1_, uint16_t userObjNum2_, InGameUser* user1_, InGameUser* user2_);
	Room* GetRoom(uint16_t roomNum_);

private:
	bool tickRateRun = false;
	SOCKET* udpSkt;
	UdpOverLappedManager* udpOverLappedManager;
	std::thread tickRateThread;
	std::unordered_map<uint16_t, Room*> roomMap; // { roomNum, Room }
};



// #include <tbb/concurrent_hash_map.h>
// 576 bytes
// tbb::concurrent_hash_map<uint16_t, Room*> roomMap; // {roomNum, Room}