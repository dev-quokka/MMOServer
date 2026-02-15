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

    bool tempServerBool = false; // 서버 인원 수를 한 번만 불러오기 위한 플래그 (서버 이동 페이지에 머무르는 동안 재요청 방지)

    while (onlineCheck) { // 서버 이동 페이지
        inChannelCheck = true;

        //bool tempServerBool = false; // 서버 인원 수를 한 번만 불러오기 위한 플래그 (서버 이동 페이지에 머무르는 동안 재요청 방지)

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
            std::cout << "===      2. 인벤토리       ===" << std::endl;
            std::cout << "===      3. 몬스터 사냥    ===" << std::endl;
            std::cout << "===      4. 레이드 매칭    ===" << std::endl;
            std::cout << "===      5. 레이드 랭킹    ===" << std::endl;
            std::cout << "===      6. 서버 이동      ===" << std::endl;
            std::cout << "===      7. 채널 이동      ===" << std::endl;
            std::cout << "===      8. 상점 이동      ===" << std::endl;
            std::cout << "===      9. 캐시 충전      ===" << std::endl;
            //std::cout << "===     10. 패스 정보      ===" << std::endl;
            //std::cout << "===11. 패스미션(패스레벨업)===" << std::endl;
            std::cout << "===     10. 로그아웃       ===" << std::endl;
            std::cout << "==============================" << std::endl;

            std::cin >> select;
            std::cout << '\n';

            switch (select) {
            case 1: {
                user.GetMyInfo();
                std::cout << std::endl;
                break;
            }
            case 3: {
                user.GetUserLevelExp();
                uint16_t k = 0;
                uint16_t mob = 0;
                while (1) {
                    std::cout << "나가려면 1번, 몹을 잡으려면 2번 입력 ";
                    std::cin >> k;
                    std::cout << '\n';
                    if (k == 2) {
                        std::cout << "경험치 1 ~ 5의 몬스터를 잡으려면 번호 입력 ";
                        std::cin >> mob;
                        std::cout << '\n';
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
            case 2: {
                while (1) {
                    int checknum;
                    std::cout << "원하는 인벤토리 번호 누르고 엔터를 눌러주세요." << std::endl;
                    std::cout << "1. 장비 " << "2. 소비 " << "3. 재료 " << "4. 뒤로가기" << std::endl;
                    std::cin >> checknum;
                    std::cout << '\n';

                    if (checknum >= 4) break;
                    user.GetInventory(checknum);
                    std::cout << '\n';
                    std::cout << "1. 슬롯 이동 " << "2. 강화(장비만)" << "3. 삭제" << "4. 뒤로가기" << std::endl;
                    int checknum2;
                    std::cin >> checknum2;
                    if (checknum2 >= 5) break;

                    if (checknum2 == 1) {
                        std::cout << "위치를 바꾸고 싶은 아이템과 위치를 적어주세요" << std::endl;
                        int currentpos;
                        int movepos;
                        std::cout << "위치 변경 희망하는 아이템 위치 : "; std::cin >> currentpos;
                        std::cout << "원하는 위치(0~9) : "; std::cin >> movepos;
                        user.MoveItem(checknum, currentpos, movepos);
                    }
                    else if (checknum2 == 2) {
                        if (checknum != 1) {
                            std::cout << "장비만 강화 가능 !" << std::endl;
                            continue;
                        }
                        std::cout << "강화하고 싶은 장비의 슬롯을 입력해주세요" << std::endl;
                        uint16_t currentpos;
                        std::cout << "강화하고 싶은 장비 아이템 위치 : "; std::cin >> currentpos;

                        if (!user.EnhanceEquip(currentpos)) {
                            std::cout << "강화 실패 !" << std::endl;
                        }

                    }
                    //else if (checknum2 == 3) {
                    //    if (checknum == 1) { // 장비 add
                    //        uint16_t code;
                    //        uint16_t enhancement;
                    //        std::cout << "원하는 아이템 코드 : "; std::cin >> code;
                    //        std::cout << "원하는 아이템 강화 수 : "; std::cin >> enhancement;
                    //        if (user.AddEquipment(code, enhancement)) {
                    //            std::cout << "아이템 획득 성공 !" << std::endl;
                    //        }
                    //        else {
                    //            std::cout << "아이템 획득 실패 !" << std::endl;
                    //        }
                    //    }
                    //    else { // 소비, 재료 add
                    //        uint16_t code;
                    //        uint16_t count;
                    //        std::cout << "원하는 아이템 코드 : "; std::cin >> code;
                    //        std::cout << "원하는 아이템 개수 : "; std::cin >> count;
                    //        if (user.AddItem(checknum, code, count)) {
                    //            std::cout << "아이템 획득 성공 !" << std::endl;
                    //        }
                    //        else {
                    //            std::cout << "아이템 획득 실패 !" << std::endl;
                    //        }
                    //    }
                    //}
                    else if (checknum2 == 3) {
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
                    else break;
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
                std::cout << '\n';
                std::cout << "구매 할 아이템의 번호를 입력해주세요 (뒤로가기 : 0) : ";
                int idx_;
                std::cin >> idx_;
                if (idx_ == 0) break;

                user.BuyItemFromShop(idx_-1);
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
            //case 10: {
            //    user.GetPassData();

            //    std::cout << "획득할 패스 ID, 레벨, 결제 유형 입력 (무료:0, 유료: 1)" << '\n';
            //    std::cout << "뒤로가기 : 0 입력" << '\n';

            //    std::string passId;
            //    uint32_t passLevel, passCurrenyType;
            //    std::cin >> passId;
            //    if (passId == "0") break;

            //    std::cin >> passLevel >> passCurrenyType;

            //    user.GetPassItem(passId, passLevel, passCurrenyType);

            //    //auto start = std::chrono::high_resolution_clock::now();

            //    // user.TEST_GetPassItem(); 클라이언트 1 / 타이머 세팅 20 + 500번

            //    //auto end = std::chrono::high_resolution_clock::now();
            //    //auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            //    //double seconds = dur / 1000000;

            //    //double tps = 1000 / seconds; // ** 수행 할 횟수 잘 적자

            //    //std::cout << "[TEST_GetPassItem] tps : " << tps << " : " << seconds << "초" << '\n';

            //    break;
            //}
            //case 11: {
            //    user.GetPassInfo();

            //    std::cout << "미션 1 : 경험치 1 증가" << '\n';
            //    std::cout << "미션 2 : 경험치 2 증가" << '\n';
            //    std::cout << "미션 3 : 경험치 3 증가" << '\n';
            //    std::cout << "미션 4 : 경험치 4 증가" << '\n';
            //    std::cout << "미션 5 : 경험치 5 증가" << '\n';
            //    std::cout << "수행할 미션 번호와 패스 ID 누르기 (뒤로가기 : 0입력)" << '\n';

            //    int k = 0; std::string passId;
            //    std::cin >> k;
            //    if (k == 0 || k > 5) break;
            //    std::cin >> passId;
            //    user.GetPassExp(passId, k);
            //    break;
            //}
            case 10: {
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
