#pragma once
#include <cstdint>
#include <string>
#include <atomic>
#include <iostream>

constexpr uint16_t MAX_RAID_ROOM_PLAYERS = 2;
constexpr uint16_t MAX_ROOM = 10;
constexpr uint16_t WAITING_USERS_TIME = 30; // Time to wait for all users to be ready
constexpr uint16_t TICK_RATE = 5; // Tick interval
constexpr int UDP_PORT = 50001; // Gameserver1 Udp Port

struct RaidUserInfo {
    std::string userId = "";
    sockaddr_in userAddr;
    unsigned int userMaxScore = 0;
    std::atomic<unsigned int> userScore = 0;
    uint16_t userPk;
    uint16_t userLevel = 1;
    uint16_t userConnObjNum = 80000; // Unique user ID for the Game Server
    uint16_t userCenterObjNum = 80000; // Unique user ID for the Center Server
    uint16_t userRaidServerObjNum = 80000; // Unique user ID for the raid game room
};
