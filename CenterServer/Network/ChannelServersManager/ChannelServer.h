#pragma once

class ChannelServer {
public:

	// ================ CHANNEL SERVER USER COUNT MANAGEMENT ================

	bool InsertUser() { // Atomically increment user count, return false if over the maximum user count (30 users)
		if (userCnt.fetch_add(1) + 1 > 30) {
			userCnt.fetch_sub(1);
			return false;
		}
		return true;
	}

	void RemoveUser() {
		userCnt.fetch_sub(1);
	}

	uint16_t GetUserCount() const {
		return userCnt.load();
	}


private:
	std::atomic<uint16_t> userCnt = 0;
};