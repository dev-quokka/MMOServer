#pragma once
#include <cstdint>

constexpr uint16_t MAX_PASS_ID_LEN = 32;

struct PassItemForSend {
    char itemName[MAX_ITEM_ID_LEN + 1];
    char passId[MAX_PASS_ID_LEN + 1];
    uint16_t itemCode = 0;
    uint16_t passLevel = 0;
    uint16_t itemCount = 1; // 아이템 개수
    uint16_t daysOrCount = 0;
    uint16_t itemType;
    uint16_t passCurrencyType;
};

struct UserPassDataForSync {
    char passId[MAX_PASS_ID_LEN + 1];
    uint16_t passLevel = 0;
    uint16_t passExp = 0;
    uint16_t passCurrencyType = 0;  
};

struct PassLevelOrExpUpCheck {
    uint16_t currentUserLevel = 1;
    uint16_t currentUserExp = 0;
    uint16_t levelupCount = 0; // 레디스 set이 아닌, hincrby하기 위한 변수
};

struct PassDataForSend {
    char* passPacketBuffer;
    size_t passPacketSize = 0;

    ~PassDataForSend() {
        delete[] passPacketBuffer;
    }
};