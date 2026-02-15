#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <unordered_map>

inline const std::unordered_map<uint16_t, std::string> PassCurrencyTypeMap = {
	 {0, "free"},
	 {1, "cash1"},
};

const uint16_t RANKING_USER_COUNT = 3; // 몇명씩 유저 랭킹 정보 가져올건지

const uint16_t MAX_IP_LEN = 32;
const uint16_t MAX_SERVER_USERS = 128; // 서버 유저 수 전달 패킷
const uint16_t MAX_USER_ID_LEN = 32;
const uint16_t MAX_JWT_TOKEN_LEN = 256;
const uint16_t MAX_SCORE_SIZE = 256;

constexpr uint16_t MAX_EVENT_LEN = 32;
constexpr uint16_t MAX_ITEM_ID_LEN = 32;
constexpr uint16_t MAX_PASS_ID_LEN = 32;
constexpr uint16_t MAX_PASS_SIZE = 512;
constexpr uint16_t MAX_INVEN_SIZE = 512;

struct PACKET_HEADER
{
	uint16_t PacketLength;
	uint16_t PacketId;
};

struct PassInfo_Client {
	std::string eventStart;
	std::string eventEnd;
	uint16_t passMaxLevel = 0;
};

struct PassItemForSend {
	char itemName[MAX_ITEM_ID_LEN + 1];
	char passId[MAX_PASS_ID_LEN + 1];
	uint16_t itemCode = 0;
	uint16_t passLevel = 0;
	uint16_t itemCount = 1; // 아이템 개수
	uint16_t daysOrCount = 0;
	uint16_t itemType;
	uint16_t passCurrencyType;
};

struct PassDataKey {
	uint16_t passLevel = 0;
	uint16_t passCurrencyType = 0;

	PassDataKey(uint16_t passLevel_, uint16_t passCurrencyType_) : passLevel(passLevel_), passCurrencyType(passCurrencyType_) {}

	bool operator==(const PassDataKey& other) const {
		return passLevel == other.passLevel && passCurrencyType == other.passCurrencyType;
	}
};

struct PassDataKeySort {
	bool operator()(const PassDataKey& a, const PassDataKey& b) const {
		if (a.passLevel != b.passLevel) return a.passLevel < b.passLevel;  

		return a.passCurrencyType < b.passCurrencyType; 
	}
};

struct PassItem_Client {
	std::string itemName;
	uint16_t itemCode = 0;
	uint16_t passLevel = 0;
	uint16_t itemCount = 1; // 아이템 개수
	uint16_t daysOrCount = 0;
	uint16_t itemType;
	uint16_t passCurrencyType;
	bool getCheck = 0; // 획득 여부 체크

	void SetData(PassItemForSend& pdf) {
		itemName = (std::string)pdf.itemName;
		itemCode = pdf.itemCode;
		passLevel = pdf.passLevel;
		itemCount = pdf.itemCount;
		daysOrCount = pdf.daysOrCount;
		itemType = pdf.itemType;
		passCurrencyType = pdf.passCurrencyType;
	}

	void PrintPassData() {
		std::cout << "패스 레벨 :  " << passLevel;
		std::cout << " / 아이템 이름 : " << itemName;

		switch (itemType) {
		case 0: { // 장비
			std::cout << " / 기한 : " << daysOrCount << "일 ";
		}
			  break;
		case 1: { // 소비
			std::cout << " / 개수 :  " << daysOrCount << "개 ";
		}
			  break;
		case 2: { // 재료
			std::cout << " / 개수 : " << daysOrCount << "개 ";
		}
			  break;
		}

		switch (passCurrencyType) {
		case 0: {
			std::cout << " / 무료 ";
		}
			  break;
		case 1: {
			std::cout << " / 유료 ";
		}
			  break;
		}

		if(getCheck) std::cout << " / 획득" << '\n';
		else  std::cout << " / 미획득" << '\n';
	}
};

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

