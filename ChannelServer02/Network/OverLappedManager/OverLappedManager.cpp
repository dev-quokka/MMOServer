#include "OverLappedManager.h"

// ====================== INITIALIZATION ======================

void OverLappedManager::init() {
	for (int i = 0; i < OVERLAPPED_QUEUE_SIZE; i++) {
		OverlappedEx* overlappedEx = new OverlappedEx;
		ZeroMemory(overlappedEx, sizeof(OverlappedEx));
		ovLapPool.push(overlappedEx);
	}
}


// ================= OVERLAPPED POOL MANAGEMENT ================

OverlappedEx* OverLappedManager::getOvLap() {
	OverlappedEx* overlappedEx;

	if (ovLapPool.pop(overlappedEx)) {
		return overlappedEx;
	}
	else return nullptr;
}

void OverLappedManager::returnOvLap(OverlappedEx* overlappedEx_) {
	delete[] overlappedEx_->wsaBuf.buf;
	ZeroMemory(overlappedEx_, sizeof(OverlappedEx));
	ovLapPool.push(overlappedEx_);
}

