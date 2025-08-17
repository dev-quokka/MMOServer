#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>
#include <algorithm>

#include "ItemDataManager.h"

enum class CurrencyType : uint16_t {
    GOLD,
    CASH,
    MILEAGE
};

inline const std::unordered_map<uint16_t, std::string> currencyTypeMap = {
     {0, "gold"}, // CurrencyType::GOLD
     {1, "cash"}, // CurrencyType::CASH
     {2, "mileage"}, // CurrencyType::MILEAGE
};

struct ShopItemKey {
    uint16_t itemCode = 0;
    uint16_t days = 0;
  
    ShopItemKey(uint16_t itemCode_, uint16_t days_) : itemCode(itemCode_), days(days_) {}

    bool operator==(const ShopItemKey& other) const {
        return itemCode == other.itemCode && days == other.days;
    }
};

struct ShopItemKeyHash { // ShopItemKey용 해시 함수 (unordered_map에서 사용)
    size_t operator()(const ShopItemKey& k) const noexcept {
        return std::hash<uint16_t>()(k.itemCode)^(std::hash<uint16_t>()(k.days) << 1);
    }
};

struct ShopItem {
    uint32_t itemPrice = 0;
    uint16_t itemCode = 0;
    uint16_t itemCount = 1; // 아이템 개수
    uint16_t daysOrCount = 0; // [장비: 기간, 소비: 개수 묶음] 
    ItemType itemType;
    CurrencyType currencyType; // 결제수단
};