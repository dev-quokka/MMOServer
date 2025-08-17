#pragma once

#include "ShopItemData.h"
#include "ShopItemPacket.h"
#include "ItemDataManager.h"

class ShopDataManager { // Singleton class for managing shop data
public:
	static ShopDataManager& GetInstance();

	// MySQL���� ���� �����͸� �ε��� ��, �� ������ �ڵ忡 �ش��ϴ� ItemData ������ ��Ī�Ͽ� ����
	bool LoadFromMySQL(std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash>& shopItemMap_, char* packetBuffer_, ShopItemForSend* itemVector_, size_t packetSize_); // {���� ������ Map, ���� ���ۿ� char*, ���� ������ ���� �޸� ù��° ��ġ, ���ۿ� char* size}

	// ������ �������� Ư�� �������� �������� �� �ش� ������ ������ ��ȯ
	const ShopItemForSend* GetItem(uint16_t itemId, uint16_t days) const;

	// ���� ���� �� ��ü ���� ������ �����ϱ� ���� ���� ��ȯ
	const ShopDataForSend& GetShopData() const;

private:
	ShopDataManager() = default;
	~ShopDataManager() = default;

	ShopDataManager(const ShopDataManager&) = delete;
	ShopDataManager& operator=(const ShopDataManager&) = delete;
	ShopDataManager(ShopDataManager&&) = delete;
	ShopDataManager& operator=(ShopDataManager&&) = delete;

	std::unordered_map<ShopItemKey, ShopItemForSend, ShopItemKeyHash> shopItemMap;

	ShopDataForSend shopDataForSend;

	bool loadCheck = false;
};