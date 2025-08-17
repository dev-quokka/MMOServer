#pragma once
#include <cstdint>

constexpr uint16_t MAX_ITEM_ID_LEN = 32;

struct ShopItemForSend {
	char itemName[MAX_ITEM_ID_LEN + 1];
	uint32_t itemPrice = 0;
	uint16_t itemCode = 0;
	uint16_t itemCount = 1; // 아이템 개수
	uint16_t daysOrCount = 0; // [장비: 기간, 소비: 개수 묶음] 
	uint16_t itemType;
	uint16_t currencyType; // 결제수단

	// 장비 아이템 필요 변수
	uint16_t attackPower = 0;

	// 소비 아이템 필요 변수

	// 재료 아이템 필요 변수
};

struct ShopDataForSend {
	char* shopPacketBuffer;
	size_t shopPacketSize = 0;

	~ShopDataForSend() {
		delete[] shopPacketBuffer;
	}
};

