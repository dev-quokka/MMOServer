#pragma once

#include <iostream>
#include <sw/redis++/redis++.h>

#include "UserProcessor.h"
#include "ServerProcessor.h"
#include "MySQLManager.h"

constexpr uint16_t serverThreadCnt = 1;
constexpr uint16_t userThreadCnt = 1;

int main() {
	std::shared_ptr<sw::redis::RedisCluster> redis;
	sw::redis::ConnectionOptions connection_options;

    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        // Redis 클러스터 연결
        redis = std::make_shared<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connect Success !" << std::endl;

    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis 에러 발생: " << err.what() << std::endl;
    }

    MySQLManager* mysqlManager = new MySQLManager;
    if (!mysqlManager->Run(redis)) {
        return 0;
    }

    ServerProcessor serverProcessor;
    UserProcessor userProcessor;

    if (serverProcessor.init(serverThreadCnt, redis, mysqlManager)) {
        std::cout << "Success To Make ServerProc" << std::endl;
    }
    else {
        std::cout << "Fail To Make ServerProc" << std::endl;
    }

    if (userProcessor.init(userThreadCnt, redis, mysqlManager)) {
        std::cout << "Success To Make userProc" << std::endl;
    }
    else {
        std::cout << "Fail To Make userProc" << std::endl;
    }


    std::cout << "=== MMO SESSION SERVER START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write session ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "session") break;
    }

    delete mysqlManager;
    return 0;
}