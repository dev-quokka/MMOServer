#pragma once
#pragma comment (lib, "libmysql.lib")

#include "DBConfig.h"

class MySQLManager {
public:
	MySQLManager() : semaphore(dbConnectionCount) {}
	~MySQLManager() {
		while (!dbPool.empty()) {
			MYSQL* conn = dbPool.front();
			dbPool.pop();
			mysql_close(conn);
		}
	}

	MYSQL* GetConnection();

	bool UpdatePassItem(char* passId_, uint32_t userPk_, uint16_t passLevel_, uint16_t passCurrencyType_, uint16_t itemCode, uint16_t daysOrCounts_, uint16_t itemType_);

	// ====================== INITIALIZATION =======================
	bool init();

	bool GetEquipmentItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_);
	bool GetConsumableItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_);
	bool GetMaterialItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_);
	bool GetShopItemData(std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash>& shopItemData_);
	bool GetPassInfo(std::vector<std::pair<std::string, PassInfo>>& passInfoVector_);
	bool GetPassItemData(std::vector<std::pair<std::string, PassInfo>>& passInfoVector_, std::unordered_map<std::string, std::unordered_map<PassDataKey, PassItemForSend, PassDataKeyHash>>& passDataMap_);
	bool GetPassExpData(std::vector<uint16_t>& passExpLimit_);


	// ======================= SYNCRONIZATION =======================
	bool LogoutSync(uint32_t userPk_, USERINFO userInfo_, std::vector<EQUIPMENT> userEquip_, std::vector<CONSUMABLES> userConsum_, std::vector<MATERIALS> userMat_, std::vector<UserPassDataForSync> userPassDataForSync_);
	bool SyncUserInfo(uint32_t userPk_, USERINFO userInfo_);
	bool SyncEquipment(uint32_t userPk_, std::vector<EQUIPMENT> userEquip_);
	bool SyncConsumables(uint32_t userPk_, std::vector<CONSUMABLES> userConsum_);
	bool SyncMaterials(uint32_t userPk_, std::vector<MATERIALS> userMat_);
	bool SyncPassInfo(uint32_t userPk_, std::vector<UserPassDataForSync>& userPassDataForSync_);

	bool MySQLSyncEqipmentEnhace(uint32_t userPk_, uint16_t itemPosition, uint16_t enhancement);
	bool MySQLSyncUserRaidScore(uint32_t userPk_, unsigned int userScore_, std::string userId_);


	// ======================== TRANSACTION ========================
	bool CashCharge(uint32_t userPk_, uint32_t chargedAmount);


private:
	std::mutex dbPoolMutex;
	std::queue<MYSQL*> dbPool;
	std::counting_semaphore<dbConnectionCount> semaphore;

	int MysqlResult;
};