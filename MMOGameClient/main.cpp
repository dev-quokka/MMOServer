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

    bool outCheck = true;

    while (outCheck) {
        std::cout << std::endl;
        uint16_t select;

        std::cout << "========================" << std::endl;
        std::cout << "===   1. 내 정보     ===" << std::endl;
        std::cout << "===2. 몹잡기(경험치) ===" << std::endl;
        std::cout << "===   3. 인벤토리    ===" << std::endl;
        std::cout << "===   4. 레이드 매칭 ===" << std::endl;
        std::cout << "===   5. 레이드 랭킹 ===" << std::endl;
        std::cout << "===   6. 로그아웃    ===" << std::endl;
        std::cout << "========================" << std::endl;

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
                    if (checknum==1) { // 장비 add
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
                        if (user.AddItem(checknum,code, count)) {
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
            outCheck = false;
            //user.End();
        }
        }
    }
    return 0;
}