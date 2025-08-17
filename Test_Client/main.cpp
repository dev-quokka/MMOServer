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

    while (onlineCheck) { // ���� �̵� ������
        inChannelCheck = true;

        bool tempServerBool = false; // ���� �ο� ���� �� ���� �ҷ����� ���� �÷��� (���� �̵� �������� �ӹ����� ���� ���û ����)

        while (1) {
            uint16_t checkServer = user.MoveServer(tempServerBool);
            if (checkServer == 0) { // ���� ���� ����. �ٽ� ���� �̵� ������
                tempServerBool = true; 
                continue;
            }
            else if (checkServer == 10) { // ���� ����
                onlineCheck = false;
                inServerCheck = false;
                inChannelCheck = false;
                break;
            }
            else { // ���� ���� ����
                inServerCheck = true;
                break;
            }
        }

        bool tempChannelBool = false; // ä�� �ο� ���� �� ���� �ҷ����� ���� �÷��� (ä�� �̵� �������� �ӹ����� ���� ���û ����)

        while (inServerCheck) {
            uint16_t checkChannel = user.SelectChannel(tempChannelBool);

            if (checkChannel == 0) { // ä�� ���� ����. �ٽ� ä�� �̵� ������
                tempChannelBool = true;
                continue;
            }
            else if (checkChannel == 10) { // ���� ���� �������� ���ư���
                user.ChannelSocketinitialization();
                inChannelCheck = false;
                std::this_thread::sleep_for(std::chrono::seconds(1)); // ���� �̵� �� 1�� ���
                break;
            }
            else { // ä�� ���� ����
                inChannelCheck = true;
                break;
            }
        }
        
        while (inChannelCheck) {
            std::cout << std::endl;
            uint16_t select;

            std::cout << "==============================" << std::endl;
            std::cout << "===      1. �� ����        ===" << std::endl;
            std::cout << "===   2. �����(����ġ)    ===" << std::endl;
            std::cout << "===      3. �κ��丮       ===" << std::endl;
            std::cout << "===      4. ���̵� ��Ī    ===" << std::endl;
            std::cout << "===      5. ���̵� ��ŷ    ===" << std::endl;
            std::cout << "===      6. ���� �̵�      ===" << std::endl;
            std::cout << "===      7. ä�� �̵�      ===" << std::endl;
            std::cout << "===      8. ���� �̵�      ===" << std::endl;
            std::cout << "===      9. ĳ�� ����      ===" << std::endl;
            std::cout << "===     10. �н� ����      ===" << std::endl;
            std::cout << "===11. �н��̼�(�н�������)===" << std::endl;
            std::cout << "===     12. �α׾ƿ�       ===" << std::endl;
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
                    std::cout << "�������� 1�� ���� �������� 2��" << std::endl;
                    std::cin >> k;
                    if (k == 2) {
                        std::cout << "����ġ 1~5�� ���͸� �������� ��ȣ �Է�" << std::endl;
                        std::cin >> mob;
                        if (mob >= 1 && mob <= 5) {
                            user.AddExpFromMob(mob);
                        }
                        else {
                            std::cout << "�߸��� ���� ��ȣ ! �ٽ��Է�" << std::endl;
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
                    std::cout << "���ϴ� �κ��丮 ��ȣ ������ ���͸� �����ּ���." << std::endl;
                    std::cout << "1. ��� " << "2. �Һ� " << "3. ��� " << "4. �ڷΰ���" << std::endl;
                    std::cin >> checknum;

                    if (checknum >= 4) break;
                    user.GetInventory(checknum);
                    std::cout << '\n';
                    std::cout << "1. ���� �̵� " << "2. ��ȭ(���)" << "3. ȹ��" << "4. ����" << "5. �ڷΰ���" << std::endl;
                    int checknum2;
                    std::cin >> checknum2;
                    if (checknum2 >= 5) break;

                    if (checknum2 == 1) {
                        std::cout << "��ġ�� �ٲٰ� ���� �����۰� ��ġ�� �����ּ���" << std::endl;
                        int currentpos;
                        int movepos;
                        std::cout << "��ġ ���� ����ϴ� ������ ��ġ : "; std::cin >> currentpos;
                        std::cout << "���ϴ� ��ġ(1~10) : "; std::cin >> movepos;
                        user.MoveItem(checknum, currentpos, movepos);
                    }
                    else if (checknum2 == 2) {
                        if (checknum != 1) {
                            std::cout << "��� ��ȭ ���� !" << std::endl;
                            continue;
                        }
                        std::cout << "��ȭ�ϰ� ���� ����� ������ �Է����ּ���." << std::endl;
                        uint16_t currentpos;
                        std::cout << "��ȭ�ϰ� ���� ��� ������ ��ġ : "; std::cin >> currentpos;

                        if (!user.EnhanceEquip(currentpos)) {
                            std::cout << "��ȭ ���� !" << std::endl;
                        }

                    }
                    else if (checknum2 == 3) {
                        if (checknum == 1) { // ��� add
                            uint16_t code;
                            uint16_t enhancement;
                            std::cout << "���ϴ� ������ �ڵ� : "; std::cin >> code;
                            std::cout << "���ϴ� ������ ��ȭ �� : "; std::cin >> enhancement;
                            if (user.AddEquipment(code, enhancement)) {
                                std::cout << "������ ȹ�� ���� !" << std::endl;
                            }
                            else {
                                std::cout << "������ ȹ�� ���� !" << std::endl;
                            }
                        }
                        else { // �Һ�, ��� add
                            uint16_t code;
                            uint16_t count;
                            std::cout << "���ϴ� ������ �ڵ� : "; std::cin >> code;
                            std::cout << "���ϴ� ������ ���� : "; std::cin >> count;
                            if (user.AddItem(checknum, code, count)) {
                                std::cout << "������ ȹ�� ���� !" << std::endl;
                            }
                            else {
                                std::cout << "������ ȹ�� ���� !" << std::endl;
                            }
                        }
                    }
                    else if (checknum2 == 4) {
                        std::cout << "�����ϰ� ���� ����� ������ �Է����ּ���." << std::endl;
                        uint16_t currentpos;
                        uint16_t check;
                        std::cout << "�����ϰ� ���� ��� ������ ��ġ : "; std::cin >> currentpos;
                        std::cout << "���� �����ұ�� ? 1�� ����, 2�� �ڷΰ���"; std::cin >> check;
                        if (check >= 2) break;

                        if (!user.DeleteItem(checknum, currentpos)) {
                            std::cout << "������ ���� ���� !" << std::endl;
                        }
                    }
                    std::cout << std::endl;
                    std::cout << std::endl;
                }
                break;
            }

            case 4:
            {
                std::cout << "���̵� ��Ī�� �����ұ�� ? 1�� ����, 2�� �ڷΰ���" << std::endl;
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
                    if (user.GetRaidScore(startNum)) { // ���� �ڿ� ���� �� ����������
                        if (startNum != 0) {
                            std::cout << "1. ���� ������, 2. ���� ������, 3. �ڷΰ���" << std::endl;
                            std::cin >> checknum;
                            if (checknum == 1) --startNum;
                            else if (checknum == 2) ++startNum;
                            else break;
                        }
                        else { // ù��° �������ϱ� ���������� ���ϰ� �ϱ�
                            std::cout << "1. ���� ������, 2. �ڷΰ���" << std::endl;
                            std::cin >> checknum;
                            if (checknum == 1) ++startNum;
                            else break;
                        }
                    }
                    else { // �ڿ� ���� �� ������
                        if (lastCnt == 0) lastCnt = startNum; // ������ ī��Ʈ üũ

                        std::cout << "������ ��ŷ ������ �Դϴ� !" << std::endl;
                        if (startNum != 0) {
                            std::cout << "1. ���� ������, 2. �ڷΰ���" << std::endl;
                            std::cin >> checknum;
                            if (checknum == 1) --startNum;
                            else break;
                        }
                        else { // ������ �ϳ��϶�
                            std::cout << "�ƹ� Ű�� ������ �ڷΰ���" << std::endl;
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
                std::this_thread::sleep_for(std::chrono::seconds(1)); // ���� �̵� �� 1�� ���
                break;
            }
            case 7: {
                bool tempChannelBool = false; // ä�� �ο� ���� �� ���� �ҷ����� ���� �÷��� (ä�� �̵� �������� �ӹ����� ���� ���û ����)

                while (1) {
                    uint16_t checkChannel = user.SelectChannel(tempChannelBool);

                    if (checkChannel == 0) { // ä�� ���� ����. �ٽ� ä�� �̵� ������
                        tempChannelBool = true;
                        continue;
                    }
                    else if (checkChannel == 10) { // ���� ���� �������� ���ư���
                        user.ChannelSocketinitialization();
                        inChannelCheck = false;
                        std::this_thread::sleep_for(std::chrono::seconds(1)); // ���� �̵� �� 1�� ���
                        break;
                    }
                    else { // ä�� ���� ����
                        inChannelCheck = true;
                        break;
                    }
                }
                break;
            }
            case 8: {
                user.GetShopInfo();
                std::cout << "���� �� ������ �ڵ�, ��� ���� or ����, Ÿ�� �Է� (���:0,�Һ�:1,���:2)" << '\n';
                std::cout << "�ڷΰ��� : 0 �Է�" << '\n';
                int a,b,c ;
                std::cin >> a;
                if (a == 0) break;
                std::cin >> b >> c;

                user.BuyItemFromShop(a, b, c);
                break;
            }
            case 9: {
                std::cout << "������ ĳ�� �Է�" << '\n';
                std::cout << "�ڷΰ��� : 0 �Է�" << '\n';
                uint32_t a;
                std::cin >> a;
                if (a == 0) break;

                user.Test_CashCharge(a);
                break;
            }
            case 10: {
                user.GetPassData();

                std::cout << "ȹ���� �н� ID, ����, ���� ���� �Է� (����:0, ����: 1)" << '\n';
                std::cout << "�ڷΰ��� : 0 �Է�" << '\n';

                std::string passId;
                uint32_t passLevel, passCurrenyType;
                std::cin >> passId;
                if (passId == "0") break;

                std::cin >> passLevel >> passCurrenyType;

                user.GetPassItem(passId, passLevel, passCurrenyType);

                //auto start = std::chrono::high_resolution_clock::now();

                // user.TEST_GetPassItem(); Ŭ���̾�Ʈ 1 / Ÿ�̸� ���� 20 + 500��

                //auto end = std::chrono::high_resolution_clock::now();
                //auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                //double seconds = dur / 1000000;

                //double tps = 1000 / seconds; // ** ���� �� Ƚ�� �� ����

                //std::cout << "[TEST_GetPassItem] tps : " << tps << " : " << seconds << "��" << '\n';

                break;
            }
            case 11: {
                user.GetPassInfo();

                std::cout << "�̼� 1 : ����ġ 1 ����" << '\n';
                std::cout << "�̼� 2 : ����ġ 2 ����" << '\n';
                std::cout << "�̼� 3 : ����ġ 3 ����" << '\n';
                std::cout << "�̼� 4 : ����ġ 4 ����" << '\n';
                std::cout << "�̼� 5 : ����ġ 5 ����" << '\n';
                std::cout << "������ �̼� ��ȣ�� �н� ID ������ (�ڷΰ��� : 0�Է�)" << '\n';

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

// 1. ������ �α��� �ϸ�, �� ������ �α��� üũ�Ŀ� ui�� �������� ������ �������ش�.
// 2. ���ӽ��� ��ư�� ������ ���������� mysql�� �ش� ���� ���� �����ͼ� ���� Ŭ�����Ϳ� �÷��ֱ� (�ش� ���� �������� ª�� ttl �����ϱ�) 
// ���𽺿� �ش� ���� UUID�� pk����صΰ� ������ ������ userinfo:{pk}, invenory:{pk} ������ ����. 

// ���� ������ ������ ó�� ��ٰ� ����. ���ӽ����� ������ ���������� uuid�� �����ؼ� ���� Ŭ�����Ϳ� UUID:"userId", "����uuid", pk������ �����صΰ�
// �������� �ش� �����͸� �����Ŀ� �ٷ� ���� ó��. ���� �ش� uuid�� ������ ���.
// uuid�� ������ ���� ���� �� ������ ���� �������ٰŰ�, ���� ���̵� ���� ������ ���Ƽ� ���ȼ� ����.

// 3. ������ �׻� ������ ����ġ�� ���� Ȯ���� �����ϰ�, { ���, �Һ�, ��� }�� �̷���� �κ��丮�� Ȯ���� �����ϴ�.
// 4. �κ��丮�� ������ �߰�, ������ ����, ������ �̵��� �����ϸ� ��� �������� Ȯ�强���� �ٸ� �����۵�� ������ packet ����
// 5. ������ �� 2������ �̷���� 2��¥�� ���̵尡 �����ϴ�.
// 6. ���ݱ�ɰ� ���� �� hp�� Ȯ�� �����ϸ�, 2������ ���� ������ ������ ����Ǹ� ���� ������ �� ������ Ȯ�� �� �� �ִ�.

// ���߿� �߰� ���� : 
// ���̵� ��Ī �߰� ��� �� ���� ���� ���� ó��
// ���� ���� ó�� (������, ������ ��)


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

// �̰� Ŭ���̾�Ʈ�� �Űܼ� �ű⼭ üũ�ϱ�
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
//    // ���� UUID ����
//    boost::uuids::random_generator generator;
//    boost::uuids::uuid uuid = generator();
//
//    // UUID�� ���� �����͸� ����
//    std::ostringstream oss;
//    oss << to_string(uuid) << "|pk:" << pk << "|id:" << id << "|level:" << level;
//
//    return oss.str();
//}
//
//int main() {
//    // ���� ������
//    int pk = 12345;
//    std::string id = "player123";
//    int level = 10;
//
//    // Ŀ���� UUID ���� �� ���
//    std::string customUUID = GenerateCustomUUID(pk, id, level);
//    std::cout << "Generated Custom UUID: " << customUUID << std::endl;
//
//    return 0;
//}


//<������ �ڵ�>
//void ParseCustomUUID(const std::string& customUUID, int& pk, std::string& id, int& level) {
//    size_t pkPos = customUUID.find("|pk:");
//    size_t idPos = customUUID.find("|id:");
//    size_t levelPos = customUUID.find("|level:");
//
//    if (pkPos == std::string::npos || idPos == std::string::npos || levelPos == std::string::npos) {
//        throw std::runtime_error("Invalid custom UUID format.");
//    }
//
//    // ������ ����
//    pk = std::stoi(customUUID.substr(pkPos + 4, idPos - (pkPos + 4)));
//    id = customUUID.substr(idPos + 4, levelPos - (idPos + 4));
//    level = std::stoi(customUUID.substr(levelPos + 7));
//}