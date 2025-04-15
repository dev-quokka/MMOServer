#pragma once

struct RaidUserInfo {
    std::string userId;
    sockaddr_in userAddr;
    unsigned int userMaxScore;
    uint16_t userLevel;
    uint16_t userPk;
    uint16_t userConnObjNum = 0; // Unique user ID for the Game Server
    uint16_t userRaidServerObjNum = 0; // Unique user ID for the raid game room
    std::atomic<unsigned int> userScore = 0;
};
