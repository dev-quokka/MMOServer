#pragma once

#define SERVER_IP "127.0.0.1"
#define SERVER_TCP_PORT 9090
#define SESSION_SERVER_PORT 9091
#define SERVER_UDP_PORT 50000
#define PACKET_SIZE 1024

#include "Packet.h"
#include "Define.h"
#include "User.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <thread>
#include <iostream>
#include <string>
#include <vector>

int main() {
    User user;

    if (!user.init()) {
        std::cout << "Connect Fail" << std::endl;
        return 0;
    }

    bool onlineCheck = true;
    bool inServerCheck = false;
    bool inChannelCheck = false;

    while (onlineCheck) { // 서버 이동 페이지
        inChannelCheck = true;

        bool tempServerBool = false; // 서버 인원 수를 한 번만 불러오기 위한 플래그 (서버 이동 페이지에 머무르는 동안 재요청 방지)

        while (1) {
            uint16_t checkServer = user.MoveServer(tempServerBool);
            if (checkServer == 0) { // 서버 입장 실패. 다시 서버 이동 페이지
                tempServerBool = true; 
                continue;
            }
            else if (checkServer == 10) { // 게임 종료
                onlineCheck = false;
                inServerCheck = false;
                inChannelCheck = false;
                break;
            }
            else { // 서버 입장 성공
                inServerCheck = true;
                break;
            }
        }

        bool tempChannelBool = false; // 채널 인원 수를 한 번만 불러오기 위한 플래그 (채널 이동 페이지에 머무르는 동안 재요청 방지)

        while (inServerCheck) {
            uint16_t checkChannel = user.SelectChannel(tempChannelBool);

            if (checkChannel == 0) { // 채널 입장 실패. 다시 채널 이동 페이지
                tempChannelBool = true;
                continue;
            }
            else if (checkChannel == 10) { // 서버 선택 페이지로 돌아가기
                user.ChannelSocketinitialization();
                inChannelCheck = false;
                std::this_thread::sleep_for(std::chrono::seconds(1)); // 서버 이동 전 1초 대기
                break;
            }
            else { // 채널 입장 성공
                inChannelCheck = true;
                break;
            }
        }
        
        while (inChannelCheck) {
            std::cout << std::endl;
            uint16_t select;

            std::cout << "==============================" << std::endl;
            std::cout << "===      1. 내 정보        ===" << std::endl;
            std::cout << "===   2. 몹잡기(경험치)    ===" << std::endl;
            std::cout << "===      3. 인벤토리       ===" << std::endl;
            std::cout << "===      4. 레이드 매칭    ===" << std::endl;
            std::cout << "===      5. 레이드 랭킹    ===" << std::endl;
            std::cout << "===      6. 서버 이동      ===" << std::endl;
            std::cout << "===      7. 채널 이동      ===" << std::endl;
            std::cout << "===      8. 상점 이동      ===" << std::endl;
            std::cout << "===      9. 캐시 충전      ===" << std::endl;
            std::cout << "===     10. 패스 정보      ===" << std::endl;
            std::cout << "===11. 패스미션(패스레벨업)===" << std::endl;
            std::cout << "===     12. 로그아웃       ===" << std::endl;
            std::cout << "==============================" << std::endl;

            std::cin >> select;

            switch (select) {
            case 1: {
                user.GetMyInfo();
                std::cout << std::endl;
                break;
            }
            case 2: {
                user.GetUserLevelExp();
                uint16_t k = 0;
                uint16_t mob = 0;
                while (1) {
                    std::cout << "나가려면 1번 몹을 잡으려면 2번" << std::endl;
                    std::cin >> k;
                    if (k == 2) {
                        std::cout << "경험치 1~5의 몬스터를 잡으려면 번호 입력" << std::endl;
                        std::cin >> mob;
                        if (mob >= 1 && mob <= 5) {
                            user.AddExpFromMob(mob);
                        }
                        else {
                            std::cout << "잘못된 몬스터 번호 ! 다시입력" << std::endl;
                        }
                    }
                    else {
                        break;
                    }
                }
                break;
            }
            case 3: {
                while (1) {
                    int checknum;
                    std::cout << "원하는 인벤토리 번호 누르고 엔터를 눌러주세요." << std::endl;
                    std::cout << "1. 장비 " << "2. 소비 " << "3. 재료 " << "4. 뒤로가기" << std::endl;
                    std::cin >> checknum;

                    if (checknum >= 4) break;
                    user.GetInventory(checknum);
                    std::cout << '\n';
                    std::cout << "1. 슬롯 이동 " << "2. 강화(장비만)" << "3. 획득" << "4. 삭제" << "5. 뒤로가기" << std::endl;
                    int checknum2;
                    std::cin >> checknum2;
                    if (checknum2 >= 5) break;

                    if (checknum2 == 1) {
                        std::cout << "위치를 바꾸고 싶은 아이템과 위치를 적어주세요" << std::endl;
                        int currentpos;
                        int movepos;
                        std::cout << "위치 변경 희망하는 아이템 위치 : "; std::cin >> currentpos;
                        std::cout << "원하는 위치(1~10) : "; std::cin >> movepos;
                        user.MoveItem(checknum, currentpos, movepos);
                    }
                    else if (checknum2 == 2) {
                        if (checknum != 1) {
                            std::cout << "장비만 강화 가능 !" << std::endl;
                            continue;
                        }
                        std::cout << "강화하고 싶은 장비의 슬롯을 입력해주세요." << std::endl;
                        uint16_t currentpos;
                        std::cout << "강화하고 싶은 장비 아이템 위치 : "; std::cin >> currentpos;

                        if (!user.EnhanceEquip(currentpos)) {
                            std::cout << "강화 실패 !" << std::endl;
                        }

                    }
                    else if (checknum2 == 3) {
                        if (checknum == 1) { // 장비 add
                            uint16_t code;
                            uint16_t enhancement;
                            std::cout << "원하는 아이템 코드 : "; std::cin >> code;
                            std::cout << "원하는 아이템 강화 수 : "; std::cin >> enhancement;
                            if (user.AddEquipment(code, enhancement)) {
                                std::cout << "아이템 획득 성공 !" << std::endl;
                            }
                            else {
                                std::cout << "아이템 획득 실패 !" << std::endl;
                            }
                        }
                        else { // 소비, 재료 add
                            uint16_t code;
                            uint16_t count;
                            std::cout << "원하는 아이템 코드 : "; std::cin >> code;
                            std::cout << "원하는 아이템 개수 : "; std::cin >> count;
                            if (user.AddItem(checknum, code, count)) {
                                std::cout << "아이템 획득 성공 !" << std::endl;
                            }
                            else {
                                std::cout << "아이템 획득 실패 !" << std::endl;
                            }
                        }
                    }
                    else if (checknum2 == 4) {
                        std::cout << "삭제하고 싶은 장비의 슬롯을 입력해주세요." << std::endl;
                        uint16_t currentpos;
                        uint16_t check;
                        std::cout << "삭제하고 싶은 장비 아이템 위치 : "; std::cin >> currentpos;
                        std::cout << "정말 삭제할까요 ? 1번 삭제, 2번 뒤로가기"; std::cin >> check;
                        if (check >= 2) break;

                        if (!user.DeleteItem(checknum, currentpos)) {
                            std::cout << "아이템 삭제 실패 !" << std::endl;
                        }
                    }
                    std::cout << std::endl;
                    std::cout << std::endl;
                }
                break;
            }

            case 4:
            {
                std::cout << "레이드 매칭을 시작할까요 ? 1번 시작, 2번 뒤로가기" << std::endl;
                int k;
                std::cin >> k;
                if (k >= 2) break;
                user.RaidStart();
                break;
            }
            case 5: {
                uint16_t startNum = 0;
                uint16_t lastCnt = 0;
                int checknum;
                while (1) {
                    if (user.GetRaidScore(startNum)) { // 아직 뒤에 유저 더 남아있을때
                        if (startNum != 0) {
                            std::cout << "1. 이전 페이지, 2. 다음 페이지, 3. 뒤로가기" << std::endl;
                            std::cin >> checknum;
                            if (checknum == 1) --startNum;
                            else if (checknum == 2) ++startNum;
                            else break;
                        }
                        else { // 첫번째 페이지니까 이전페이지 못하게 하기
                            std::cout << "1. 다음 페이지, 2. 뒤로가기" << std::endl;
                            std::cin >> checknum;
                            if (checknum == 1) ++startNum;
                            else break;
                        }
                    }
                    else { // 뒤에 유저 더 없을때
                        if (lastCnt == 0) lastCnt = startNum; // 마지막 카운트 체크

                        std::cout << "마지막 랭킹 페이지 입니다 !" << std::endl;
                        if (startNum != 0) {
                            std::cout << "1. 이전 페이지, 2. 뒤로가기" << std::endl;
                            std::cin >> checknum;
                            if (checknum == 1) --startNum;
                            else break;
                        }
                        else { // 페이지 하나일때
                            std::cout << "아무 키를 누르면 뒤로가기" << std::endl;
                            std::cin >> checknum;
                            break;
                        }
                    }
                }
                break;
            }
            case 6: {
                user.ChannelSocketinitialization();
                inChannelCheck = false;
                std::this_thread::sleep_for(std::chrono::seconds(1)); // 서버 이동 전 1초 대기
                break;
            }
            case 7: {
                bool tempChannelBool = false; // 채널 인원 수를 한 번만 불러오기 위한 플래그 (채널 이동 페이지에 머무르는 동안 재요청 방지)

                while (1) {
                    uint16_t checkChannel = user.SelectChannel(tempChannelBool);

                    if (checkChannel == 0) { // 채널 입장 실패. 다시 채널 이동 페이지
                        tempChannelBool = true;
                        continue;
                    }
                    else if (checkChannel == 10) { // 서버 선택 페이지로 돌아가기
                        user.ChannelSocketinitialization();
                        inChannelCheck = false;
                        std::this_thread::sleep_for(std::chrono::seconds(1)); // 서버 이동 전 1초 대기
                        break;
                    }
                    else { // 채널 입장 성공
                        inChannelCheck = true;
                        break;
                    }
                }
                break;
            }
            case 8: {
                user.GetShopInfo();
                std::cout << "구매 할 아이템 코드, 사용 기한 or 개수, 타입 입력 (장비:0,소비:1,재료:2)" << '\n';
                std::cout << "뒤로가기 : 0 입력" << '\n';
                int a,b,c ;
                std::cin >> a;
                if (a == 0) break;
                std::cin >> b >> c;

                user.BuyItemFromShop(a, b, c);
                break;
            }
            case 9: {
                std::cout << "충전할 캐시 입력" << '\n';
                std::cout << "뒤로가기 : 0 입력" << '\n';
                uint32_t a;
                std::cin >> a;
                if (a == 0) break;

                user.Test_CashCharge(a);
                break;
            }
            case 10: {
                user.GetPassData();

                std::cout << "획득할 패스 ID, 레벨, 결제 유형 입력 (무료:0, 유료: 1)" << '\n';
                std::cout << "뒤로가기 : 0 입력" << '\n';

                std::string passId;
                uint32_t passLevel, passCurrenyType;
                std::cin >> passId;
                if (passId == "0") break;

                std::cin >> passLevel >> passCurrenyType;

                user.GetPassItem(passId, passLevel, passCurrenyType);

                //auto start = std::chrono::high_resolution_clock::now();

                // user.TEST_GetPassItem(); 클라이언트 1 / 타이머 세팅 20 + 500번

                //auto end = std::chrono::high_resolution_clock::now();
                //auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                //double seconds = dur / 1000000;

                //double tps = 1000 / seconds; // ** 수행 할 횟수 잘 적자

                //std::cout << "[TEST_GetPassItem] tps : " << tps << " : " << seconds << "초" << '\n';

                break;
            }
            case 11: {
                user.GetPassInfo();

                std::cout << "미션 1 : 경험치 1 증가" << '\n';
                std::cout << "미션 2 : 경험치 2 증가" << '\n';
                std::cout << "미션 3 : 경험치 3 증가" << '\n';
                std::cout << "미션 4 : 경험치 4 증가" << '\n';
                std::cout << "미션 5 : 경험치 5 증가" << '\n';
                std::cout << "수행할 미션 번호와 패스 ID 누르기 (뒤로가기 : 0입력)" << '\n';

                int k = 0; std::string passId;
                std::cin >> k;
                if (k == 0 || k > 5) break;
                std::cin >> passId;
                user.GetPassExp(passId, k);
                break;
            }
            case 12: {
                inChannelCheck = false;
                onlineCheck = false;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                break;
            }
            }
        }
    }
    return 0;
}

