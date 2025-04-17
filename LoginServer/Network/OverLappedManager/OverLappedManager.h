#pragma once

#include "Define.h"
#include <iostream>
#include <boost/lockfree/queue.hpp>

constexpr uint16_t OVERLAPPED_TCP_QUEUE_SIZE = 10;

class OverLappedManager {
public:
	~OverLappedManager() {
		OverlappedEx* overlappedEx;
		while (ovLapPool.pop(overlappedEx)) {
			delete[] overlappedEx->wsaBuf.buf;
			delete overlappedEx;
		}
	}

	// ======================= INITIALIZATION =======================
	void init();


	// ================= OVERLAPPED POOL MANAGEMENT =================
	OverlappedEx* getOvLap();
	void returnOvLap(OverlappedEx* overlappedEx_); // Reset object


private:
	boost::lockfree::queue<OverlappedEx*> ovLapPool{ OVERLAPPED_TCP_QUEUE_SIZE };
};

