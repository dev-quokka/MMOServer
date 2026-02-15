#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <string>
#include <chrono>

#include "UserSyncData.h"
#include "ShopItemPacket.h"
#include "PassDataPacket.h"

constexpr uint16_t MAX_IP_LEN = 32;
constexpr uint16_t MAX_SERVER_USERS = 128;
constexpr uint16_t MAX_JWT_TOKEN_LEN = 256;
constexpr uint16_t MAX_SCORE_SIZE = 512;

struct DataPacket {
	uint32_t dataSize;
	uint16_t connObjNum;
	DataPacket(uint32_t dataSize_, uint16_t connObjNum_) : dataSize(dataSize_), connObjNum(connObjNum_) {}
	DataPacket() = default;
};

struct PacketInfo
{
	uint16_t packetId = 0;
	uint16_t dataSize = 0;
	uint16_t connObjNum = 0;
	char* pData = nullptr;
};

struct PACKET_HEADER
{
	uint16_t PacketLength;
	uint16_t PacketId;
};


// ======================= TEST =======================

struct CASH_CHARGE_COMPLETE_REQUEST : PACKET_HEADER {
	uint32_t chargedCash = 0;
};

struct CASH_CHARGE_COMPLETE_RESPONSE : PACKET_HEADER {
	uint32_t chargedCash = 0;
	bool isSuccess = false;
};


// ======================= CENTER SERVER =======================

struct USER_CONNECT_REQUEST_PACKET : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1];
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_RESPONSE_PACKET : PACKET_HEADER {
	bool isSuccess = false;
};

struct USER_LOGOUT_REQUEST_PACKET : PACKET_HEADER {

};

struct SYNCRONIZE_LEVEL_REQUEST : PACKET_HEADER {
	uint16_t level;
	uint16_t userPk;
	unsigned int currentExp;
};

struct SYNCRONIZE_LOGOUT_REQUEST : PACKET_HEADER {
	uint16_t userPk;
};

struct SERVER_USER_COUNTS_REQUEST : PACKET_HEADER {

};

struct SERVER_USER_COUNTS_RESPONSE : PACKET_HEADER {
	uint16_t serverCount;
	uint16_t serverUserCnt[MAX_SERVER_USERS + 1];
};

struct SHOP_DATA_REQUEST : PACKET_HEADER {

};

struct SHOP_DATA_RESPONSE : PACKET_HEADER {
	uint16_t shopItemSize;
};

struct PASS_DATA_REQUEST : PACKET_HEADER {

};

struct PASS_DATA_RESPONSE : PACKET_HEADER {
	uint16_t passDataSize;
};

struct MOVE_SERVER_REQUEST : PACKET_HEADER {
	uint16_t serverNum;
};

struct MOVE_SERVER_RESPONSE : PACKET_HEADER {
	char serverToken[MAX_JWT_TOKEN_LEN + 1];
	char ip[MAX_IP_LEN + 1];
	uint16_t port;
};

struct RAID_READY_REQUEST : PACKET_HEADER {
	char serverToken[MAX_JWT_TOKEN_LEN + 1];
	char ip[MAX_IP_LEN + 1];
	uint16_t port;
	uint16_t roomNum;
};

struct RAID_READY_FAIL : PACKET_HEADER {
	uint16_t userCenterObjNum;
	uint16_t roomNum;
};

struct RAID_RANKING_REQUEST : PACKET_HEADER {
	uint16_t startRank;
};

struct RAID_RANKING_RESPONSE : PACKET_HEADER {
	uint16_t rkCount;
	char reqScore[MAX_SCORE_SIZE + 1];
};

struct SHOP_BUY_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemCode = 0;
	uint16_t daysOrCount = 0; // [장비: 유저가 원하는 아이템의 사용 기간, 소비: 유저가 원하는 아이템 개수 묶음] 
	uint16_t itemType; // 0: 장비, 1: 소비, 2: 재료
	uint16_t position;
};

struct SHOP_BUY_ITEM_RESPONSE : PACKET_HEADER {
	ShopItemForSend shopItemForSend;
	uint32_t remainMoney;
	uint16_t currencyType;
	bool isSuccess = false;
};

struct PASS_EXP_UP_REQUEST : PACKET_HEADER {
	char passId[MAX_PASS_ID_LEN + 1];
	uint16_t missionNum; // 수행할 미션 번호
};

