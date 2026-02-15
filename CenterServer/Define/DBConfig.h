#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <mysql.h>
#include <sstream>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <semaphore>

#include "UserSyncData.h"
#include "ItemData.h"
#include "PassData.h"
#include "ShopItemData.h"
#include "PassDataPacket.h"

constexpr const char* DB_HOST = "127.0.0.1";
constexpr const char* DB_USER = "quokka";
constexpr const char* DB_PASSWORD = "1234";
constexpr const char* DB_NAME = "iocp";

constexpr uint16_t DB_PORT = 3306;

constexpr uint16_t dbConnectionCount = 5;


struct AutoConn { // 함수 실행 후 커넥션, 세마포어 자동 반환을 위한 임시 구조체 (안전성 및 가독성 향상 목적)
	MYSQL* tempConn;
	std::queue<MYSQL*>& dbPool_;
	std::mutex& dbPoolMutex_;
	std::counting_semaphore<dbConnectionCount>& semaphore_;

	AutoConn(MYSQL* conn, std::queue<MYSQL*>& pool, std::mutex& mtx, std::counting_semaphore<dbConnectionCount>& sem)
		: tempConn(conn), dbPool_(pool), dbPoolMutex_(mtx), semaphore_(sem) {
	}

	~AutoConn() {
		std::lock_guard<std::mutex> lock(dbPoolMutex_);
		dbPool_.push(tempConn);
		semaphore_.release();
	}
};