// ========================== USER STRUCT ==========================

// 1. 웹에서 로그인 하면, 웹 서버는 로그인 체크후에 ui에 보여야할 정보를 전송해준다.
// 2. 게임시작 버튼을 누르면 웹서버에서 mysql로 해당 유저 정보 가져와서 레디스 클러스터에 올려주기 (해당 레디스 정보들은 짧은 ttl 설정하기) 
// 레디스에 해당 유저 UUID로 pk등록해두고 나머지 값들은 userinfo:{pk}, invenory:{pk} 등으로 저장. 

// 위에 까지는 웹에서 처리 됬다고 가정. 게임시작을 누르면 웹서버에서 uuid를 생성해서 레디스 클러스터에 UUID:"userId", "만든uuid", pk값으로 저장해두고
// 서버에서 해당 데이터를 저장후에 바로 삭제 처리. 이제 해당 uuid로 유저와 통신.
// uuid는 유저가 게임 시작 할 때마다 새로 생성해줄거고, 같은 아이디 이중 접속을 막아서 보안성 유지.

// 3. 유저는 항상 본인의 경험치와 레벨 확인이 가능하고, { 장비, 소비, 재료 }로 이루어진 인벤토리를 확인이 가능하다.
// 4. 인벤토리는 아이템 추가, 아이템 삭제, 아이템 이동이 가능하며 장비 아이템은 확장성으로 다른 아이템들과 차별된 packet 전송
// 5. 유저는 총 2명으로 이루어진 2분짜리 레이드가 가능하다.
// 6. 공격기능과 현재 몹 hp를 확인 가능하며, 2분전에 몹을 잡으면 게임이 종료되며 본인 점수와 팀 점수를 확인 할 수 있다.

