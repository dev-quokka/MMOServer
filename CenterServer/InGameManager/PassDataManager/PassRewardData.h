#pragma once

#include "PassData.h"
#include "PassDataPacket.h"

class PassRewardData {
public:
    // Mysql���� ������ �ε� �� ����
    bool LoadFromMySQL(PassInfo& passInfo_, std::unordered_map<PassDataKey, PassItemForSend, PassDataKeyHash>& PassDataMap_);

    const PassItemForSend* GetPassItemData(uint16_t passLevel_, uint16_t passCurrencyType_) const;

    const uint16_t GetPassMaxLevel() const;

private:
    std::unordered_map<PassDataKey, PassItemForSend, PassDataKeyHash> passDataMap;

    PassInfo passInfo;

    bool loadCheck = false;
};