struct USERINFO {
	unsigned int raidScore = 0;
	unsigned int exp = 0;
	uint32_t gold = 0;
	uint32_t cash = 0;
	uint32_t mileage = 0;
	uint16_t level = 0;
};

struct EQUIPMENT {
	uint16_t itemCode = 0;
	uint16_t position = 0;
	uint16_t enhance = 0;
};

struct CONSUMABLES {
	uint16_t itemCode = 0;
	uint16_t position = 0;
	uint16_t count = 0;
};

struct MATERIALS {
	uint16_t itemCode = 0;
	uint16_t position = 0;
	uint16_t count = 0;
};

struct PASSINFO {
	char passId[MAX_PASS_ID_LEN + 1];
	char eventStart[MAX_EVENT_LEN + 1];
	char eventEnd[MAX_EVENT_LEN + 1];
	uint16_t passMaxLevel;
};

struct USERPASSINFO {
	PASSINFO passInfo;
	uint16_t passLevel;
	uint16_t passExp;
	uint16_t passCurrencyType;
};

struct PASSREWARDNFO {
	char passId[MAX_PASS_ID_LEN + 1];
	uint64_t passReqwardBits = 0;
	uint16_t passCurrencyType = 254;
};

struct PASSREWARDNFO_CLIENT {
	uint64_t passReqwardBits = 0;
	uint16_t passCurrencyType = 254;
};

struct RAID_USERINFO{
	std::string userId  = "";
	uint16_t userLevel = 0;
	uint16_t userScore = 0;
};

struct RANKING {
	uint16_t score = 0;
	char userId[MAX_USER_ID_LEN + 1] = {};
};

//  ---------------------------- TEST  ----------------------------

struct CASH_CHARGE_COMPLETE_REQUEST : PACKET_HEADER {
	uint32_t chargedCash = 0;
};

struct CASH_CHARGE_COMPLETE_RESPONSE : PACKET_HEADER {
	uint32_t chargedCash = 0;
	bool isSuccess;
};


//  ---------------------------- SYSTEM  ----------------------------

struct USER_CONNECT_REQUEST_PACKET : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_RESPONSE_PACKET : PACKET_HEADER {
	bool isSuccess;
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
	char serverToken[MAX_JWT_TOKEN_LEN + 1]; // Token For Server Connection
	char ip[MAX_IP_LEN + 1];
	uint16_t port;
};

struct CHANNEL_USER_COUNTS_REQUEST : PACKET_HEADER {

};

struct CHANNEL_USER_COUNTS_RESPONSE : PACKET_HEADER {
	uint16_t channelCount;
	char channelUserCnt[MAX_SERVER_USERS + 1];
};

struct USER_CONNECT_CHANNEL_REQUEST_PACKET : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_CHANNEL_RESPONSE_PACKET : PACKET_HEADER {
	bool isSuccess;
};

struct MOVE_CHANNEL_REQUEST : PACKET_HEADER {
	uint16_t channelNum;
};

