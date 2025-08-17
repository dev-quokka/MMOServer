#pragma once
#include "ItemData.h"

class ItemDataManager { // Singleton class for managing item data
public:
    static ItemDataManager& GetInstance();

    // Mysql에서 데이터 로드 후 세팅
    bool LoadFromMySQL(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& ItemMap_);

    const ItemData* GetItemData(uint16_t itemId_, uint16_t itemType_) const;

private:
    ItemDataManager() = default;
    ItemDataManager(const ItemDataManager&) = delete;
    ItemDataManager& operator=(const ItemDataManager&) = delete;
    ItemDataManager(ItemDataManager&&) = delete;
    ItemDataManager& operator=(ItemDataManager&&) = delete;

    std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash> ItemMap;

    bool loadCheck = false;
};