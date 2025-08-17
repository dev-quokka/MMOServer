#pragma once
#include <unordered_map>
#include <string>
#include <iostream>

#include "ShopItemPacket.h"

enum class ItemType : uint16_t {
    EQUIPMENT,
    CONSUMABLE,
    MATERIAL
};

struct ItemData {
    std::string itemName = "";
    uint16_t itemCode = 0;
    ItemType itemType;
};

struct EquipmentItemData : public ItemData {
    uint16_t attackPower = 0;

    // 추후 필요한 데이터(레벨 제한 및 등급 제한) 추가 예정

    void setEquipmentItemData(ShopItemForSend& tempShopData_) const {
        strncpy_s(tempShopData_.itemName, itemName.c_str(), MAX_ITEM_ID_LEN);
        tempShopData_.attackPower = attackPower;
    }
};

struct ConsumableItemData : public ItemData {

    // 추후 필요한 데이터 추가 예정
    
    //void setConsumableItemData(ShopItemForSend& tempShopData_) {
    //     strncpy(tempShopData_.itemName, itemName.c_str(), MAX_ITEM_ID_LEN);
    //}
};

struct MaterialItemData : public ItemData {

    // 추후 필요한 데이터 추가 예정

    //void setMaterialItemData(ShopItemForSend& tempShopData_) {
    //     strncpy(tempShopData_.itemName, itemName.c_str(), MAX_ITEM_ID_LEN);
    //}
};

struct ItemDataKey {
    uint16_t itemCode = 0;
    uint16_t ItemType = 0;

    ItemDataKey(uint16_t itemCode_, uint16_t ItemType_) : itemCode(itemCode_), ItemType(ItemType_) {}

    bool operator==(const ItemDataKey& other) const {
        return itemCode == other.itemCode && ItemType == other.ItemType;
    }
};

struct ItemDataKeyHash { // ItemDataKey용 해시 함수 (unordered_map에서 사용)
    size_t operator()(const ItemDataKey& k) const noexcept {
        return std::hash<uint16_t>()(k.itemCode) ^ (std::hash<uint16_t>()(k.ItemType) << 1);
    }
};