struct MOVE_CHANNEL_RESPONSE : PACKET_HEADER {
	bool isSuccess;
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
	uint16_t position = 0;
	bool passAcq = false;
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

//  ---------------------------- LOGIN  ----------------------------

struct USER_GAMESTART_REQUEST : PACKET_HEADER {
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_GAMESTART_RESPONSE : PACKET_HEADER {
	char Token[MAX_JWT_TOKEN_LEN + 1];
};

struct USERINFO_REQUEST : PACKET_HEADER {
	char userId[MAX_USER_ID_LEN + 1];
};

struct USERINFO_RESPONSE : PACKET_HEADER {
	USERINFO UserInfo;
};

struct EQUIPMENT_REQUEST : PACKET_HEADER {

};

struct EQUIPMENT_RESPONSE : PACKET_HEADER {
	uint16_t eqCount;
	char Equipments[MAX_INVEN_SIZE + 1];
};

struct CONSUMABLES_REQUEST : PACKET_HEADER {

};

struct CONSUMABLES_RESPONSE : PACKET_HEADER {
	uint16_t csCount;
	char Consumables[MAX_INVEN_SIZE + 1];
};

struct MATERIALS_REQUEST : PACKET_HEADER {

};

struct MATERIALS_RESPONSE : PACKET_HEADER {
	uint16_t mtCount;
	char Materials[MAX_INVEN_SIZE + 1];
};

struct PASSINFO_REQUEST : PACKET_HEADER {

};

struct PASSINFO_RESPONSE : PACKET_HEADER {
	uint16_t passCount = 0;
};

struct PASSREWARDINFO_REQUEST : PACKET_HEADER {

};

struct PASSREWARDINFO_RESPONSE : PACKET_HEADER {
	uint16_t passRewordCount = 0;
};


//  ---------------------------- USER STATUS  ----------------------------

struct EXP_UP_REQUEST : PACKET_HEADER {
	short mobNum; // Number of Mob
};

struct EXP_UP_RESPONSE : PACKET_HEADER {
	unsigned int currentExp;
	uint16_t increaseLevel;
};

struct LEVEL_UP_RESPONSE : PACKET_HEADER {
	uint16_t increaseLevel;
	unsigned int currentExp;
};

//  ---------------------------- INVENTORY  ----------------------------

struct ADD_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
	uint16_t itemCount; // (Max 99)
	uint16_t itemCode; // (Max 5000)
};

struct ADD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
};

struct DEL_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOD_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
	int8_t itemCount; // (Max 99)
	uint16_t itemCode; // (Max 5000)
};

struct MOD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOV_ITEM_REQUEST : PACKET_HEADER {
	uint16_t ItemType; // (Max 3)

	uint16_t dragItemPos; // (Max 10)
	uint16_t dragItemCode;
	uint16_t dragItemCount; // (Max 99)

	uint16_t targetItemPos; // (Max 10)
	uint16_t targetItemCode;
	uint16_t targetItemCount; // (Max 99)
};

struct MOV_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

//  ---------------------------- INVENTORY:EQUIPMENT  ----------------------------

struct ADD_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
	uint16_t Enhancement; // (Max 20)
	uint16_t itemCode; // (Max 5000)
};

struct ADD_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
};

struct DEL_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct ENH_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
};

struct ENH_EQUIPMENT_RESPONSE : PACKET_HEADER {
	uint16_t Enhancement = 0;
	bool isSuccess;
};

struct MOV_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t currentItemPos;
	uint16_t targetItemPos;
};

struct MOV_EQUIPMENT_RESPONSE : PACKET_HEADER {
	uint16_t checkNum;
};


//  ---------------------------- RAID  ----------------------------

struct RAID_MATCHING_REQUEST : PACKET_HEADER { // Users Matching Request

};

struct RAID_MATCHING_RESPONSE : PACKET_HEADER {
	bool insertSuccess; // Insert Into Matching Queue Check
};

struct RAID_READY_REQUEST : PACKET_HEADER {
	char serverToken[MAX_JWT_TOKEN_LEN + 1]; // Token For Server Connection
	char ip[MAX_IP_LEN + 1];
	uint16_t port;
	uint16_t roomNum;
};

struct RAID_RANKING_REQUEST : PACKET_HEADER {
	uint16_t startRank;
};

struct RAID_RANKING_RESPONSE : PACKET_HEADER {
	uint16_t rkCount;
	char reqScore[MAX_SCORE_SIZE + 1];
};


//  ---------------------------- GAME SERVER  ----------------------------

struct USER_CONNECT_GAME_REQUEST : PACKET_HEADER {
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
	char userId[MAX_USER_ID_LEN + 1];
};

struct USER_CONNECT_GAME_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct RAID_TEAMINFO_REQUEST : PACKET_HEADER {
	sockaddr_in userAddr;
};

struct RAID_TEAMINFO_RESPONSE : PACKET_HEADER {
	char teamId[MAX_USER_ID_LEN + 1];
	uint16_t userLevel;
	uint16_t userRaidServerObjNum;
};