struct PASS_EXP_UP_RESPONSE : PACKET_HEADER {
	char passId[MAX_PASS_ID_LEN + 1];
	uint16_t passLevel = 0;
	uint16_t passExp;
	bool isSuccess = false;
};

struct GET_PASS_ITEM_REQUEST : PACKET_HEADER {
	char passId[MAX_PASS_ID_LEN + 1];
	uint16_t passLevel;
	uint16_t passCurrencyType;
};

struct GET_PASS_ITEM_RESPONSE : PACKET_HEADER {
	PassItemForSend passItemForSend;
	char passId[MAX_PASS_ID_LEN + 1];
	uint16_t passLevel;
	uint16_t passCurrencyType;
	uint16_t position = 1;
	bool passAcq = false;
	bool isSuccess = false;
};


// ======================= CASH SERVER =======================

struct CASH_SERVER_CONNECT_REQUEST : PACKET_HEADER {

};

struct CASH_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
	bool isSuccess = false;
};

struct CASH_CHARGE_RESULT_RESPONSE : PACKET_HEADER {
	uint32_t chargedAmount; // 충전된 금액
	uint16_t uCASH_CHARGE_RESULTserId; // 유저 고유 번호
	bool isSuccess = false; // 충전 성공 유무
};


// ======================= LOGIN SERVER =======================

struct LOGIN_SERVER_CONNECT_REQUEST : PACKET_HEADER {

};

struct LOGIN_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
	bool isSuccess = false;
};


// ======================= CHANNEL SERVER =======================

struct CHANNEL_SERVER_CONNECT_REQUEST : PACKET_HEADER {
	uint16_t channelServerNum;
};

struct CHANNEL_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
	bool isSuccess = false;
};

struct USER_DISCONNECT_AT_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelServerNum;
};

struct SYNC_EQUIPMENT_ENHANCE_REQUEST : PACKET_HEADER {
	uint16_t itemPosition;
	uint16_t enhancement;
	uint16_t userPk;
};


// ======================= MATCHING SERVER =======================

struct MATCHING_SERVER_CONNECT_REQUEST : PACKET_HEADER {

};

struct MATCHING_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
	bool isSuccess = false;
};

struct MATCHING_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userPk;
	uint16_t userCenterObjNum;
	uint16_t userGroupNum;
};

struct MATCHING_RESPONSE_FROM_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	bool isSuccess = false;
};

struct MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	uint16_t roomNum;
};

struct RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t roomNum;
};

struct MATCHING_CANCEL_REQUEST : PACKET_HEADER {

};

struct MATCHING_CANCEL_RESPONSE : PACKET_HEADER {
	bool isSuccess = false;
};

struct MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	uint16_t userGroupNum;
};

struct MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	bool isSuccess = false;
};


// ======================= RAID GAME SERVER =======================

struct RAID_SERVER_CONNECT_REQUEST : PACKET_HEADER {
	uint16_t gameServerNum;
};

struct RAID_SERVER_CONNECT_RESPONSE : PACKET_HEADER {
	bool isSuccess = false;
};

struct RAID_MATCHING_REQUEST : PACKET_HEADER {

};

struct RAID_MATCHING_RESPONSE : PACKET_HEADER {
	bool insertSuccess;
};

struct MATCHING_RESPONSE_FROM_GAME_SERVER : PACKET_HEADER {
	uint16_t userCenterObjNum;
	uint16_t userRaidServerObjNum;
	uint16_t serverNum;
	uint16_t roomNum;
};

struct RAID_END_REQUEST_TO_CENTER_SERVER : PACKET_HEADER {
	uint16_t gameServerNum;
	uint16_t roomNum;
};

struct SYNC_HIGHSCORE_REQUEST : PACKET_HEADER {
	char userId[MAX_USER_ID_LEN + 1];
	unsigned int userScore;
	uint16_t userPk;
};


enum class PACKET_ID : uint16_t {

	// ======================= CENTER SERVER (1~ ) =======================
	
	// SYSTEM (1~)
	USER_CONNECT_REQUEST = 1,
	USER_CONNECT_RESPONSE = 2,
	USER_LOGOUT_REQUEST = 3, 
	USER_FULL_REQUEST = 6, 
	WAITTING_NUMBER_REQUSET = 7,
	SERVER_USER_COUNTS_REQUEST = 8,
	SERVER_USER_COUNTS_RESPONSE = 9,
	MOVE_SERVER_REQUEST = 10,
	MOVE_SERVER_RESPONSE = 11,

