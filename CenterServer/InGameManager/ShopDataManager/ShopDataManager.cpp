#include "ShopDataManager.h"

ShopDataManager& ShopDataManager::GetInstance() {
    static ShopDataManager instance;
    return instance;
}

bool ShopDataManager::LoadFromMySQL(std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash>& shopItemMap_, char* packetBuffer_, ShopItemForSend* itemVector_, size_t packetSize_) {
	
	if (loadCheck) { // 이미 데이터가 로드되었으므로 중복 호출 방지
		std::cout << "[ShopDataManager::LoadFromMySQL] LoadFromMySQL already completed." << '\n';
		return true;
	}

	std::vector<ShopItemForSend> shopItemVector;

	// 상점 장비 데이터 저장
	for (auto& [itemId, shopItem] : shopItemMap_) {
		auto tempItemInfo = ItemDataManager::GetInstance().GetItemData(shopItem.itemCode, static_cast<uint16_t>(shopItem.itemType));
		if (!tempItemInfo) {
			std::cerr << "[ShopDataManager::LoadFromMySQL] Invalid itemCode: " << shopItem.itemCode << '\n';
			continue;
		}

        ShopItemForSend tempSendData;
		tempSendData.itemPrice = shopItem.itemPrice;
		tempSendData.itemCode = shopItem.itemCode;
		tempSendData.itemCount = shopItem.itemCount;
		tempSendData.daysOrCount = shopItem.daysOrCount;
		tempSendData.itemType = static_cast<uint16_t>(shopItem.itemType);
		tempSendData.currencyType = static_cast<uint16_t>(shopItem.currencyType);

		// 각 타입에 맞는 아이템 정보 세팅
        switch (shopItem.itemType) {
			case ItemType::EQUIPMENT: {
				const EquipmentItemData* eq = static_cast<const EquipmentItemData*>(tempItemInfo);
				eq->setEquipmentItemData(tempSendData);
			}
			break;
			case ItemType::CONSUMABLE:
				// const ConsumableItemData* cs = static_cast<const ConsumableItemData*>(tempItemInfo);
				break;
			case ItemType::MATERIAL:
				// const MaterialItemData* mt = static_cast<const MaterialItemData*>(tempItemInfo);
				break;
			default:
				break;
        }

		shopItemMap[itemId] = tempSendData;
		shopItemVector.emplace_back(tempSendData);
	}

	std::sort(shopItemVector.begin(), shopItemVector.end(), [](const auto& a, const auto& b) { // itemType 기준 오름차순 정렬 후, itemCode 기준 오름차순 정렬 후, daysOrCount 기준 오름차순 정렬
		return std::tie(a.itemType, a.itemCode, a.daysOrCount) < 
			   std::tie(b.itemType, b.itemCode, b.daysOrCount);
	});

	shopDataForSend.shopPacketBuffer = packetBuffer_;

	for (int i = 0; i < shopItemVector.size(); ++i) {
		itemVector_[i] = shopItemVector[i];
	}

	shopDataForSend.shopPacketSize = packetSize_;

	loadCheck = true;
	return true;
}

const ShopItemForSend* ShopDataManager::GetItem(uint16_t itemId, uint16_t days) const {
	auto it = shopItemMap.find({ itemId , days});
	if (it == shopItemMap.end()) {
		return nullptr;
	}

	return &(it->second);
}

const ShopDataForSend& ShopDataManager::GetShopData() const {
	return shopDataForSend;
}