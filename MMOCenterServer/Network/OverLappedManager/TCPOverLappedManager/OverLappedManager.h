#pragma once

#include "Define.h"
#include <iostream>
#include <boost/lockfree/queue.hpp>

constexpr uint16_t OVERLAPPED_TCP_QUEUE_SIZE = 10;

class OverLappedManager {
public:
	~OverLappedManager() {
		OverlappedTCP* overlappedTCP;
		while (ovLapPool.pop(overlappedTCP)) {	
			delete[] overlappedTCP->wsaBuf.buf;
			delete overlappedTCP;
		}
	}

	void init();
	OverlappedTCP* getOvLap();
	void returnOvLap(OverlappedTCP* overlappedTCP_);

private:
	boost::lockfree::queue<OverlappedTCP*> ovLapPool{ OVERLAPPED_TCP_QUEUE_SIZE };
};