	// RAID (45~)
	RAID_MATCHING_REQUEST = 45,
	RAID_MATCHING_RESPONSE = 46,
	MATCHING_CANCEL_REQUEST = 47,
	MATCHING_CANCEL_RESPONSE = 48,
	RAID_READY_REQUEST = 49,
	RAID_READY_FAIL = 50,

	RAID_END_REQUEST_TO_GAME_SERVER = 52,

	// SHOP (101~ )
	SHOP_DATA_REQUEST = 101,
	SHOP_DATA_RESPONSE = 102,
	SHOP_BUY_ITEM_REQUEST = 103,
	SHOP_BUY_ITEM_RESPONSE = 104,

	// PASSITEM (301~ )
	PASS_DATA_REQUEST = 301,
	PASS_DATA_RESPONSE = 302,
	GET_PASS_ITEM_REQUEST = 303,
	GET_PASS_ITEM_RESPONSE = 304,

	PASS_EXP_UP_REQUEST = 310,
	PASS_EXP_UP_RESPONSE = 311,


	// ======================= CASH SERVER (501~ ) =======================	

	CASH_SERVER_CONNECT_REQUEST = 501,
	CASH_SERVER_CONNECT_RESPONSE = 502,

	CASH_CHARGE_RESULT_RESPONSE = 503,




	// 유저 캐시 충전 완료 요청 테스트용 (511,512)
	CASH_CHARGE_COMPLETE_REQUEST = 511,
	CASH_CHARGE_COMPLETE_RESPONSE = 512,
	

	// ======================= LOGIN SERVER (801~ ) =======================

	// SYSTEM (801~)
	LOGIN_SERVER_CONNECT_REQUEST = 801,
	LOGIN_SERVER_CONNECT_RESPONSE = 802,

	// USER LOGIN (811~)
	USER_LOGIN_REQUEST = 811,
	USER_LOGIN_RESPONSE = 812,
	USER_GAMESTART_REQUEST = 813,
	USER_GAMESTART_RESPONSE = 814,
	USERINFO_REQUEST = 815,
	USERINFO_RESPONSE = 816,
	EQUIPMENT_REQUEST = 817,
	EQUIPMENT_RESPONSE = 818,
	CONSUMABLES_REQUEST = 819,
	CONSUMABLES_RESPONSE = 820,
	MATERIALS_REQUEST = 821,
	MATERIALS_RESPONSE = 822,

	// SYNCRONIZATION (851~)
	SYNCRONIZE_LEVEL_REQUEST = 851,
	SYNCRONIZE_LOGOUT_REQUEST = 852,
	SYNCRONIZE_DISCONNECT_REQUEST = 853,


	// ======================= CHANNEL SERVER (1501~ ) =======================
	
	// SYSTEM (1501~)
	CHANNEL_SERVER_CONNECT_REQUEST = 1501,
	CHANNEL_SERVER_CONNECT_RESPONSE = 1502,
	USER_DISCONNECT_AT_CHANNEL_REQUEST = 1503,
	MOVE_CENTER_SERVER_REQUEST = 1504,
	MOVE_CENTER_SERVER_RESPONSE = 1505,

	SYNC_EQUIPMENT_ENHANCE_REQUEST = 1541,


	// ======================= MATCHING SERVER (5001~ ) =======================
	
	//SYSTEM (5001~)
	MATCHING_SERVER_CONNECT_REQUEST = 5001,
	MATCHING_SERVER_CONNECT_RESPONSE = 5002,

	//RAID(5011~)
	MATCHING_REQUEST_TO_MATCHING_SERVER = 5011,
	MATCHING_RESPONSE_FROM_MATCHING_SERVER = 5012,
	MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER = 5013,
	RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER = 5014,

	MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER = 5021,
	MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER = 5022,


	// ======================= RAID GAME SERVER (8001~ ) =======================
	
	RAID_SERVER_CONNECT_REQUEST = 8001,
	RAID_SERVER_CONNECT_RESPONSE = 8002,

	MATCHING_RESPONSE_FROM_GAME_SERVER = 8012,

	SYNC_HIGHSCORE_REQUEST = 8091,
};