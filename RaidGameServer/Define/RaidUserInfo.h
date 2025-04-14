#pragma once

struct RaidUserInfo {
    std::string userId;
    sockaddr_in userAddr;
    unsigned int userMaxScore;
    uint16_t userLevel;
    uint16_t userPk;
    uint16_t userConnObjNum = 0; // 유저 통신 고유 번호
    uint16_t userRaidServerObjNum = 0; // 레이드 방에서 사용하는 번호
    std::atomic<unsigned int> userScore = 0;
};