// 나중에 추가 구현 : 
// 레이드 매칭 중간 취소 및 유저 연결 끊김 처리
// 유저 상태 처리 (게임중, 접속중 등)


// ========================== REDIS CLUSTER SLOTS STRUCT ==========================
// 
// UUID:quokka, "Made UUID" , quokka pk num
// 
// 1. userInfo / pk,id,level,exp,lastlogin
// 2. inventory:equipment / equipment:pk, code:position, enhance...
// 3. inventory:else / else:pk, code:position, count


// If login success, Insert into Redis pk,id, level, exp,last_login

// Level Up Check
//std::pair<uint8_t, unsigned int> InGameUserManager::LevelUp(UINT16 connObjNum_, unsigned int expUp_) {
//	uint8_t currentLevel = inGmaeUser[connObjNum_]->currentLevel;
//	unsigned int currentExp = inGmaeUser[connObjNum_]->currentExp;
//	uint8_t levelUpCnt = 0;
//
//	if (enhanceProbabilities[currentLevel] <= currentExp + expUp_) { // LEVEL UP
//		currentExp += expUp_;
//		while (currentExp >= enhanceProbabilities[currentLevel]) {
//			currentExp -= enhanceProbabilities[currentLevel];
//			currentLevel++;
//			levelUpCnt++;
//		}
//	}
//	else { // Just Exp Up
//		currentLevel = 0;
//		currentExp = currentExp + expUp_;
//	}
//
//	return { levelUpCnt , currentExp }; // Level Up Increase, currenExp
//}

