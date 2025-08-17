#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <map>
#include <unordered_map>
#include <iomanip>

#include "Packet.h"
#include "Define.h"

#pragma comment(lib, "ws2_32.lib") // ���־󿡼� �������α׷��� �ϱ� ���� ��

constexpr uint16_t INVENTORY_SIZE = 11; // 10���� +1�ؼ� 11���� �صα�
constexpr uint16_t UDP_PORT = 40001;

class User {
public:
    ~User() {
        closesocket(userSkt);
        WSACleanup();
    }

    void Test_CashCharge(uint32_t cashCharge_) {

        CASH_CHARGE_COMPLETE_REQUEST ccReq;
        ccReq.PacketId = (UINT16)PACKET_ID::CASH_CHARGE_COMPLETE_REQUEST;
        ccReq.PacketLength = sizeof(CASH_CHARGE_COMPLETE_REQUEST);
        ccReq.chargedCash = cashCharge_;

        send(userSkt, (char*)&ccReq, sizeof(ccReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto* mtPacket = reinterpret_cast<CASH_CHARGE_COMPLETE_RESPONSE*>(recvBuffer);
    
        if (!mtPacket->isSuccess) {
            std::cout << "���� ����" << '\n';
            return;
        }

        userCash = mtPacket->chargedCash;
        std::cout << "���� ���� ! ĳ�� �ݾ� : " << mtPacket->chargedCash << '\n';
        std::this_thread::sleep_for(std::chrono::seconds(1)); // ���� �� ������ ���� �ð� 1�� ����
    }

    bool init() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        sessionSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sessionSkt == INVALID_SOCKET) {
            std::cout << "Session Socket Make Fail" << std::endl;
            return false;
        }

        SOCKADDR_IN addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SESSION_SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &addr.sin_addr.s_addr);

        std::cout << "Login Server Connecting..." << std::endl;

        if (connect(sessionSkt, (SOCKADDR*)&addr, sizeof(addr))) {
            std::cout << "Login Server Connect Fail" << std::endl;
            return false;
        }

        std::cout << "Login Server Connect Success" << std::endl;


        // ============================ ���� ������ ��û ============================

        USERINFO_REQUEST uiReq;
        uiReq.PacketId = (UINT16)PACKET_ID::USERINFO_REQUEST;
        uiReq.PacketLength = sizeof(USERINFO_REQUEST);
        strncpy_s(uiReq.userId, userId.c_str(), MAX_USER_ID_LEN);

        send(sessionSkt, (char*)&uiReq, sizeof(uiReq), 0); // ���� ���� ��û
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // ���� ����

        auto uiPacket = reinterpret_cast<USERINFO_RESPONSE*>(recvBuffer);
        USERINFO tempU = uiPacket->UserInfo;
        exp = tempU.exp;
        level = tempU.level;
        raidScore = tempU.raidScore;
        userGold = tempU.gold;
        userCash = tempU.cash;
        userMileage = tempU.mileage;

        if (level == 0) {
            std::cout << "Get Userinfo Fail" << std::endl;
            return false;
        }
        std::cout << "Get Userinfo Success" << std::endl;


        // ============================ ��� �κ� ������ ��û ============================

        EQUIPMENT_REQUEST eqReq;
        eqReq.PacketId = (UINT16)PACKET_ID::EQUIPMENT_REQUEST;
        eqReq.PacketLength = sizeof(EQUIPMENT_REQUEST);

        send(sessionSkt, (char*)&eqReq, sizeof(eqReq), 0); // ��� ���� ��û
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // ��� ����

