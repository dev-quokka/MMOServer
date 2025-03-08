#include "OverLappedManager.h"

void OverLappedManager::init() {
	for (int i = 0; i < OVERLAPPED_TCP_QUEUE_SIZE; i++) {
		OverlappedTCP* overlappedTCP = new OverlappedTCP; // 생성
		ZeroMemory(overlappedTCP, sizeof(OverlappedTCP)); // 초기화
		ovLapPool.push(overlappedTCP);
	}
}

OverlappedTCP* OverLappedManager::getOvLap() {
	OverlappedTCP* overlappedTCP_;
	if (ovLapPool.pop(overlappedTCP_)) {
		return overlappedTCP_;
	}
	else return nullptr;
}

void OverLappedManager::returnOvLap(OverlappedTCP* overlappedTCP_){
	delete[] overlappedTCP_->wsaBuf.buf;
	ZeroMemory(overlappedTCP_, sizeof(OverlappedTCP)); // 초기화
	ovLapPool.push(overlappedTCP_);
}

