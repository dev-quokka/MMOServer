#pragma once

#include "PassRewardData.h"

class PassRewardManager { // Singleton class for managing item data
public:
    static PassRewardManager& GetInstance();

    // Mysql���� ������ �ε� �� ����
    // { �н� ����, �н� ������ Map, �� �н� ������ �ʿ� ����ġ, ���� ���ۿ� char*, ���� ������ ���� �޸� ù��° ��ġ, ���ۿ� char* size }
    bool LoadFromMySQL(std::vector<std::pair<std::string, PassInfo>> passIdVector_,
        std::unordered_map<std::string, std::unordered_map<PassDataKey, PassItemForSend, PassDataKeyHash>>& passDataMap_,
        std::vector<uint16_t>& passExpLimit_, char* packetBuffer_, PassItemForSend* passVector_, size_t packetSize_); 

    // ���� ���� �� ��ü �н� ������ �����ϱ� ���� ���� ��ȯ
    const PassDataForSend& GetPassData() const;

    const PassItemForSend* GetPassItemDataByPassId(std::string& passId_, uint16_t passLevel_, uint16_t passCurrencyType_) const;
    // const uint32_t GetPassLevelUpExp(std::string& passId_, uint16_t passLevel_) const;

    const std::vector<std::string>& GetPassIdVector();

    const PassLevelOrExpUpCheck PassExpUp(std::string passId_, uint16_t acqPassExp_, uint16_t userLevel, uint16_t currentPassExp_);

private:
    PassRewardManager() = default;
    PassRewardManager(const PassRewardManager&) = delete;
    PassRewardManager& operator=(const PassRewardManager&) = delete;
    PassRewardManager(PassRewardManager&&) = delete;
    PassRewardManager& operator=(PassRewardManager&&) = delete;

    std::unordered_map<std::string, PassRewardData> passMap;

    std::vector<std::string> passIdVector;
    std::vector<uint16_t> passExpLimit; // �н� ���� �� �ʿ� ����ġ��

    PassDataForSend passDataForSend;

    bool loadCheck = false;
};