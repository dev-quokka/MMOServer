#pragma once
#include <atomic>
#include <unordered_map>

class InGameUser;

class Channel {
public:
	void InsertUser(uint16_t userObjNum_, InGameUser* user_) {
		connectedUsers[userObjNum_] = user_;
		userCount.fetch_add(1);
	}

	void RemoveUser(uint16_t userObjNum_) {
		auto it = connectedUsers.find(userObjNum_);

		if (it != connectedUsers.end()) {
			connectedUsers.erase(it);
			userCount.fetch_sub(1);
		}

	}

	uint16_t GetUserCount() const {
		return userCount.load();
	}

private:
	// 80 bytes
	std::unordered_map<uint16_t, InGameUser*> connectedUsers;

	// 1 bytes
	std::atomic<uint16_t> userCount;
};