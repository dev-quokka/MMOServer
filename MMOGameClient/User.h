#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

#include "Packet.h"
#include "Define.h"

#pragma comment(lib, "ws2_32.lib") // 비주얼에서 소켓프로그래밍 하기 위한 것

const uint16_t INVENTORY_SIZE = 11; // 10개면 +1해서 11개로 해두기

class User {
public:
    ~User() {
        closesocket(userSkt);
        WSACleanup();
    }

    bool init() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        sessionSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sessionSkt == INVALID_SOCKET) {
            std::cout << "Server Socket Make Fail" << std::endl;
            return false;
        }

        SOCKADDR_IN addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SESSION_SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &addr.sin_addr.s_addr);

        std::cout << "Session Server Connecting..." << std::endl;

        if (connect(sessionSkt, (SOCKADDR*)&addr, sizeof(addr))) {
            std::cout << "Session Server Connect Fail" << std::endl;
            return false;
        }
        std::cout << "Session Server Connect Success" << std::endl;

        memset(recvBuffer, 0, PACKET_SIZE);

        USERINFO_REQUEST uiReq;
        uiReq.PacketId = (UINT16)SESSIONPACKET_ID::USERINFO_REQUEST;
        uiReq.PacketLength = sizeof(USERINFO_REQUEST);
        strncpy_s(uiReq.userId, userId.c_str(), MAX_USER_ID_LEN);

        send(sessionSkt, (char*)&uiReq, sizeof(uiReq), 0); // 유저 정보 요청
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // 유저 정보

        auto uiPacket = reinterpret_cast<USERINFO_RESPONSE*>(recvBuffer);
        USERINFO tempU = uiPacket->UserInfo;
        exp = tempU.exp;
        level = tempU.level;
        raidScore = tempU.raidScore;

        if (tempU.level == 0) {
            std::cout << "Get Userinfo Fail" << std::endl;
            return false;
        }
        std::cout << "Get Userinfo Success" << std::endl;

        EQUIPMENT_REQUEST eqReq;
        eqReq.PacketId = (UINT16)SESSIONPACKET_ID::EQUIPMENT_REQUEST;
        eqReq.PacketLength = sizeof(EQUIPMENT_REQUEST);

        send(sessionSkt, (char*)&eqReq, sizeof(eqReq), 0); // 장비 정보 요청
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // 장비 정보

        auto eqPacket = reinterpret_cast<EQUIPMENT_RESPONSE*>(recvBuffer);
        char* ptr = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);

        for (int i = 1; i < INVENTORY_SIZE; i++) {
            eq[i].position = i;
        }

        for (int i = 0; i < eqPacket->eqCount; i++) {
            EQUIPMENT tempE;
            memcpy((char*)&tempE, ptr, sizeof(EQUIPMENT));
            eq[tempE.position] = tempE;
            ptr += sizeof(EQUIPMENT);
        }

        std::cout << "Get EQUIPMENT Success" << std::endl;

        CONSUMABLES_REQUEST csReq;
        csReq.PacketId = (UINT16)SESSIONPACKET_ID::CONSUMABLES_REQUEST;
        csReq.PacketLength = sizeof(CONSUMABLES_REQUEST);

        send(sessionSkt, (char*)&csReq, sizeof(csReq), 0);
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // 소비 정보

        auto csPacket = reinterpret_cast<CONSUMABLES_RESPONSE*>(recvBuffer);

        std::vector<CONSUMABLES> tempCs(csPacket->csCount);
        char* ptr2 = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);

        for (int i = 1; i < INVENTORY_SIZE; i++) {
            cs[i].position = i;
        }

        for (int i = 0; i < eqPacket->eqCount; i++) {
            CONSUMABLES tempCon;
            memcpy((char*)&tempCon, ptr2, sizeof(CONSUMABLES));
            cs[tempCon.position] = tempCon;
            ptr2 += sizeof(CONSUMABLES);
        }

        std::cout << "Get CONSUMABLES Success" << std::endl;

        MATERIALS_REQUEST mtReq;
        mtReq.PacketId = (UINT16)SESSIONPACKET_ID::MATERIALS_REQUEST;
        mtReq.PacketLength = sizeof(MATERIALS_REQUEST);

        send(sessionSkt, (char*)&mtReq, sizeof(mtReq), 0);
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // 재료 정보

        auto mtPacket = reinterpret_cast<MATERIALS_RESPONSE*>(recvBuffer);

        std::vector<MATERIALS> tempMt(mtPacket->mtCount);
        char* ptr3 = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);

        for (int i = 1; i < INVENTORY_SIZE; i++) {
            mt[i].position = i;
        }

        for (int i = 0; i < eqPacket->eqCount; i++) {
            MATERIALS tempM;
            memcpy((char*)&tempM, ptr3, sizeof(MATERIALS));
            mt[tempM.position] = tempM;
            ptr3 += sizeof(MATERIALS);
        }

        //if (mt.empty()) {
        //    std::cout << "Get MATERIALS Fail" << std::endl;
        //    return false;
        //}

        std::cout << "Get MATERIALS Success" << std::endl;

        USER_GAMESTART_REQUEST ugReq;
        ugReq.PacketId = (UINT16)SESSIONPACKET_ID::USER_GAMESTART_REQUEST;
        ugReq.PacketLength = sizeof(USER_GAMESTART_REQUEST);
        strncpy_s(ugReq.userId, userId.c_str(), MAX_USER_ID_LEN);

        send(sessionSkt, (char*)&ugReq, sizeof(ugReq), 0); // 게임 시작 준비 요청
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // 게임 시작을 위한 웹 토큰

        auto ucReqPacket = reinterpret_cast<USER_GAMESTART_RESPONSE*>(recvBuffer);

        std::string Token = ucReqPacket->Token;

        if (Token == "") { // 세션 서버에서 토큰 생성 실패
            std::cout << "Get Token Fail" << std::endl;
            return false;
        }

        std::cout << "Connect Success" << std::endl;

        shutdown(sessionSkt, SD_BOTH);
        closesocket(sessionSkt); // 세션 서버 소켓 닫기

        std::cout << "If you Press 1, game start. If you want out, press any key" << std::endl;

        int k = 0;
        std::cin >> k;
        if (k != 1) exit(0);

        userSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SERVER_TCP_PORT);
        inet_pton(AF_INET, SERVER_IP, &addr.sin_addr.s_addr);

        std::cout << "Quokka Server Connecting..." << std::endl;

        if (connect(userSkt, (SOCKADDR*)&addr, sizeof(addr))) {
            std::cout << "Quokka Server Connect Fail" << std::endl;
            return false;
        }

        USER_CONNECT_REQUEST_PACKET ucReq;
        ucReq.PacketId = (UINT16)PACKET_ID::USER_CONNECT_REQUEST;
        ucReq.PacketLength = sizeof(USER_CONNECT_REQUEST_PACKET);
        strncpy_s(ucReq.userId, userId.c_str(), MAX_USER_ID_LEN);
        strncpy_s(ucReq.userToken, Token.c_str(), MAX_JWT_TOKEN_LEN);

        std::cout << "Connect Requset To Game Server.." << std::endl;

        send(userSkt, (char*)&ucReq, sizeof(ucReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto ucResPacket = reinterpret_cast<USER_CONNECT_RESPONSE_PACKET*>(recvBuffer);

        if (ucResPacket->isSuccess == false) return false;

        std::cout << "Connect Success In Game Server" << std::endl;

        std::cout << userId << " 게임 접속 성공 !" << std::endl;
    }

    bool makeUDPSocket() {
        udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (udpSocket == INVALID_SOCKET) {
            std::cout << "Udp Socket Make Fail Error : " << WSAGetLastError() << std::endl;
            return false;
        }

        udpAddr.sin_family = AF_INET;
        udpAddr.sin_port = htons(SERVER_UDP_PORT);
        udpAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        bind(udpSocket, (sockaddr*)&udpAddr, sizeof(udpAddr));

        std::cout << "Udp Socket Make Success" << std::endl;

        return true;
    }

    void GetMyInfo() {
        std::cout << "아이디 : " << userId << std::endl;
        std::cout << "레벨 : " << level << std::endl;
        std::cout << "경험치 : " << exp << std::endl;
        std::cout << "레이드 최고 점수 : " << raidScore << std::endl;
    }

    void AddExpFromMob(uint16_t mobNum_) {
        EXP_UP_REQUEST euReq;
        euReq.PacketId = (UINT16)PACKET_ID::EXP_UP_REQUEST;
        euReq.PacketLength = sizeof(EXP_UP_REQUEST);
        euReq.mobNum = mobNum_;

        send(userSkt, (char*)&euReq, sizeof(euReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto ucResPacket = reinterpret_cast<EXP_UP_RESPONSE*>(recvBuffer);

        if (ucResPacket->increaseLevel == 0) { // Only Exp Up
            exp = ucResPacket->currentExp;
            std::cout << mobNum_ << " 몬스터를 잡았습니다 !" << std::endl;
            std::cout << "현재 레벨 : " << level.load() << std::endl;
            std::cout << "현재 경험치 : " << exp << std::endl;
        }
        else { // Level up
            level.fetch_add(ucResPacket->increaseLevel);
            exp = ucResPacket->currentExp;
            std::cout << mobNum_ << " 몬스터를 잡았습니다 !" << std::endl;
            std::cout << "레벨업 했습니다 !" << std::endl;
            std::cout << "현재 레벨 : " << level.load() << std::endl;
            std::cout << "현재 경험치 : " << exp << std::endl;
        }
    }

    std::pair<uint16_t, unsigned int> GetUserLevelExp() {
        std::cout << "현재 레벨 : " << level.load() << std::endl;
        std::cout << "현재 경험치 : " << exp.load() << std::endl;
        return { level, exp };
    }

    void GetInventory(uint16_t invenNum_) {
        if (invenNum_ == 1) {
            std::cout << "장비 인벤토리" << std::endl;
            for (int i = 0; i < eq.size(); i++) {
                if (eq[i].itemCode == 0) continue;
                std::cout << eq[i].position << "번 위치에 +" << eq[i].enhance << "강화 되어있는 " << eq[i].itemCode << "번 아이템 존재" << std::endl;
            }
        }
        else if (invenNum_ == 2) {
            std::cout << "소비 인벤토리" << std::endl;
            for (int i = 0; i < eq.size(); i++) {
                if (cs[i].itemCode == 0) continue;
                std::cout << cs[i].position << "번 위치에 " << cs[i].itemCode << "번 아이템 " << cs[i].count << "개 존재" << std::endl;
            }
        }
        else if (invenNum_ == 3) {
            std::cout << "재료 인벤토리" << std::endl;
            for (int i = 0; i < eq.size(); i++) {
                if (mt[i].itemCode == 0) continue;
                std::cout << mt[i].position << "번 위치에 " << mt[i].itemCode << "번 아이템 " << mt[i].count << "개 존재" << std::endl;
            }
        }
        return;
    }

    bool MoveItem(uint16_t invenNum_, uint16_t currentpos_, uint16_t movepos_) {
        if (invenNum_ == 1) { // 장비
            MOV_EQUIPMENT_REQUEST miReq;
            miReq.PacketId = (UINT16)PACKET_ID::MOV_EQUIPMENT_REQUEST;
            miReq.PacketLength = sizeof(MOV_EQUIPMENT_REQUEST);

            EQUIPMENT currentE = eq[currentpos_];
            EQUIPMENT moveE = eq[movepos_];

            miReq.dragItemCode = moveE.itemCode;
            miReq.dragItemEnhancement = moveE.enhance;
            miReq.dragItemPos = currentE.position;
            miReq.targetItemCode = currentE.itemCode;
            miReq.targetItemEnhancement = currentE.enhance;
            miReq.targetItemPos = moveE.position;

            send(userSkt, (char*)&miReq, sizeof(miReq), 0);
            recv(userSkt, recvBuffer, PACKET_SIZE, 0);

            auto miResPacket = reinterpret_cast<MOV_EQUIPMENT_RESPONSE*>(recvBuffer);

            if (!miResPacket->isSuccess) return false;

            moveE.position = miReq.dragItemPos;
            currentE.position = miReq.targetItemPos;

            eq[currentpos_] = moveE;
            eq[movepos_] = currentE;

            return true;
        }
        else { // 소비 or 재료
            MOV_ITEM_REQUEST miReq;
            miReq.PacketId = (UINT16)PACKET_ID::MOV_ITEM_REQUEST;
            miReq.PacketLength = sizeof(MOV_ITEM_REQUEST);

            if (invenNum_ == 2) { // 소비
                CONSUMABLES currentC = cs[currentpos_];
                CONSUMABLES moveC = cs[movepos_];

                miReq.ItemType = invenNum_ - 1;
                miReq.dragItemCode = moveC.itemCode;
                miReq.dragItemCount = moveC.count;
                miReq.dragItemPos = currentC.position;
                miReq.targetItemCode = currentC.itemCode;
                miReq.targetItemCount = currentC.count;
                miReq.targetItemPos = moveC.position;

                send(userSkt, (char*)&miReq, sizeof(miReq), 0);
                recv(userSkt, recvBuffer, PACKET_SIZE, 0);

                auto miResPacket = reinterpret_cast<MOV_ITEM_RESPONSE*>(recvBuffer);

                if (!miResPacket->isSuccess) return false;

                moveC.position = miReq.dragItemPos;
                currentC.position = miReq.targetItemPos;

                cs[currentpos_] = moveC;
                cs[movepos_] = currentC;

                return true;
            }
            else if (invenNum_ == 3) {
                MATERIALS currentM = mt[currentpos_];
                MATERIALS moveM = mt[movepos_];

                miReq.ItemType = invenNum_ - 1;
                miReq.dragItemCode = moveM.itemCode;
                miReq.dragItemCount = moveM.count;
                miReq.dragItemPos = currentM.position;
                miReq.targetItemCode = currentM.itemCode;
                miReq.targetItemCount = currentM.count;
                miReq.targetItemPos = moveM.position;

                send(userSkt, (char*)&miReq, sizeof(miReq), 0);
                recv(userSkt, recvBuffer, PACKET_SIZE, 0);

                auto miResPacket = reinterpret_cast<MOV_ITEM_RESPONSE*>(recvBuffer);

                if (!miResPacket->isSuccess) return false;

                moveM.position = miReq.dragItemPos;
                currentM.position = miReq.targetItemPos;

                mt[currentpos_] = moveM;
                mt[movepos_] = currentM;

                return true;
            }
        }
    }

    bool AddEquipment(uint16_t itemCode_, uint16_t enhancement_) {
        ADD_EQUIPMENT_REQUEST aeReq;
        aeReq.PacketId = (UINT16)PACKET_ID::ADD_EQUIPMENT_REQUEST;
        aeReq.PacketLength = sizeof(ADD_EQUIPMENT_REQUEST);

        uint16_t addPosition = INVENTORY_SIZE + 1;

        for (int i = 1; i < INVENTORY_SIZE; i++) {
            if (eq[i].itemCode == 0) {
                addPosition = i;
                break;
            }
        }

        if (addPosition == INVENTORY_SIZE + 1) { // 넣을 공간 없으면 false 반환
            std::cout << "Equipments Full" << std::endl;
            return false;
        }

        aeReq.itemCode = itemCode_;
        aeReq.itemPosition = addPosition;
        aeReq.Enhancement = enhancement_;

        send(userSkt, (char*)&aeReq, sizeof(aeReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto miResPacket = reinterpret_cast<ADD_EQUIPMENT_RESPONSE*>(recvBuffer);

        if (!miResPacket->isSuccess) return false;

        EQUIPMENT tempE;
        tempE.itemCode = itemCode_;
        tempE.position = addPosition;
        tempE.enhance = enhancement_;

        eq[addPosition] = tempE;

        return true;
    }

    bool AddItem(uint16_t invenNum_, uint16_t itemCode_, uint16_t count_) {
        ADD_ITEM_REQUEST aiReq;
        aiReq.PacketId = (UINT16)PACKET_ID::ADD_ITEM_REQUEST;
        aiReq.PacketLength = sizeof(ADD_ITEM_REQUEST);

        if (invenNum_ == 2) { // 소비
            uint16_t addPosition = 0;
            uint16_t addCount = count_;

            for (int i = 1; i < INVENTORY_SIZE; i++) {
                if (cs[i].itemCode == itemCode_) {
                    addPosition = i;
                    addCount += cs[i].count;
                    break;
                }
            }

            if (count_ == addCount) {
                for (int i = 1; i < INVENTORY_SIZE; i++) {
                    if (cs[i].itemCode == 0) {
                        addPosition = i;
                        break;
                    }
                }
            }

            if (addPosition == 0) { // 넣을 공간 없으면 false 반환
                std::cout << "Consumables Full" << std::endl;
                return false;
            }

            aiReq.itemType = invenNum_ - 1;
            aiReq.itemCode = itemCode_;
            aiReq.itemPosition = addPosition;
            aiReq.itemCount = addCount;

            send(userSkt, (char*)&aiReq, sizeof(aiReq), 0);
            recv(userSkt, recvBuffer, PACKET_SIZE, 0);

            auto miResPacket = reinterpret_cast<ADD_ITEM_RESPONSE*>(recvBuffer);

            if (!miResPacket->isSuccess) return false;

            CONSUMABLES tempC;
            tempC.itemCode = itemCode_;
            tempC.position = addPosition;
            tempC.count = addCount;

            cs[addPosition] = tempC;

            return true;
        }
        else if (invenNum_ == 3) {
            uint16_t addPosition = 0;
            uint16_t addCount = count_;

            for (int i = 1; i < INVENTORY_SIZE; i++) {
                if (mt[i].itemCode == itemCode_) {
                    addPosition = i;
                    addCount += mt[i].count;
                    break;
                }
            }

            if (count_ == addCount) {
                for (int i = 1; i < INVENTORY_SIZE; i++) {
                    if (mt[i].itemCode == 0) {
                        addPosition = i;
                        break;
                    }
                }
            }

            if (addPosition == 0) { // 넣을 공간 없으면 false 반환
                std::cout << "Materials Full" << std::endl;
                return false;
            }

            aiReq.itemType = invenNum_ - 1;
            aiReq.itemCode = itemCode_;
            aiReq.itemPosition = addPosition;
            aiReq.itemCount = count_;

            send(userSkt, (char*)&aiReq, sizeof(aiReq), 0);
            recv(userSkt, recvBuffer, PACKET_SIZE, 0);

            auto miResPacket = reinterpret_cast<ADD_ITEM_RESPONSE*>(recvBuffer);

            if (!miResPacket->isSuccess) return false;

            MATERIALS tempM;
            tempM.itemCode = itemCode_;
            tempM.position = addPosition;
            tempM.count = addCount;

            mt[addPosition] = tempM;

            return true;
        }
    }

    bool DeleteItem(uint16_t invenNum_, uint16_t pos_) {
        DEL_ITEM_REQUEST delReq;
        delReq.PacketId = (UINT16)PACKET_ID::DEL_ITEM_REQUEST;
        delReq.PacketLength = sizeof(DEL_ITEM_REQUEST);
        delReq.itemPosition = pos_;

        if (invenNum_ == 1) {
            DEL_EQUIPMENT_REQUEST delEReq;
            delEReq.PacketId = (UINT16)PACKET_ID::DEL_EQUIPMENT_REQUEST;
            delEReq.PacketLength = sizeof(DEL_EQUIPMENT_REQUEST);
            delEReq.itemPosition = pos_;

            send(userSkt, (char*)&delEReq, sizeof(delEReq), 0);
            recv(userSkt, recvBuffer, PACKET_SIZE, 0);

            auto enhResPacket = reinterpret_cast<DEL_EQUIPMENT_RESPONSE*>(recvBuffer);

            if (!enhResPacket->isSuccess) return false;

            eq[pos_].enhance = 0;
            eq[pos_].itemCode = 0;

            return true;
        }
        else if (invenNum_ == 2) {
            delReq.itemType = invenNum_ - 1;

            send(userSkt, (char*)&delReq, sizeof(delReq), 0);
            recv(userSkt, recvBuffer, PACKET_SIZE, 0);

            auto delResPacket = reinterpret_cast<DEL_ITEM_RESPONSE*>(recvBuffer);

            if (!delResPacket->isSuccess) return false;

            cs[pos_].count = 0;
            cs[pos_].itemCode = 0;

            return true;
        }
        else if (invenNum_ == 3) {
            delReq.itemType = invenNum_ - 1;

            send(userSkt, (char*)&delReq, sizeof(delReq), 0);
            recv(userSkt, recvBuffer, PACKET_SIZE, 0);

            auto delResPacket = reinterpret_cast<DEL_ITEM_RESPONSE*>(recvBuffer);

            if (!delResPacket->isSuccess) return false;

            mt[pos_].count = 0;
            mt[pos_].itemCode = 0;

            return true;
        }
    }

    bool EnhanceEquip(uint16_t pos_) { // Equipment Only
        ENH_EQUIPMENT_REQUEST enhReq;
        enhReq.PacketId = (UINT16)PACKET_ID::ENH_EQUIPMENT_REQUEST;
        enhReq.PacketLength = sizeof(ENH_EQUIPMENT_REQUEST);
        enhReq.itemPosition = pos_;

        send(userSkt, (char*)&enhReq, sizeof(enhReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        std::cout << "강화중 ,," << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        auto enhResPacket = reinterpret_cast<ENH_EQUIPMENT_RESPONSE*>(recvBuffer);

        if (!enhResPacket->isSuccess) {
            return false;
        }

        eq[pos_].enhance = enhResPacket->Enhancement;
        std::cout << "+" << enhResPacket->Enhancement << "로 강화 성공" << std::endl;

        return true;
    }

    void RaidStart() {
        RAID_MATCHING_REQUEST rmReq;
        rmReq.PacketId = (UINT16)PACKET_ID::RAID_MATCHING_REQUEST;
        rmReq.PacketLength = sizeof(RAID_MATCHING_REQUEST);

        send(userSkt, (char*)&rmReq, sizeof(rmReq), 0);
        std::cout << "Match Insert Waitting " << std::endl;
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto rmReqPacket = reinterpret_cast<RAID_MATCHING_RESPONSE*>(recvBuffer);

        if (!rmReqPacket->insertSuccess) { // Mathing Success
            std::cout << "Server Matching Full. Matching Fail" << std::endl;
            return;
        }

        std::cout << "Match Insert Success" << std::endl;
        std::cout << "Team Waitting" << std::endl;
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);
        auto rrReqPacket = reinterpret_cast<RAID_READY_REQUEST*>(recvBuffer);

        timer = rrReqPacket->timer; // Minutes
        roomNum = rrReqPacket->roomNum; // If Max RoomNum Up to Short Range, Back to Number One
        myNum = rrReqPacket->yourNum;
        mobHp = rrReqPacket->mobHp;

        if (!makeUDPSocket()) { // UDP 소켓 생성 실패했을때
            std::cout << "Udp Socket Make Fail" << std::endl;
            return;
        }

        RAID_TEAMINFO_REQUEST rtReq;
        rtReq.PacketId = (UINT16)PACKET_ID::RAID_TEAMINFO_REQUEST;
        rtReq.PacketLength = sizeof(RAID_TEAMINFO_REQUEST);
        rtReq.imReady = true;
        rtReq.myNum = myNum;
        rtReq.roomNum = roomNum;
        rtReq.userAddr = udpAddr;

        send(userSkt, (char*)&rtReq, sizeof(rtReq), 0);
        std::cout << "Team Info Waitting" << std::endl;
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto rtiReqPacket = reinterpret_cast<RAID_TEAMINFO_RESPONSE*>(recvBuffer);
        uint16_t teamLevel = rtiReqPacket->teamLevel;
        std::string teamId = (std::string)rtiReqPacket->teamId;

        std::cout << "Team Waitting" << std::endl;
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto rsReqPacket = reinterpret_cast<RAID_START_REQUEST*>(recvBuffer);

        unsigned int myScore = 0;
        unsigned int teamScore = 0;

        std::cout << "Raid Start !" << std::endl;
        std::cout << "Mob Hp : " << mobHp << std::endl;
        std::cout << "My ID : " << userId << " / Level : " << level << std::endl;
        std::cout << "Team ID : " << teamId << " / Level : " << teamLevel << std::endl;

        rEndTime = rsReqPacket->endTime;

        while (1) {
            while (mobHp >= 0 || (std::chrono::steady_clock::now() < rEndTime)) {
                std::cout << "Input Damage" << std::endl;
                unsigned int damage;
                std::cin >> damage;

                RAID_HIT_REQUEST rhReq;
                rhReq.PacketId = (UINT16)PACKET_ID::RAID_HIT_REQUEST;
                rhReq.PacketLength = sizeof(RAID_HIT_REQUEST);
                rhReq.myNum = myNum;
                rhReq.roomNum = roomNum;
                rhReq.damage = damage;

                send(userSkt, (char*)&rhReq, sizeof(rhReq), 0);
                recv(userSkt, recvBuffer, PACKET_SIZE, 0);

                auto rhResPacket = reinterpret_cast<RAID_HIT_RESPONSE*>(recvBuffer);

                if (rhResPacket->currentMobHp <= 0) { // mob dead
                    if (rhResPacket->yourScore != 0) {
                        std::cout << "My Socre : " << rhResPacket->yourScore << std::endl;
                    }
                    std::cout << "Game End Waitting..." << std::endl;

                    recv(userSkt, recvBuffer, PACKET_SIZE, 0);

                    auto reReq = reinterpret_cast<RAID_END_REQUEST*>(recvBuffer);

                    std::cout << "Raid End. Your Score : " << reReq->userScore << std::endl;
                    std::cout << "Raid End. Team Score : " << reReq->teamScore << std::endl;
                    std::cout << "Raid End." << std::endl;
                    break;
                }
                else {
                    if (mobHp.load() > rhResPacket->currentMobHp) mobHp.store(rhResPacket->currentMobHp);
                    myScore = rhResPacket->yourScore;
                    std::cout << "My Socre : " << rhResPacket->yourScore << std::endl;
                }
            }
            break;
        }
        mobHp = 0;
        timer = 0;
        roomNum = 0;
        myNum = 0;

        if (syncRun) {
            syncRun = false;
            if (syncThread.joinable()) syncThread.join();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            closesocket(udpSocket);
            std::cout << "Sync Thread End" << std::endl;
        }

    }

    bool CreateSyncThread() {
        syncRun = true;
        std::thread(User::SyncThread, this);
        std::cout << "WorkThread Start" << std::endl;
        return true;
    }

    void SyncThread() {
        sockaddr_in serverAddr;
        int serverAddrSize = sizeof(serverAddr);
        std::cout << "Sync Thread Start" << std::endl;

        while (syncRun)
        {
            int received = recvfrom(udpSocket, recvUDPBuffer, sizeof(recvUDPBuffer), 0,
                (sockaddr*)&serverAddr, &serverAddrSize);

            if (received == sizeof(unsigned int) && syncRun) {
                unsigned int mobHp_ = *(unsigned int*)recvUDPBuffer;
                mobHp.store(mobHp_);
                std::cout << "Mob Hp : " << mobHp_ << std::endl;
            }
        }
    }

    bool GetRaidScore(uint16_t startNum_) { // 마지막 유저 스코어면 false 반환. 그 뒤 유저 없으니까 체크 x
        int rank;
        if (startNum_ == 0) {
            rank = startNum_;
        }
        else {
            rank = startNum_ * RANKING_USER_COUNT + 1;
        }

        char recvBuffer[PACKET_SIZE];
        memset(recvBuffer, 0, PACKET_SIZE);

        RAID_RANKING_REQUEST rrReq;
        rrReq.PacketId = (UINT16)PACKET_ID::RAID_RANKING_REQUEST;
        rrReq.PacketLength = sizeof(RAID_RANKING_REQUEST);
        rrReq.startRank = rank;

        send(userSkt, (char*)&rrReq, sizeof(rrReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto reRes = reinterpret_cast<RAID_RANKING_RESPONSE*>(recvBuffer);

        char* ptr = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);

        if (rank == 0) {
            for (int i = rank; i < rank + reRes->rkCount; i++) {
                RANKING tempR;
                memcpy((char*)&tempR, ptr, sizeof(RANKING));
                ptr += sizeof(RANKING);
                std::cout << i + 1 << "등 아이디 : " << (std::string)tempR.userId << " / 점수 : " << tempR.score << std::endl;
            }

            std::cout << std::endl;

            if (reRes->rkCount != RANKING_USER_COUNT) return false;

            return true;
        }

        for (int i = rank; i < rank + reRes->rkCount; i++) {
            RANKING tempR;
            memcpy((char*)&tempR, ptr, sizeof(RANKING));
            ptr += sizeof(RANKING);
            std::cout << i << "등 아이디 : " << (std::string)tempR.userId << " / 점수 : " << tempR.score << std::endl;
        }

        std::cout << std::endl;

        if (reRes->rkCount != RANKING_USER_COUNT) return false;

        return true;
    }

    void End() {
        char recvBuffer[PACKET_SIZE];
        memset(recvBuffer, 0, PACKET_SIZE);

        USER_LOGOUT_REQUEST_PACKET ulReq;
        ulReq.PacketId = (UINT16)PACKET_ID::USER_LOGOUT_REQUEST;
        ulReq.PacketLength = sizeof(USER_LOGOUT_REQUEST_PACKET);

        send(userSkt, (char*)&ulReq, sizeof(ulReq), 0);
    }

private:
    bool syncRun = false;
    std::atomic<uint16_t> level;
    std::atomic<unsigned int> exp;
    unsigned int raidScore;
    char recvUDPBuffer[sizeof(unsigned int)];

    // Raid
    std::atomic<int> mobHp;
    uint16_t timer;
    uint16_t roomNum;
    uint16_t myNum;

    SOCKET sessionSkt;
    SOCKET userSkt;
    SOCKET udpSocket;
    std::thread syncThread;

    sockaddr_in udpAddr;
    std::string userId = "quokka";

    std::chrono::time_point<std::chrono::steady_clock> rEndTime;

    std::vector<EQUIPMENT> eq{ INVENTORY_SIZE };
    std::vector<CONSUMABLES> cs{ INVENTORY_SIZE };
    std::vector<MATERIALS> mt{ INVENTORY_SIZE };

    char recvBuffer[PACKET_SIZE];
};