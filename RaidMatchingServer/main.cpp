#include <iostream>
#include <sw/redis++/redis++.h>

#include "MatchingServer.h"

constexpr uint16_t MATCHING_SERVER_PORT = 9131;
constexpr uint16_t maxThreadCount = 1;

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

	MatchingServer matchingServer;
    matchingServer.Init(maxThreadCount, MATCHING_SERVER_PORT);
    matchingServer.StartWork();

    std::cout << "=== MATCHING SERVER START ===" << std::endl;
    std::cout << "=== If You Want Exit, Write matching ===" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "matching") break;
    }

    matchingServer.ServerEnd();

	return 0;
}