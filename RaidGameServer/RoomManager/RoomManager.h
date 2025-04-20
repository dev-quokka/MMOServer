#pragma once
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <set>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

#include "RaidConfig.h"
#include "Room.h"

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

	struct EndTimeComp { // Timeout checker for created game rooms
		bool operator()(Room* r1, Room* r2) const {
			return r1->GetEndTime() > r2->GetEndTime();
		}
	};

	// ====================== INITIALIZATION =======================
	bool init();

	// ===================== THREAD MANAGEMENT =====================
	bool CreateTimeCheckThread();
	bool CreateTickRateThread();
	void TimeCheckThread();
	void TickRateThread();

	// ===================== ROOM MANAGEMENT =======================
	void MakeRoom(uint16_t roomNum_);
	void InsertEndCheckSet(Room* tempRoom_);
	Room* GetRoom(uint16_t roomNum_);
	void DeleteRoom(uint16_t roomNum_);

	// ====================== RAID CLEANUP =========================
	void DeleteMob(Room* room_);

private:
	// 80 bytes
	std::mutex mDeleteRoom;
	std::mutex mDeleteMatch;

	tbb::concurrent_hash_map<uint16_t, Room*> roomMap;

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