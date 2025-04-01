#include "RoomManager.h"

bool RoomManager::Init() {
	CreateTickRateThread();
	return true;
}

void RoomManager::CreateTickRateThread() {
	tickRateRun = true;
	tickRateThread = std::thread([this]() {TickRateThread(); });
	std::cout << "TickRateThread Start" << std::endl;
}

void RoomManager::TickRateThread() {
	while (tickRateRun) {
		auto tickRate = std::chrono::milliseconds(1000 / TICK_RATE);
		auto timeCheck = std::chrono::steady_clock::now() + tickRate;

		for (int i = 1; i <= roomVec.size(); i++) {

		}

		while (timeCheck > std::chrono::steady_clock::now()) { // 틱레이트 까지 대기
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

Room* RoomManager::MakeRoom(uint16_t roomNum_, uint16_t timer_, unsigned int mobHp_, uint16_t userObjNum1_, uint16_t userObjNum2_, InGameUser* user1_, InGameUser* user2_) {
	Room* room = new Room(udpSkt, udpOverLappedManager);
	room->set(roomNum_, timer_, mobHp_, userObjNum1_, userObjNum2_, user1_, user2_);
	roomMap[roomNum_] = room;
	return room;
	//roomMap.emplace(roomNum_, room); => 나중에 매칭 쓰레드 늘어나면 concurrent_hash_map 사용
}

Room* RoomManager::GetRoom(uint16_t roomNum_) {
	return roomMap[roomNum_];
}

bool RoomManager::DeleteRoom(uint16_t roomNum_) {
	Room* room = roomMap[roomNum_];
	delete room;
	roomMap.erase(roomNum_);
	return true;
}