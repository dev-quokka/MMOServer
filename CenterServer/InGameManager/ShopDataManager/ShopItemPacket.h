#pragma once
#include <cstdint>

constexpr uint16_t MAX_ITEM_ID_LEN = 32;

struct ShopItemForSend {
	char itemName[MAX_ITEM_ID_LEN + 1];
	uint32_t itemPrice = 0;
	uint16_t itemCode = 0;
	uint16_t itemCount = 1; // ������ ����
	uint16_t daysOrCount = 0; // [���: �Ⱓ, �Һ�: ���� ����] 
	uint16_t itemType;
	uint16_t currencyType; // ��������

	// ��� ������ �ʿ� ����
	uint16_t attackPower = 0;

	// �Һ� ������ �ʿ� ����

	// ��� ������ �ʿ� ����
};

struct ShopDataForSend {
	char* shopPacketBuffer;
	size_t shopPacketSize = 0;

	~ShopDataForSend() {
		delete[] shopPacketBuffer;
	}
};