// 이거 클라이언트로 옮겨서 거기서 체크하기
    //try {
    //    auto existUserInfo = redis.exists("user:" + uuidCheck->uuId); // Check If a UserInfo Value Exists
    //    auto existUserInven = redis.exists("user:" + uuidCheck->uuId); // Check If a UserInventory Value Exists

    //    USERINFO_RESPONSE_PACKET urp;
    //    urp.PacketId = (UINT16)PACKET_ID::USERINFO_RESPONSE;
    //    urp.PacketLength = sizeof(USERINFO_RESPONSE_PACKET);

    //    if (existUserInfo > 0 && existUserInven>0) {
    //        /*urp.userInfo = ;
    //        urp.inventory = ;*/
    //        TempConnUser->PushSendMsg(sizeof(USERINFO_RESPONSE_PACKET), (char*)&urp);
    //    }

    //    else TempConnUser->PushSendMsg(sizeof(USERINFO_RESPONSE_PACKET), (char*)&urp); // Send Nullptr If User Does Not Exist In Redis (Connect Fail)
    //}
    //catch (const sw::redis::Error& e) {
    //    std::cerr << "Redis Error: " << e.what() << std::endl;
    //}

//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>
//#include <boost/uuid/uuid_io.hpp>
//#include <iostream>
//#include <sstream>
//
//std::string GenerateCustomUUID(int pk, const std::string& id, int level) {
//    // 랜덤 UUID 생성
//    boost::uuids::random_generator generator;
//    boost::uuids::uuid uuid = generator();
//
//    // UUID와 유저 데이터를 결합
//    std::ostringstream oss;
//    oss << to_string(uuid) << "|pk:" << pk << "|id:" << id << "|level:" << level;
//
//    return oss.str();
//}
//
//int main() {
//    // 유저 데이터
//    int pk = 12345;
//    std::string id = "player123";
//    int level = 10;
//
//    // 커스텀 UUID 생성 및 출력
//    std::string customUUID = GenerateCustomUUID(pk, id, level);
//    std::cout << "Generated Custom UUID: " << customUUID << std::endl;
//
//    return 0;
//}


//<서버쪽 코드>
//void ParseCustomUUID(const std::string& customUUID, int& pk, std::string& id, int& level) {
//    size_t pkPos = customUUID.find("|pk:");
//    size_t idPos = customUUID.find("|id:");
//    size_t levelPos = customUUID.find("|level:");
//
//    if (pkPos == std::string::npos || idPos == std::string::npos || levelPos == std::string::npos) {
//        throw std::runtime_error("Invalid custom UUID format.");
//    }
//
//    // 데이터 추출
//    pk = std::stoi(customUUID.substr(pkPos + 4, idPos - (pkPos + 4)));
//    id = customUUID.substr(idPos + 4, levelPos - (idPos + 4));
//    level = std::stoi(customUUID.substr(levelPos + 7));
//}