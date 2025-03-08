#pragma once

#include "Define.h"
#include <iostream>
#include <boost/lockfree/queue.hpp>

constexpr uint16_t OVERLAPPED_UDP_QUEUE_SIZE = 10;

class UdpOverLappedManager {
public:
	~UdpOverLappedManager() {
		OverlappedUDP* overlappedUDP;
		while (ovLapPool.pop(overlappedUDP)) {
			delete[] overlappedUDP->wsaBuf.buf;
			delete overlappedUDP;
		}
	}

	void init();
	OverlappedUDP* getOvLap();
	void returnOvLap(OverlappedUDP* overlappedUDP_);

private:
	boost::lockfree::queue<OverlappedUDP*> ovLapPool{ OVERLAPPED_UDP_QUEUE_SIZE };
};