#pragma once
#include <unordered_map>
#include <string>
#include <iostream>
#include <algorithm>

#include "ItemDataManager.h"

enum class PassCurrencyType : uint16_t { // 무료 패스 or 유료 패스
    FREE,
    CASH1
};

inline const std::unordered_map<PassCurrencyType, std::string> PassCurrencyTypeMap = {
     {PassCurrencyType::FREE, "free"},
     {PassCurrencyType::CASH1, "cash1"},
};

struct PassInfo {
    std::string eventStart;
    std::string eventEnd;
    uint16_t passMaxLevel = 0;
};

struct PassDataKey {
    uint16_t passLevel = 0;
    uint16_t passCurrencyType = 0;

    PassDataKey(uint16_t passLevel_, uint16_t passCurrencyType_) : passLevel(passLevel_), passCurrencyType(passCurrencyType_) {}

    bool operator==(const PassDataKey& other) const {
        return passLevel == other.passLevel && passCurrencyType == other.passCurrencyType;
    }
};

struct PassDataKeyHash { // PassDataKey용 해시 함수 (unordered_map에서 사용)
    size_t operator()(const PassDataKey& k) const noexcept {
        return std::hash<uint16_t>()(k.passLevel) ^ (std::hash<uint16_t>()(k.passCurrencyType) << 1);
    }
};