        auto eqPacket = reinterpret_cast<EQUIPMENT_RESPONSE*>(recvBuffer);
        char* ptr = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);

        for (int i = 1; i < INVENTORY_SIZE; i++) {
            eq[i].position = i;
        }

        for (int i = 0; i < eqPacket->eqCount; i++) {
            EQUIPMENT tempE;
            memcpy((char*)&tempE, ptr, sizeof(EQUIPMENT));

            if (tempE.itemCode == 0) return false; // �߸��� ������

            eq[tempE.position] = tempE;
            ptr += sizeof(EQUIPMENT);
        }

        std::cout << "Get EQUIPMENT Success" << std::endl;


        // ============================ �Һ� �κ� ������ ��û ============================

        CONSUMABLES_REQUEST csReq;
        csReq.PacketId = (UINT16)PACKET_ID::CONSUMABLES_REQUEST;
        csReq.PacketLength = sizeof(CONSUMABLES_REQUEST);

        send(sessionSkt, (char*)&csReq, sizeof(csReq), 0);
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // �Һ� ����

        auto csPacket = reinterpret_cast<CONSUMABLES_RESPONSE*>(recvBuffer);

        std::vector<CONSUMABLES> tempCs(csPacket->csCount);
        char* ptr2 = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);

        for (int i = 1; i < INVENTORY_SIZE; i++) {
            cs[i].position = i;
        }

        for (int i = 0; i < eqPacket->eqCount; i++) {
            CONSUMABLES tempCs;
            memcpy((char*)&tempCs, ptr2, sizeof(CONSUMABLES));

            if (tempCs.itemCode == 0) return false; // �߸��� ������

            cs[tempCs.position] = tempCs;
            ptr2 += sizeof(CONSUMABLES);
        }

        std::cout << "Get CONSUMABLES Success" << std::endl;


        // ============================ ��� �κ� ������ ��û ============================

        MATERIALS_REQUEST mtReq;
        mtReq.PacketId = (UINT16)PACKET_ID::MATERIALS_REQUEST;
        mtReq.PacketLength = sizeof(MATERIALS_REQUEST);

        send(sessionSkt, (char*)&mtReq, sizeof(mtReq), 0);
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // ��� ����

        auto mtPacket = reinterpret_cast<MATERIALS_RESPONSE*>(recvBuffer);

        std::vector<MATERIALS> tempMt(mtPacket->mtCount);
        char* ptr3 = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);

        for (int i = 1; i < INVENTORY_SIZE; i++) {
            mt[i].position = i;
        }

        for (int i = 0; i < eqPacket->eqCount; i++) {
            MATERIALS tempM;
            memcpy((char*)&tempM, ptr3, sizeof(MATERIALS));

            if (tempM.itemCode == 0) return false; // �߸��� ������

            mt[tempM.position] = tempM;
            ptr3 += sizeof(MATERIALS);
        }

        std::cout << "Get MATERIALS Success" << std::endl;


        // ============================ �̺�Ʈ �н� ���� ��û ============================

        PASSINFO_REQUEST piReq;
        piReq.PacketId = (UINT16)PACKET_ID::PASSINFO_REQUEST;
        piReq.PacketLength = sizeof(PASSINFO_REQUEST);

        send(sessionSkt, (char*)&piReq, sizeof(piReq), 0);
        recv(sessionSkt, recvBuffer, sizeof(PASSINFO_RESPONSE), 0);

        auto piPacket = reinterpret_cast<PASSINFO_RESPONSE*>(recvBuffer);
        uint16_t tempPassInfoCount = piPacket->passCount;

        recv(sessionSkt, recvBuffer + sizeof(PASSINFO_RESPONSE), PACKET_SIZE, 0);

        USERPASSINFO* tempPassInfos = reinterpret_cast<USERPASSINFO*>(recvBuffer + sizeof(PASSINFO_RESPONSE));

        for (int i = 0; i < tempPassInfoCount; i++) {
            USERPASSINFO tempPi = tempPassInfos[i];

            if (tempPi.passCurrencyType == 254) return false; // �߸��� ������

            passInfoMap[tempPi.passInfo.passId] = { tempPi.passInfo, tempPi.passLevel, tempPi.passExp, tempPi.passCurrencyType };
        }

        std::cout << "Get PASSINFO Success" << std::endl;


        // ============================ �н� ���� ȹ�� ���� ��û ============================

        PASSREWARDINFO_REQUEST priReq;
        priReq.PacketId = (UINT16)PACKET_ID::PASSREWARDINFO_REQUEST;
        priReq.PacketLength = sizeof(PASSREWARDINFO_REQUEST);

        send(sessionSkt, (char*)&priReq, sizeof(priReq), 0);
        recv(sessionSkt, recvBuffer, sizeof(PASSREWARDINFO_RESPONSE), 0);

        auto priPacket = reinterpret_cast<PASSREWARDINFO_RESPONSE*>(recvBuffer);
        uint16_t tempPassRewordCount = priPacket->passRewordCount;

        recv(sessionSkt, recvBuffer + sizeof(PASSREWARDINFO_RESPONSE), PACKET_SIZE, 0);

        PASSREWARDNFO* tempPassRewordInfos = reinterpret_cast<PASSREWARDNFO*>(recvBuffer + sizeof(PASSREWARDINFO_RESPONSE));

        std::unordered_map<std::string, PASSREWARDNFO_CLIENT> passRewardInfoMap; // �ӽ� �н� ������ ��Ʈ ���� ��

        for (int i = 0; i < tempPassRewordCount; i++) {
            PASSREWARDNFO tempPri = tempPassRewordInfos[i];

            if (tempPri.passCurrencyType == 254) return false; // �߸��� ������

            passRewardInfoMap[tempPri.passId] = { tempPri.passReqwardBits, tempPri.passCurrencyType };
        }

        std::cout << "Get PASSREWARDNFO Success" << std::endl;


        // ============================ �߾� ���� ���� ���� ��û ============================

        USER_GAMESTART_REQUEST ugReq;
        ugReq.PacketId = (UINT16)PACKET_ID::USER_GAMESTART_REQUEST;
        ugReq.PacketLength = sizeof(USER_GAMESTART_REQUEST);
        strncpy_s(ugReq.userId, userId.c_str(), MAX_USER_ID_LEN);

        send(sessionSkt, (char*)&ugReq, sizeof(ugReq), 0); // ���� ���� �غ� ��û
        recv(sessionSkt, recvBuffer, PACKET_SIZE, 0); // ���� ������ ���� �� ��ū

        auto ucReqPacket = reinterpret_cast<USER_GAMESTART_RESPONSE*>(recvBuffer);

        std::string Token = ucReqPacket->Token;

        if (Token == "") { // ���� �������� ��ū ���� ����
            std::cout << "Get Token Fail" << std::endl;
            return false;
        }

        std::cout << "Connect Success" << std::endl;

        shutdown(sessionSkt, SD_BOTH);
        closesocket(sessionSkt); // ���� ���� ���� �ݱ�

        std::cout << "If you Press 1, game start. If you want out, press any key" << std::endl;

        int k = 0;
        std::cin >> k;
        if (k != 1) exit(0);

        userSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (userSkt == INVALID_SOCKET) {
            std::cout << "Server Socket Make Fail" << std::endl;
            return false;
        }

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

        gameServerSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (gameServerSkt == INVALID_SOCKET) {
            std::cout << "Game Server Socket Make Fail" << std::endl;
            return false;
        }

        channelSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (channelSkt == INVALID_SOCKET) {
            std::cout << "Channel Server Socket Make Fail" << std::endl;
            return false;
        }

        std::cout << "Connect Success In Game Server" << std::endl;


        // ============================ ���� ������ ��û ============================

        SHOP_DATA_REQUEST shReq;
        shReq.PacketId = (UINT16)PACKET_ID::SHOP_DATA_REQUEST;
        shReq.PacketLength = sizeof(SHOP_DATA_REQUEST);

        send(userSkt, (char*)&shReq, sizeof(shReq), 0);
        recv(userSkt, recvBuffer, sizeof(SHOP_DATA_RESPONSE), 0);

        auto sdRes = reinterpret_cast<SHOP_DATA_RESPONSE*>(recvBuffer);
        uint16_t tempShopCount = sdRes->shopItemSize;

        recv(userSkt, recvBuffer + sizeof(SHOP_DATA_RESPONSE), PACKET_SIZE, 0);

        ShopItemForSend* tempShopItems = reinterpret_cast<ShopItemForSend*>(recvBuffer + sizeof(SHOP_DATA_RESPONSE));

        shopItems.resize(tempShopCount);
        for (int i = 0; i < tempShopCount; i++) {
            shopItems[i] = tempShopItems[i];
        }

        std::cout << "���� ������ ���� ����" << '\n';


        // ============================ �н� ������ ��û ============================

        PASS_DATA_REQUEST passReq;
        passReq.PacketId = (uint16_t)PACKET_ID::PASS_DATA_REQUEST;
        passReq.PacketLength = sizeof(PASS_DATA_REQUEST);

        send(userSkt, (char*)&passReq, sizeof(passReq), 0);
        recv(userSkt, recvBuffer, sizeof(PASS_DATA_RESPONSE), 0);

        auto pdRes = reinterpret_cast<PASS_DATA_RESPONSE*>(recvBuffer);
        uint16_t passDataCount = pdRes->passDataSize;

        recv(userSkt, recvBuffer + sizeof(PASS_DATA_RESPONSE), PACKET_SIZE, 0);

        PassItemForSend* dataArray = reinterpret_cast<PassItemForSend*>(recvBuffer + sizeof(PASS_DATA_RESPONSE));

        for (int i = 0; i < passDataCount; i++) {
            std::string passId = (std::string)dataArray[i].passId;
            PassItem_Client pc;

            pc.SetData(dataArray[i]);

            auto it = passRewardInfoMap.find(passId);
            if (it != passRewardInfoMap.end()) {
                PASSREWARDNFO_CLIENT& rewardInfo = it->second;

                if (rewardInfo.passCurrencyType == pc.passCurrencyType) {
                    uint16_t level = dataArray[i].passLevel;

                    if (level < 64) {
                        pc.getCheck = (rewardInfo.passReqwardBits >> (level-1)) != 0; // �ڵ� �� ������ üũ. (*��Ʈ ����Ҷ� -1 �սô�)
                    }
                    else { // �߸��� ������
                        return false;
                    }
                }
            }

            passDataMap[passId][{pc.passLevel, pc.passCurrencyType}] = pc;
        }

        std::cout << "�н� ������ ���� ����" << '\n';
        std::cout << userId << " ���� ���� ���� !" << '\n';
    }

    uint16_t MoveServer(bool countCheck_) {
        if (countCheck_ == true) {
            std::cout << std::endl;
            uint16_t tempC = 0;
            for (int i = 1; i < tempServerUserCounts.size(); i++) {
                std::cout << i << "���� ���� �� : " << tempServerUserCounts[i] << std::endl;
            }
            std::cout << "�̵��� ä���� �������ּ��� (������ �����Ͻ÷��� 10���� �����ּ���) : ";
            std::cin >> tempC;

            if (tempC == 10) return 10;

            MOVE_SERVER_REQUEST movesServerReq;
            movesServerReq.PacketId = (UINT16)PACKET_ID::MOVE_SERVER_REQUEST;
            movesServerReq.PacketLength = sizeof(MOVE_SERVER_REQUEST);

            if (tempC == 1) {
                movesServerReq.serverNum = static_cast<uint16_t>(ChannelServerType::CH_01);
            }
            else if (tempC == 2) {
                movesServerReq.serverNum = static_cast<uint16_t>(ChannelServerType::CH_02);
            }

            send(userSkt, (char*)&movesServerReq, sizeof(movesServerReq), 0);
            recv(userSkt, recvBuffer, PACKET_SIZE, 0);

            auto msResPacket = reinterpret_cast<MOVE_SERVER_RESPONSE*>(recvBuffer);

            if (msResPacket->port == 0) { // ���� �̵� ����
                std::cout << "���� �ش� ������ �̵��� �� �����ϴ�. �ٸ� ������ �̿����ּ���" << std::endl;
                return 0;
            }

            std::string Token = msResPacket->serverToken;

            SOCKADDR_IN addr;
            ZeroMemory(&addr, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(msResPacket->port);
            inet_pton(AF_INET, msResPacket->ip, &addr.sin_addr.s_addr);

            if (connect(channelSkt, (SOCKADDR*)&addr, sizeof(addr))) {
                std::cout << "���� �ش� ������ �̵��� �� �����ϴ�. �ٸ� ������ �̿����ּ���" << std::endl;
                return 0;
            }

            USER_CONNECT_CHANNEL_REQUEST_PACKET uccReq;
            uccReq.PacketId = (UINT16)PACKET_ID::USER_CONNECT_CHANNEL_REQUEST;
            uccReq.PacketLength = sizeof(USER_CONNECT_CHANNEL_REQUEST_PACKET);
            strncpy_s(uccReq.userId, userId.c_str(), MAX_USER_ID_LEN);
            strncpy_s(uccReq.userToken, Token.c_str(), MAX_JWT_TOKEN_LEN);

            std::cout << "Connect Requset To Channel Server.." << std::endl;

            send(channelSkt, (char*)&uccReq, sizeof(uccReq), 0);
            recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

            auto uccResPacket = reinterpret_cast<USER_CONNECT_CHANNEL_RESPONSE_PACKET*>(recvBuffer);

            if (uccResPacket->isSuccess == false) { // ����� ���� ���� �ݰ� �ٽ� ����
                std::cout << "���� �ش� ������ �̵��� �� �����ϴ�. �ٸ� ������ �̿����ּ���" << std::endl;
                ChannelSocketinitialization();
                return 0;
            }

            std::cout << "Connect Success To Channel Server" << std::endl;

            // ��ū���� üũ �Ϸ� (���� ���� ����)
            currentServer = tempC;
            return 1;
        }

        SERVER_USER_COUNTS_REQUEST serverUserCountsReq;
        serverUserCountsReq.PacketId = (UINT16)PACKET_ID::SERVER_USER_COUNTS_REQUEST;
        serverUserCountsReq.PacketLength = sizeof(SERVER_USER_COUNTS_REQUEST);

        send(userSkt, (char*)&serverUserCountsReq, sizeof(serverUserCountsReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto ucResPacket = reinterpret_cast<SERVER_USER_COUNTS_RESPONSE*>(recvBuffer);
        char* ptr = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);
        std::vector<uint16_t> tempV;
        tempV.resize(ucResPacket->serverCount, 0);
        uint16_t tempC = 0;

        std::cout << std::endl;

        for (int i = 1; i < ucResPacket->serverCount; i++) {
            memcpy((char*)&tempC, ptr, sizeof(uint16_t));
            std::cout << i << "���� ���� �� : " << tempC << std::endl;
            tempV[i] = tempC;
            ptr += sizeof(uint16_t);
        }

        tempServerUserCounts = tempV;

        std::cout << "�̵��� ������ �������ּ��� (������ �����Ͻ÷��� 10���� �����ּ���) : ";
        std::cin >> tempC;

        if (tempC == 10) return 10;

        MOVE_SERVER_REQUEST movesServerReq;
        movesServerReq.PacketId = (UINT16)PACKET_ID::MOVE_SERVER_REQUEST;
        movesServerReq.PacketLength = sizeof(MOVE_SERVER_REQUEST);

        if (tempC == 1) {
            movesServerReq.serverNum = static_cast<uint16_t>(ChannelServerType::CH_01);
        }
        else if (tempC == 2) {
            movesServerReq.serverNum = static_cast<uint16_t>(ChannelServerType::CH_02);
        }

        send(userSkt, (char*)&movesServerReq, sizeof(movesServerReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto msResPacket = reinterpret_cast<MOVE_SERVER_RESPONSE*>(recvBuffer);

        if (msResPacket->port == 0) { // ���� �̵� ����
            std::cout << "���� �ش� ������ �̵��� �� �����ϴ�. �ٸ� ������ �̿����ּ���" << std::endl;
            return 0;
        }

        std::string Token = msResPacket->serverToken;

        SOCKADDR_IN addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(msResPacket->port);
        inet_pton(AF_INET, msResPacket->ip, &addr.sin_addr.s_addr);

        if (connect(channelSkt, (SOCKADDR*)&addr, sizeof(addr))) {
            std::cout << "���� �ش� ������ �̵��� �� �����ϴ�. �ٸ� ������ �̿����ּ���" << std::endl;
            return 0;
        }

        USER_CONNECT_CHANNEL_REQUEST_PACKET uccReq;
        uccReq.PacketId = (UINT16)PACKET_ID::USER_CONNECT_CHANNEL_REQUEST;
        uccReq.PacketLength = sizeof(USER_CONNECT_CHANNEL_REQUEST_PACKET);
        strncpy_s(uccReq.userId, userId.c_str(), MAX_USER_ID_LEN);
        strncpy_s(uccReq.userToken, Token.c_str(), MAX_JWT_TOKEN_LEN);

        std::cout << "Connect Requset To Channel Server.." << std::endl;

        send(channelSkt, (char*)&uccReq, sizeof(uccReq), 0);
        recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

        auto uccResPacket = reinterpret_cast<USER_CONNECT_CHANNEL_RESPONSE_PACKET*>(recvBuffer);

        if (uccResPacket->isSuccess == false) { // ����� ���� ���� �ݰ� �ٽ� ����
            std::cout << "���� �ش� ������ �̵��� �� �����ϴ�. �ٸ� ������ �̿����ּ���" << std::endl;
            ChannelSocketinitialization();
            return 0;
        }

        std::cout << "Connect Success To Channel Server" << std::endl;

        // ��ū���� üũ �Ϸ� (���� ���� ����)
        currentServer = tempC;
        return 1;
    }

    bool ChannelSocketinitialization() {
        shutdown(channelSkt, SD_BOTH);
        closesocket(channelSkt); // ä�� ���� ���� �ݱ�

        channelSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (channelSkt == INVALID_SOCKET) {
            std::cout << "Channel Server Socket Make Fail" << std::endl;
            return false;
        }

        return true;
    }

    bool gameServerSocketinitialization() {
        shutdown(gameServerSkt, SD_BOTH);
        closesocket(gameServerSkt); // ���� ���� ���� �ݱ�
        closesocket(udpSkt);
        return true;
    }

    uint16_t SelectChannel(bool countCheck_) {

        if (countCheck_ == true) { // ä�� ���� �̹� üũ ������ �޾ƿ� ä�� �� �� ���� �״�� �̿��ϱ�
            std::cout << std::endl;
            uint16_t tempC = 0;
            for (int i = 1; i < tempChannelUserCounts.size(); i++) {
                std::cout << i << "ä�� ���� �� : " << tempChannelUserCounts[i] << std::endl;
            }
            std::cout << "�̵��� ä���� �������ּ��� (�ڷΰ����� 10���� �����ּ���) : ";
            std::cin >> tempC;

            if (tempC == 10) return 10;

            MOVE_CHANNEL_REQUEST mcReq;
            mcReq.PacketId = (UINT16)PACKET_ID::MOVE_CHANNEL_REQUEST;
            mcReq.PacketLength = sizeof(MOVE_CHANNEL_REQUEST);
            mcReq.channelNum = tempC;

            send(channelSkt, (char*)&mcReq, sizeof(mcReq), 0);
            recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

            auto mcResPacket = reinterpret_cast<MOVE_CHANNEL_RESPONSE*>(recvBuffer);

            if (!mcResPacket->isSuccess) {
                std::cout << "���� �ش� ä�η� �̵��� �� �����ϴ�. �ٸ� ä���� �̿����ּ���" << std::endl;
                return 0;
            }

            currentChannel = tempC;
            return 1;
        }

        CHANNEL_USER_COUNTS_REQUEST chUserCountsReq;
        chUserCountsReq.PacketId = (UINT16)PACKET_ID::CHANNEL_USER_COUNTS_REQUEST;
        chUserCountsReq.PacketLength = sizeof(CHANNEL_USER_COUNTS_REQUEST);

        send(channelSkt, (char*)&chUserCountsReq, sizeof(chUserCountsReq), 0);
        recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

        auto cucResPacket = reinterpret_cast<CHANNEL_USER_COUNTS_RESPONSE*>(recvBuffer);
        char* ptr = recvBuffer + sizeof(PACKET_HEADER) + sizeof(uint16_t);

        uint16_t tempC = 0;
        std::vector<uint16_t> tempV;
        tempV.resize(cucResPacket->channelCount, 0);

        std::cout << std::endl;

        for (int i = 1; i < cucResPacket->channelCount; i++) {
            memcpy((char*)&tempC, ptr, sizeof(uint16_t));
            std::cout << i << "ä�� ���� �� : " << tempC << std::endl;
            tempV[i] = tempC;
            ptr += sizeof(uint16_t);
        }

        tempChannelUserCounts = tempV;

        std::cout << "�̵��� ä���� �������ּ��� (�ڷΰ����� 10���� �����ּ���) : ";
        std::cin >> tempC;

        if (tempC == 10) return 10;

        MOVE_CHANNEL_REQUEST mcReq;
        mcReq.PacketId = (UINT16)PACKET_ID::MOVE_CHANNEL_REQUEST;
        mcReq.PacketLength = sizeof(MOVE_CHANNEL_REQUEST);
        mcReq.channelNum = tempC;

        send(channelSkt, (char*)&mcReq, sizeof(mcReq), 0);
        recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

        auto mcResPacket = reinterpret_cast<MOVE_CHANNEL_RESPONSE*>(recvBuffer);

        if (!mcResPacket->isSuccess) {
            std::cout << "���� �ش� ä�η� �̵��� �� �����ϴ�. �ٸ� ä���� �̿����ּ���" << std::endl;
            return 0;
        }

        currentChannel = tempC;
        return 1;
    }

    bool makeudpSkt() {
        udpSkt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (udpSkt == INVALID_SOCKET) {
            std::cout << "Udp Socket Make Fail Error : " << WSAGetLastError() << std::endl;
            return false;
        }

        udpAddr.sin_family = AF_INET;
        udpAddr.sin_port = htons(UDP_PORT);
        inet_pton(AF_INET, "127.0.0.1", &udpAddr.sin_addr);

        bind(udpSkt, (sockaddr*)&udpAddr, sizeof(udpAddr));

        std::cout << "Udp Socket Make Success" << std::endl;

        return true;
    }

    void GetMyInfo() {
        std::cout << "��� : " << userGold << "ĳ�� : " << userCash << "���ϸ��� : " << userMileage << '\n';

        std::cout << "���̵� : " << userId << std::endl;
        std::cout << "���� : " << level << std::endl;
        std::cout << "����ġ : " << exp << std::endl;
        std::cout << "���̵� �ְ� ���� : " << raidScore << std::endl;
    }

    void AddExpFromMob(uint16_t mobNum_) {
        EXP_UP_REQUEST euReq;
        euReq.PacketId = (UINT16)PACKET_ID::EXP_UP_REQUEST;
        euReq.PacketLength = sizeof(EXP_UP_REQUEST);
        euReq.mobNum = mobNum_;

        send(channelSkt, (char*)&euReq, sizeof(euReq), 0);
        recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

        auto ucResPacket = reinterpret_cast<EXP_UP_RESPONSE*>(recvBuffer);

        if (ucResPacket->increaseLevel == 0) { // Only Exp Up
            exp = ucResPacket->currentExp;
            std::cout << mobNum_ << " ���͸� ��ҽ��ϴ� !" << std::endl;
            std::cout << "���� ���� : " << level.load() << std::endl;
            std::cout << "���� ����ġ : " << exp << std::endl;
        }
        else { // Level up
            level.fetch_add(ucResPacket->increaseLevel);
            exp = ucResPacket->currentExp;
            std::cout << mobNum_ << " ���͸� ��ҽ��ϴ� !" << std::endl;
            std::cout << "������ �߽��ϴ� !" << std::endl;
            std::cout << "���� ���� : " << level.load() << std::endl;
            std::cout << "���� ����ġ : " << exp << std::endl;
        }
    }

    std::pair<uint16_t, unsigned int> GetUserLevelExp() {
        std::cout << "���� ���� : " << level.load() << std::endl;
        std::cout << "���� ����ġ : " << exp.load() << std::endl;
        return { level, exp };
    }

    void GetInventory(uint16_t invenNum_) {
        if (invenNum_ == 1) {
            std::cout << "��� �κ��丮" << std::endl;
            for (int i = 0; i < eq.size(); i++) {
                if (eq[i].itemCode == 0) continue;
                std::cout << eq[i].position << "�� ��ġ�� +" << eq[i].enhance << "��ȭ �Ǿ��ִ� " << eq[i].itemCode << "�� ������ ����" << std::endl;
            }
        }
        else if (invenNum_ == 2) {
            std::cout << "�Һ� �κ��丮" << std::endl;
            for (int i = 0; i < eq.size(); i++) {
                if (cs[i].itemCode == 0) continue;
                std::cout << cs[i].position << "�� ��ġ�� " << cs[i].itemCode << "�� ������ " << cs[i].count << "�� ����" << std::endl;
            }
        }
        else if (invenNum_ == 3) {
            std::cout << "��� �κ��丮" << std::endl;
            for (int i = 0; i < eq.size(); i++) {
                if (mt[i].itemCode == 0) continue;
                std::cout << mt[i].position << "�� ��ġ�� " << mt[i].itemCode << "�� ������ " << mt[i].count << "�� ����" << std::endl;
            }
        }
        return;
    }

    bool MoveItem(uint16_t invenNum_, uint16_t currentpos_, uint16_t movepos_) {
        if (invenNum_ == 1) { // ���
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

            send(channelSkt, (char*)&miReq, sizeof(miReq), 0);
            recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

            auto miResPacket = reinterpret_cast<MOV_EQUIPMENT_RESPONSE*>(recvBuffer);

            if (!miResPacket->isSuccess) return false;

            moveE.position = miReq.dragItemPos;
            currentE.position = miReq.targetItemPos;

            eq[currentpos_] = moveE;
            eq[movepos_] = currentE;

            return true;
        }
        else { // �Һ� or ���
            MOV_ITEM_REQUEST miReq;
            miReq.PacketId = (UINT16)PACKET_ID::MOV_ITEM_REQUEST;
            miReq.PacketLength = sizeof(MOV_ITEM_REQUEST);

            if (invenNum_ == 2) { // �Һ�
                CONSUMABLES currentC = cs[currentpos_];
                CONSUMABLES moveC = cs[movepos_];

                miReq.ItemType = invenNum_ - 1;
                miReq.dragItemCode = moveC.itemCode;
                miReq.dragItemCount = moveC.count;
                miReq.dragItemPos = currentC.position;
                miReq.targetItemCode = currentC.itemCode;
                miReq.targetItemCount = currentC.count;
                miReq.targetItemPos = moveC.position;

                send(channelSkt, (char*)&miReq, sizeof(miReq), 0);
                recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

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

                send(channelSkt, (char*)&miReq, sizeof(miReq), 0);
                recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

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

        if (addPosition == INVENTORY_SIZE + 1) { // ���� ���� ������ false ��ȯ
            std::cout << "Equipments Full" << std::endl;
            return false;
        }

        aeReq.itemCode = itemCode_;
        aeReq.itemPosition = addPosition;
        aeReq.Enhancement = enhancement_;

        send(channelSkt, (char*)&aeReq, sizeof(aeReq), 0);
        recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

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

        if (invenNum_ == 2) { // �Һ�
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

            if (addPosition == 0) { // ���� ���� ������ false ��ȯ
                std::cout << "Consumables Full" << std::endl;
                return false;
            }

            aiReq.itemType = invenNum_ - 1;
            aiReq.itemCode = itemCode_;
            aiReq.itemPosition = addPosition;
            aiReq.itemCount = addCount;

            send(channelSkt, (char*)&aiReq, sizeof(aiReq), 0);
            recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

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

            if (addPosition == 0) { // ���� ���� ������ false ��ȯ
                std::cout << "Materials Full" << std::endl;
                return false;
            }

            aiReq.itemType = invenNum_ - 1;
            aiReq.itemCode = itemCode_;
            aiReq.itemPosition = addPosition;
            aiReq.itemCount = count_;

            send(channelSkt, (char*)&aiReq, sizeof(aiReq), 0);
            recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

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

            send(channelSkt, (char*)&delEReq, sizeof(delEReq), 0);
            recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

            auto enhResPacket = reinterpret_cast<DEL_EQUIPMENT_RESPONSE*>(recvBuffer);

            if (!enhResPacket->isSuccess) return false;

            eq[pos_].enhance = 0;
            eq[pos_].itemCode = 0;

            return true;
        }
        else if (invenNum_ == 2) {
            delReq.itemType = invenNum_ - 1;

            send(channelSkt, (char*)&delReq, sizeof(delReq), 0);
            recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

            auto delResPacket = reinterpret_cast<DEL_ITEM_RESPONSE*>(recvBuffer);

            if (!delResPacket->isSuccess) return false;

            cs[pos_].count = 0;
            cs[pos_].itemCode = 0;

            return true;
        }
        else if (invenNum_ == 3) {
            delReq.itemType = invenNum_ - 1;

            send(channelSkt, (char*)&delReq, sizeof(delReq), 0);
            recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

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

        send(channelSkt, (char*)&enhReq, sizeof(enhReq), 0);
        recv(channelSkt, recvBuffer, PACKET_SIZE, 0);

        std::cout << "��ȭ�� ,," << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        auto enhResPacket = reinterpret_cast<ENH_EQUIPMENT_RESPONSE*>(recvBuffer);

        if (!enhResPacket->isSuccess) {
            return false;
        }

        eq[pos_].enhance = enhResPacket->Enhancement;
        std::cout << "+" << enhResPacket->Enhancement << "�� ��ȭ ����" << std::endl;

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

        gameServerSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (gameServerSkt == INVALID_SOCKET) {
            std::cout << "gameServerSkt Socket Make Fail" << std::endl;
        }

        SOCKADDR_IN addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(rrReqPacket->port);
        inet_pton(AF_INET, rrReqPacket->ip, &addr.sin_addr.s_addr);

        std::cout << "Raid Server Connecting..." << std::endl;

        if (connect(gameServerSkt, (SOCKADDR*)&addr, sizeof(addr))) {
            std::cout << "Session Server Connection Fail" << std::endl;
            return;
        }

        USER_CONNECT_GAME_REQUEST userConnReq;
        userConnReq.PacketId = (UINT16)PACKET_ID::USER_CONNECT_GAME_REQUEST;
        userConnReq.PacketLength = sizeof(USER_CONNECT_GAME_REQUEST);
        strncpy_s(userConnReq.userId, userId.c_str(), MAX_USER_ID_LEN);
        strncpy_s(userConnReq.userToken, rrReqPacket->serverToken, MAX_JWT_TOKEN_LEN);

        send(gameServerSkt, (char*)&userConnReq, sizeof(userConnReq), 0);
        recv(gameServerSkt, recvBuffer, PACKET_SIZE, 0);

        auto userConnResPacket = reinterpret_cast<USER_CONNECT_GAME_RESPONSE*>(recvBuffer);

        if (!userConnResPacket->isSuccess) { // ���� ���� ���� ����
            std::cout << "���� ����" << std::endl;
            gameServerSocketinitialization();
            return;
        }

        if (!makeudpSkt()) { // UDP ���� ���� ����������
            std::cout << "Udp Socket Make Fail" << std::endl;
            return;
        }

        std::cout << "Raid Server Connection Success" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
        raidUserInfos.resize(3);

        RAID_TEAMINFO_REQUEST rtReq;
        rtReq.PacketId = (UINT16)PACKET_ID::RAID_TEAMINFO_REQUEST;
        rtReq.PacketLength = sizeof(RAID_TEAMINFO_REQUEST);
        rtReq.userAddr = udpAddr;

        std::cout << "Team Info Waitting" << std::endl;
        send(gameServerSkt, (char*)&rtReq, sizeof(rtReq), 0);

        for (int i = 1; i < raidUserInfos.size(); i++) {
            recv(gameServerSkt, recvBuffer, PACKET_SIZE, 0);

            auto rtiReqPacket = reinterpret_cast<RAID_TEAMINFO_RESPONSE*>(recvBuffer);

            RAID_USERINFO tempRaidUserInfo;
            tempRaidUserInfo.userId = (std::string)rtiReqPacket->teamId;
            tempRaidUserInfo.userLevel = rtiReqPacket->userLevel;

            raidUserInfos[rtiReqPacket->userRaidServerObjNum] = tempRaidUserInfo;
        }

        std::cout << "Get Team Info Success" << std::endl;
        recv(gameServerSkt, recvBuffer, PACKET_SIZE, 0);
        auto rsReqPacket = reinterpret_cast<RAID_START*>(recvBuffer);

        mobHp.store(rsReqPacket->mobHp);
        mapNum = rsReqPacket->mapNum;

        std::cout << "Waiting 2 seconds before starting the raid..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "Raid Start !" << std::endl;
        std::cout << "Map : " << mapNum << std::endl;
        std::cout << "Mob Hp : " << mobHp << std::endl;

        std::cout << std::endl;
        std::cout << "Raid participant information:" << std::endl;

        for (int i = 1; i < raidUserInfos.size(); i++) {
            std::cout << "My ID : " << raidUserInfos[i].userId << " / Level : " << raidUserInfos[i].userLevel << std::endl;
        }
        std::cout << std::endl;

        rEndTime = rsReqPacket->endTime;

        CreateSyncThread();
        CreateInGameThread();

        std::this_thread::sleep_for(std::chrono::seconds(1));

        while (!inGameRun && !syncRun) { std::this_thread::sleep_for(std::chrono::seconds(1)); }

        syncRun = false;
        inGameRun = false;

        if (inGameThread.joinable()) inGameThread.join();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "InGame Thread End" << std::endl;

        if (syncThread.joinable()) syncThread.join();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Sync Thread End" << std::endl;

        raidUserInfos.clear();
    }

    bool CreateSyncThread() {
        syncRun = true;
        syncThread = std::thread([this]() {SyncThread(); });
        std::cout << "SyncThread Start" << std::endl;
        return true;
    }

    bool CreateInGameThread() {
        inGameRun = true;
        inGameThread = std::thread([this]() {InGameThread(); });
        std::cout << "InGameThread Start" << std::endl;
        return true;
    }

    void SyncThread() {
        sockaddr_in serverAddr = {};

        while (syncRun) {
            int serverAddrSize = sizeof(serverAddr);
            int received = recvfrom(udpSkt, recvUDPBuffer, sizeof(recvUDPBuffer), 0, (sockaddr*)&serverAddr, &serverAddrSize);
            
            if (received == sizeof(unsigned int)) { // ������ �� �������� ����ȭ
                unsigned int mobHp_ = *(unsigned int*)recvUDPBuffer;
                mobHp.store(mobHp_);
            }

        }
    }

    void InGameThread() {
        unsigned int myScore = 0;
        unsigned int teamScore = 0;

        while (mobHp >= 0 || (std::chrono::steady_clock::now() < rEndTime)) {
			std::cout << "Mob Hp : " << mobHp.load() << std::endl;
            std::cout << "Input Damage" << std::endl;
            unsigned int damage;
            std::cin >> damage;

            RAID_HIT_REQUEST rhReq;
            rhReq.PacketId = (UINT16)PACKET_ID::RAID_HIT_REQUEST;
            rhReq.PacketLength = sizeof(RAID_HIT_REQUEST);
            rhReq.damage = damage;

            send(gameServerSkt, (char*)&rhReq, sizeof(rhReq), 0);
            recv(gameServerSkt, recvBuffer, PACKET_SIZE, 0);

            auto rhResPacket = reinterpret_cast<RAID_HIT_RESPONSE*>(recvBuffer);

            if (rhResPacket->currentMobHp <= 0) { // mob dead
                if (rhResPacket->yourScore != 0) {
                    std::cout << "My Socre : " << rhResPacket->yourScore << std::endl;
                }

                std::cout << "Game End Waitting..." << std::endl;

                recv(gameServerSkt, recvBuffer, PACKET_SIZE, 0);

                std::cout << "Game End" << std::endl;

                for (int i = 1; i < raidUserInfos.size(); i++) {
                    recv(gameServerSkt, recvBuffer, PACKET_SIZE, 0);

                    auto scoreRes = reinterpret_cast<SEND_RAID_SCORE*>(recvBuffer);
                    raidUserInfos[scoreRes->userRaidServerObjNum].userScore = scoreRes->userScore;
                }

                for (int i = 1; i < raidUserInfos.size(); i++) {
                    std::cout << raidUserInfos[i].userId << " Score : " << raidUserInfos[i].userScore << std::endl;
                }

                std::cout << "Raid End." << std::endl;
                break;
            }
            else {
                if (mobHp.load() > rhResPacket->currentMobHp) mobHp.store(rhResPacket->currentMobHp);
                myScore = rhResPacket->yourScore;
                std::cout << "My Socre : " << rhResPacket->yourScore << std::endl;
            }
        }

        mobHp = 0;
        timer = 0;
        roomNum = 0;
        myNum = 0;

        inGameRun = false;
        syncRun = false;
        closesocket(udpSkt);
    }

    bool GetRaidScore(uint16_t startNum_) { // ������ ���� ���ھ�� false ��ȯ. �� �� ���� �����ϱ� üũ x
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
                std::cout << i + 1 << "�� ���̵� : " << (std::string)tempR.userId << " / ���� : " << tempR.score << std::endl;
            }

            std::cout << std::endl;

            if (reRes->rkCount != RANKING_USER_COUNT) return false;

            return true;
        }

        for (int i = rank; i < rank + reRes->rkCount; i++) {
            RANKING tempR;
            memcpy((char*)&tempR, ptr, sizeof(RANKING));
            ptr += sizeof(RANKING);
            std::cout << i << "�� ���̵� : " << (std::string)tempR.userId << " / ���� : " << tempR.score << std::endl;
        }

        std::cout << std::endl;

        if (reRes->rkCount != RANKING_USER_COUNT) return false;

        return true;
    }

    void GetShopInfo() {
        std::cout << "��� : " << userGold << "ĳ�� : " << userCash << "���ϸ��� : " << userMileage << '\n' << '\n';

        for (auto& s : shopItems) {

            switch (s.itemType) {
            case 0: { // ���
                std::cout << s.itemCode << "�� ��� ������ ";

                if (s.currencyType == 0) {
                    std::cout  << s.daysOrCount  <<"�ϱ� ����: " << s.itemPrice << "���" << '\n';
                }
                else if (s.currencyType == 1) {
                    std::cout << s.daysOrCount << "�ϱ� ����: " << s.itemPrice << "ĳ��" << '\n';
                }
                else if (s.currencyType == 2) {
                    std::cout << s.daysOrCount << "�ϱ� ����: " << s.itemPrice << "���ϸ���" << '\n';
                }
                break;
            }
            case 1: { // �Һ�
                std::cout << s.itemCode << "�� �Һ� ������ ";
                if (s.currencyType == 0) {
                    std::cout << s.daysOrCount << "�� ����: " << s.itemPrice << "���" << '\n';
                }
                else if (s.currencyType == 1) {
                    std::cout << s.daysOrCount << "�� ����: " << s.itemPrice << "ĳ��" << '\n';
                }
                else if (s.currencyType == 2) {
                    std::cout << s.daysOrCount << "�� ����: " << s.itemPrice << "���ϸ���" << '\n';
                }
                break;
            }
            case 2: { // ���
                std::cout << s.itemCode << "�� ��� ������ ";

                if (s.currencyType == 0) {
                    std::cout << s.daysOrCount << "�� ����: " << s.itemPrice << "���" << '\n';
                }
                else if (s.currencyType == 1) {
                    std::cout << s.daysOrCount << "�� ����: " << s.itemPrice << "ĳ��" << '\n';
                }
                else if (s.currencyType == 2) {
                    std::cout << s.daysOrCount << "�� ����: " << s.itemPrice << "���ϸ���" << '\n';
                }
                break;
            }
            default: break;
            }
        }
    }

    void BuyItemFromShop(uint16_t a, uint16_t b, uint16_t c) {

        SHOP_BUY_ITEM_REQUEST sbReq;
        sbReq.PacketId = (UINT16)PACKET_ID::SHOP_BUY_ITEM_REQUEST;
        sbReq.PacketLength = sizeof(SHOP_BUY_ITEM_REQUEST);
        sbReq.itemCode = a;
        sbReq.daysOrCount = b;
        sbReq.itemType = c;

        send(userSkt, (char*)&sbReq, sizeof(sbReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto sbRes = reinterpret_cast<SHOP_BUY_ITEM_RESPONSE*>(recvBuffer);
        std::cout << sbRes->position << std::endl;
        if (sbRes->isSuccess) {
            switch (sbRes->shopItemForSend.itemType) {
            case 0: { // ���
                EQUIPMENT acqEq;
                acqEq.itemCode = sbRes->shopItemForSend.itemCode;
                eq[sbRes->position] = acqEq;
                break;
            }
            case 1: { // �Һ�
                CONSUMABLES acqCs;
                acqCs.itemCode = sbRes->shopItemForSend.itemCode;
                cs[sbRes->position] = acqCs;
                break;
            }
            case 2: { // ���
                MATERIALS acqMt;
                acqMt.itemCode = sbRes->shopItemForSend.itemCode;
                mt[sbRes->position] = acqMt;
                break;
            }
            }

            switch (sbRes->currencyType) {
            case 0: { // ���
                userGold = sbRes->remainMoney;
                std::cout << "���� ���� !" << '\n';
                std::cout << "���� ��� : " << sbRes->remainMoney << '\n';
                break;
            }
            case 1: { // ĳ��
                userCash = sbRes->remainMoney;
                std::cout << "���� ���� !" << '\n';
                std::cout << "���� ĳ�� : " << sbRes->remainMoney << '\n';
                break;
            }
            case 2: { // ���ϸ���
                userMileage = sbRes->remainMoney;
                std::cout << "���� ���� !" << '\n';
                std::cout << "���� ���ϸ��� : " << sbRes->remainMoney << '\n';
                break;
            }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // ���� ��û �� �� ���� ���� �ð� ����
        }
        else {
            std::cout << "���� ����. �ݾ� ���� !" << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // ���� ��û �� �� ���� ���� �ð� ����
        }
    }

    void GetPassData() {
        for (auto& m : passInfoMap) {
            std::cout << "�н� ID : " << m.first << "/ ���� : " << m.second.passLevel << "/ ����ġ : " << m.second.passExp << '\n';
            std::cout << "�̺�Ʈ �Ⱓ : " << m.second.passInfo.eventStart << " ~ " << m.second.passInfo.eventEnd << '\n';

            for (auto& k : passDataMap[m.first]) {
                k.second.PrintPassData();
            }
            std::cout << '\n';
        }
    }

    void GetPassInfo() {
        for (auto& m : passInfoMap) {
            std::cout << "�н� ID : " << m.first << "/ ���� : " << m.second.passLevel << "/ ����ġ : " << m.second.passExp << '\n';
            std::cout << "�̺�Ʈ �Ⱓ : " << m.second.passInfo.eventStart << " ~ " << m.second.passInfo.eventEnd << '\n';
        }
        std::cout << '\n';
    }

    void GetPassItem(std::string passId_, uint16_t passLevel_, uint16_t passCurrenyType_) {

        GET_PASS_ITEM_REQUEST gpReq;
        gpReq.PacketId = (UINT16)PACKET_ID::GET_PASS_ITEM_REQUEST;
        gpReq.PacketLength = sizeof(GET_PASS_ITEM_REQUEST);
        strncpy_s(gpReq.passId, passId_.c_str(), MAX_PASS_ID_LEN + 1);
        gpReq.passLevel = passLevel_;
        gpReq.passCurrencyType = passCurrenyType_;

        send(userSkt, (char*)&gpReq, sizeof(gpReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto sbRes = reinterpret_cast<GET_PASS_ITEM_RESPONSE*>(recvBuffer);
        
        if (!sbRes->isSuccess) {
            std::cout << "ȹ�� ����. �н� ���� �̴�" << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // �н� ȹ�� ��û �� ȹ�� ���� �ð� ����
            return;
        }

        PassItemForSend tempP = sbRes->passItemForSend;

        switch (tempP.itemType) {
            case 0: { // ���
                EQUIPMENT acqEq;
                acqEq.itemCode = tempP.itemCode;
                eq[sbRes->position] = acqEq;
            }
            break;
            case 1: { // �Һ�
                CONSUMABLES acqCs;
                acqCs.itemCode = tempP.itemCode;
                cs[sbRes->position] = acqCs;
            }
            break;
            case 2: { // ���
                MATERIALS acqMt;
                acqMt.itemCode = tempP.itemCode;
                mt[sbRes->position] = acqMt;
            }
            break;
        }

        std::cout << "ȹ�� ����" << '\n';

        // Ŭ���̾�Ʈ���� ó���ϴ� ���� �ƴ� ��� ������ �������� ������ �����ͷθ� ����ϵ��� ����
        passDataMap[sbRes->passId][{sbRes->passLevel, sbRes->passCurrencyType}].getCheck = sbRes->passAcq;

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // �н� ȹ�� ��û �� ȹ�� ���� �ð� ����
    }

    void GetPassExp(std::string& passId_, uint16_t missionNum_){

        PASS_EXP_UP_REQUEST peuReq;
        peuReq.PacketId = (UINT16)PACKET_ID::PASS_EXP_UP_REQUEST;
        peuReq.PacketLength = sizeof(PASS_EXP_UP_REQUEST);
        strncpy_s(peuReq.passId, passId_.c_str(), MAX_PASS_ID_LEN + 1);
        peuReq.missionNum = missionNum_;

        send(userSkt, (char*)&peuReq, sizeof(peuReq), 0);
        recv(userSkt, recvBuffer, PACKET_SIZE, 0);

        auto sbRes = reinterpret_cast<PASS_EXP_UP_RESPONSE*>(recvBuffer);

        if (!sbRes->isSuccess) {
            std::cout << "������ �̼����� Ȯ�����ּ��� !" << '\n';
            return;
        }

        std::cout << "�н� ID : " << sbRes->passId
            << " ����ġ ���� ����. ���� �н� ���� : " << sbRes->passLevel << ", �н� ����ġ : " << sbRes->passExp << '\n';

        passInfoMap[sbRes->passId].passLevel = sbRes->passLevel;
        passInfoMap[sbRes->passId].passExp = sbRes->passExp;
    }

private:

    // SYSTEM
    SOCKET userSkt;
    SOCKET sessionSkt;
    SOCKET gameServerSkt;
    SOCKET channelSkt;
    SOCKET udpSkt;

    char recvBuffer[PACKET_SIZE];

    std::vector<uint16_t> tempServerUserCounts;
    std::vector<uint16_t> tempChannelUserCounts;


    // USERINFO
    std::atomic<unsigned int> exp;
    std::atomic<uint16_t> level;

    uint32_t userGold = 0;
    uint32_t userCash = 0;
    uint32_t userMileage = 0;

    unsigned int raidScore;

    std::string userId = "quokka";

    std::vector<EQUIPMENT> eq{ INVENTORY_SIZE };
    std::vector<CONSUMABLES> cs{ INVENTORY_SIZE };
    std::vector<MATERIALS> mt{ INVENTORY_SIZE };


    // PASSINFO
    std::unordered_map<std::string, USERPASSINFO> passInfoMap;
    std::unordered_map<std::string, std::map<PassDataKey, PassItem_Client, PassDataKeySort>> passDataMap;

    // SHOPINFO
    std::vector<ShopItemForSend> shopItems;


    // RAID
    std::atomic<bool> syncRun = false;
    std::atomic<bool> inGameRun = false;
    std::atomic<int> mobHp;

    uint16_t timer;
    uint16_t roomNum;
    uint16_t myNum;
    uint16_t mapNum;
    uint16_t currentServer = 0;
    uint16_t currentChannel = 0;

    std::thread syncThread;
    std::thread inGameThread;
    sockaddr_in udpAddr;

    std::chrono::time_point<std::chrono::steady_clock> rEndTime;

    char recvUDPBuffer[sizeof(unsigned int)];

    std::vector<RAID_USERINFO> raidUserInfos;
};