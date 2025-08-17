#include "ItemDataManager.h"

ItemDataManager& ItemDataManager::GetInstance() {
	static ItemDataManager instance;
	return instance;
}

bool ItemDataManager::LoadFromMySQL(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& ItemMap_){
	
	if (loadCheck) { // �̹� �����Ͱ� �ε�Ǿ����Ƿ� �ߺ� ȣ�� ����
		std::cout << "[ItemDataManager::LoadFromMySQL] LoadFromMySQL already completed." << '\n';
		return true;
	}
	
	ItemMap = std::move(ItemMap_);

	loadCheck = true;
	return true;
}

const ItemData* ItemDataManager::GetItemData(uint16_t itemId_, uint16_t itemType_) const {
	auto it = ItemMap.find({ itemId_, itemType_ });
	if (it == ItemMap.end()) {
		return nullptr;
	}

	return it->second.get();
}