struct RAID_START : PACKET_HEADER {
	std::chrono::time_point<std::chrono::steady_clock> endTime; // 설정한 종료 시간 + 5초 (모든 유저 들어오는 시간 5초 설정)
	unsigned int mobHp;
	uint16_t mapNum;
};

struct RAID_HIT_REQUEST : PACKET_HEADER {
	unsigned int damage;
};

struct RAID_HIT_RESPONSE : PACKET_HEADER {
	unsigned int yourScore;
	unsigned int currentMobHp;
};

struct RAID_END : PACKET_HEADER {

};

struct SEND_RAID_SCORE : PACKET_HEADER {
	unsigned int userScore;
	uint16_t userRaidServerObjNum;
};


enum class PACKET_ID : uint16_t {
	//  ---------------------------- CENTER (1~)  ----------------------------
	
	// SYSTEM (1~)
	USER_CONNECT_REQUEST = 1,
	USER_CONNECT_RESPONSE = 2,
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

	// 유저 캐시 충전 완료 요청 테스트용 (511,512)
	CASH_CHARGE_COMPLETE_REQUEST = 511,
	CASH_CHARGE_COMPLETE_RESPONSE = 512,


	//  ---------------------------- SESSION (801~)  ----------------------------
	
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
	PASSREWARDINFO_REQUEST = 823,
	PASSREWARDINFO_RESPONSE = 824,
	PASSINFO_REQUEST = 825,
	PASSINFO_RESPONSE = 826,

	//  ---------------------------- CHANNEL (1501~)  ----------------------------

	// CHANNEL SERVER (1511~)
	USER_CONNECT_CHANNEL_REQUEST = 1511,
	USER_CONNECT_CHANNEL_RESPONSE = 1512,
	CHANNEL_USER_COUNTS_REQUEST = 1513,
	CHANNEL_USER_COUNTS_RESPONSE = 1514,
	MOVE_CHANNEL_REQUEST = 1515,
	MOVE_CHANNEL_RESPONSE = 1516,
	RAID_RANKING_REQUEST = 1517,
	RAID_RANKING_RESPONSE = 1518,


	// USER STATUS (1521~)
	EXP_UP_REQUEST = 1521,
	EXP_UP_RESPONSE = 1522,
	LEVEL_UP_REQUEST = 1523,
	LEVEL_UP_RESPONSE = 1524,

	// INVENTORY (1525~)
	ADD_ITEM_REQUEST = 1525,
	ADD_ITEM_RESPONSE = 1526,
	DEL_ITEM_REQUEST = 1527,
	DEL_ITEM_RESPONSE = 1528,
	MOD_ITEM_REQUEST = 1529,
	MOD_ITEM_RESPONSE = 1530,
	MOV_ITEM_REQUEST = 1531,
	MOV_ITEM_RESPONSE = 1532,

	// INVENTORY::EQUIPMENT 
	ADD_EQUIPMENT_REQUEST = 1533,
	ADD_EQUIPMENT_RESPONSE = 1534,
	DEL_EQUIPMENT_REQUEST = 1535,
	DEL_EQUIPMENT_RESPONSE = 1536,
	ENH_EQUIPMENT_REQUEST = 1537,
	ENH_EQUIPMENT_RESPONSE = 1538,
	MOV_EQUIPMENT_REQUEST = 1539,
	MOV_EQUIPMENT_RESPONSE = 1540,

	//  ---------------------------- RAID(8001~)  ----------------------------

	USER_CONNECT_GAME_REQUEST = 8005,
	USER_CONNECT_GAME_RESPONSE = 8006,

	RAID_TEAMINFO_REQUEST = 8055,
	RAID_TEAMINFO_RESPONSE = 8056,
	RAID_START = 8057,
	RAID_HIT_REQUEST = 8058,
	RAID_HIT_RESPONSE = 8059,

	RAID_END = 8101,
};