#pragma once
#pragma comment(lib, "ws2_32.lib") // 비주얼에서 소켓프로그래밍 하기 위한 것
#pragma comment(lib,"mswsock.lib") //AcceptEx를 사용하기 위한 것

#define SERVER_IP "127.0.0.1"
#define TO_USER_PORT 9091
#define PACKET_SIZE 1024
#define SEND_QUEUE_CNT 5
#define MAX_OVERLAP_CNT 10

#include <jwt-cpp/jwt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <string>
#include <thread>
#include <atomic>
#include <sw/redis++/redis++.h>
//#include <boost/lockfree/queue.hpp>

#include "Packet.h"
#include "Define.h"
#include "User.h"
#include "MySQLManager.h"

class UserProcessor {
public:
    ~UserProcessor() {
        delete user;
        PostQueuedCompletionStatus(u_IOCPHandle, 0, 0, nullptr);

        if (userProcThread.joinable()) {
            userProcThread.join();
        }

        CloseHandle(u_IOCPHandle);
        closesocket(userIOSkt);
        WSACleanup();

        std::cout << "userProcThread End" << std::endl;
    }

	bool init(uint16_t threadCnt_, std::shared_ptr<sw::redis::RedisCluster> redis_, MySQLManager* mysqlManager_) {
        WSADATA wsadata;
        int check = 0;
        threadCnt = threadCnt_;

        check = WSAStartup(MAKEWORD(2, 2), &wsadata);
        if (check) {
            std::cout << "WSAStartup Fail" << std::endl;
            return false;
        }

        userIOSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
        if (userIOSkt == INVALID_SOCKET) {
            std::cout << "Make Server Socket Fail" << std::endl;
            return false;
        }

        SOCKADDR_IN addr;
        addr.sin_port = htons(TO_USER_PORT);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        check = bind(userIOSkt, (SOCKADDR*)&addr, sizeof(addr));
        if (check) {
            std::cout << "bind fail" << std::endl;
            return false;
        }

        check = listen(userIOSkt, SOMAXCONN);
        if (check) {
            std::cout << "listen fail" << std::endl;
            return false;
        }

        u_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, threadCnt_);
        if (u_IOCPHandle == NULL) {
            std::cout << "Make Iocp Hande Fail" << std::endl;
            return false;
        }

        auto bIOCPHandle = CreateIoCompletionPort((HANDLE)userIOSkt, u_IOCPHandle, (ULONG_PTR)0, 0);
        if (bIOCPHandle == nullptr) {
            std::cout << "Bind Iocp Hande Fail" << std::endl;
            return false;
        }

        user = new User();
        user->init(u_IOCPHandle);

        //for (int i = 0; i < MAX_OVERLAP_CNT; i++) {
        //    OverlappedTCP* temp = new OverlappedTCP;
        //    overLapPool.push(temp);
        //}

        redis = redis_;
        mysqlManager = mysqlManager_;

        CreateUserProcThread();
        
        user->PostAccept(userIOSkt, u_IOCPHandle);

        return true;
	}

    void CreateUserProcThread() {
        userProcRun = true;
        userProcThread = std::thread([this]() {WorkRun(); });
    }

    void WorkRun() {
        LPOVERLAPPED lpOverlapped = NULL;
        DWORD dwIoSize = 0;
        User* tempUser = nullptr;
        bool gqSucces = TRUE;

        while (userProcRun) {
            gqSucces = GetQueuedCompletionStatus(
                u_IOCPHandle,
                &dwIoSize,
                (PULONG_PTR)&tempUser,
                &lpOverlapped,
                INFINITE
            );

            if (gqSucces && dwIoSize == 0 && lpOverlapped == NULL) { // Server End Request
                userProcRun = false;
                continue;
            }

            auto overlappedTCP = (OverlappedTCP*)lpOverlapped;

            tempUser = overlappedTCP->user;
            int a = overlappedTCP->a;

            if (!gqSucces || (dwIoSize == 0 && overlappedTCP->taskType != TaskType::ACCEPT)) { // User Disconnect
                std::cout << "비정상 종료 감지. 소켓 초기화" << std::endl;
                tempUser->Reset(u_IOCPHandle);
                tempUser->PostAccept(userIOSkt, u_IOCPHandle);
                continue;
            }

            if (a==0) {
                tempUser->UserRecv();
            }
            else if (a==1) {
                auto k = reinterpret_cast<PACKET_HEADER*>(overlappedTCP->wsaBuf.buf);
                
                if (k->PacketId == (uint16_t)SESSIONPACKET_ID::USER_GAMESTART_REQUEST) {
                    auto ugReq = reinterpret_cast<USER_GAMESTART_REQUEST*>(overlappedTCP->wsaBuf.buf);
                    GameStart(tempUser, ugReq);
                    delete[] overlappedTCP->wsaBuf.buf;
                    delete overlappedTCP;
                    tempUser->UserRecv();
                }

                else if (k->PacketId == (uint16_t)SESSIONPACKET_ID::USERINFO_REQUEST) { // GetUserInfo
                    auto ugReq = reinterpret_cast<USERINFO_REQUEST*>(overlappedTCP->wsaBuf.buf);
                    GetUserInfo(tempUser, ugReq);
                    delete[] overlappedTCP->wsaBuf.buf;
                    delete overlappedTCP;
                    tempUser->UserRecv();
                }

                else if (k->PacketId == (uint16_t)SESSIONPACKET_ID::EQUIPMENT_REQUEST) { // GetEquipment
                    GetEquipment(tempUser);
                    delete[] overlappedTCP->wsaBuf.buf;
                    delete overlappedTCP;
                    tempUser->UserRecv();
                }

                else if (k->PacketId == (uint16_t)SESSIONPACKET_ID::CONSUMABLES_REQUEST) { // GetConsumables
                    GetConsumables(tempUser);
                    delete[] overlappedTCP->wsaBuf.buf;
                    delete overlappedTCP;
                    tempUser->UserRecv();
                }

                else if (k->PacketId == (uint16_t)SESSIONPACKET_ID::MATERIALS_REQUEST) { // GetMaterials
                    GetMaterials(tempUser);
                    delete[] overlappedTCP->wsaBuf.buf;
                    delete overlappedTCP;
                    tempUser->UserRecv();
                }
                //ZeroMemory(overlappedTCP,sizeof(OverlappedTCP)); // Push After Init
                //overLapPool.push(overlappedTCP); // Push OverLappedTcp
                //sendQueueSize.fetch_add(1);
            }
            else if (a==2) {
                delete[] overlappedTCP->wsaBuf.buf;
                delete overlappedTCP;
            }
            else if (a == 3) { // Last UserInfo Send
                delete[] overlappedTCP->wsaBuf.buf;
                delete overlappedTCP;

                tempUser->Reset(u_IOCPHandle);
                tempUser->PostAccept(userIOSkt, u_IOCPHandle);
            }
        }
    }

    void GetUserInfo(User* tempUser, USERINFO_REQUEST* uiReq) { // 레디스 클러스터에 뒤에 {}를 ID로 하면 ID는 한번씩 유저가 바꾸니까 변하지 않는 PK로 설정.
        std::pair<uint32_t,USERINFO> userInfoPk = mysqlManager->GetUserInfoById(uiReq->userId);

        if (userInfoPk.first == 0) { // 0번 pk를 받으면 실패
            std::cout << "GetUserInfo Fail" << std::endl;
            tempUser->Reset(u_IOCPHandle);
            tempUser->PostAccept(userIOSkt, u_IOCPHandle);
            return;
        }

        tempUser->SetPk(userInfoPk.first);
        USERINFO_RESPONSE uiRes;
        uiRes.PacketId = (UINT16)SESSIONPACKET_ID::USERINFO_RESPONSE;
        uiRes.PacketLength = sizeof(USERINFO_RESPONSE);
        uiRes.UserInfo = userInfoPk.second;
        tempUser->SendUserInfo(uiRes);
        std::cout << "유저 정보 게임 서버에 업로드 성공" << std::endl;
    }

    void GetEquipment(User* tempUser) {

        std::pair<uint16_t, char*> eq = mysqlManager->GetUserEquipByPk(std::to_string(tempUser->GetPk()));

        if (eq.first==100) { // 100을 받으면 인벤토리 Get 실패
            std::cout << "GetUserEquip Fail" << std::endl;
            tempUser->Reset(u_IOCPHandle);
            tempUser->PostAccept(userIOSkt, u_IOCPHandle);
            return;
        }

        EQUIPMENT_RESPONSE eqSend;
        eqSend.PacketId = (UINT16)SESSIONPACKET_ID::EQUIPMENT_RESPONSE;
        eqSend.PacketLength = sizeof(EQUIPMENT_RESPONSE);
        eqSend.eqCount = eq.first;
        std::memcpy(eqSend.Equipments, eq.second, MAX_INVEN_SIZE + 1);
        tempUser->SendEquipment(eqSend);
        std::cout << "유저 장비 게임 서버에 아이템 업로드 성공" << std::endl;
        delete[] eq.second;
    }

    void GetConsumables(User* tempUser) {

        std::pair<uint16_t, char*> es = mysqlManager->GetUserConsumablesByPk(std::to_string(tempUser->GetPk()));

        if (es.first == 100) {
            std::cout << "GetUserConsumables Fail" << std::endl;
            tempUser->Reset(u_IOCPHandle);
            tempUser->PostAccept(userIOSkt, u_IOCPHandle);
            return;
        }

        CONSUMABLES_RESPONSE csSend;
        csSend.PacketId = (UINT16)SESSIONPACKET_ID::CONSUMABLES_RESPONSE;
        csSend.PacketLength = sizeof(CONSUMABLES_RESPONSE);
        csSend.csCount = es.first;
        std::memcpy(csSend.Consumables, es.second, MAX_INVEN_SIZE + 1);
        tempUser->SendConsumables(csSend);
        std::cout << "유저 소비 아이템 게임 서버에 업로드 성공" << std::endl;
        delete[] es.second;
    }

    void GetMaterials(User* tempUser) {
         
        std::pair<uint16_t, char*> em = mysqlManager->GetUserMaterialsByPk(std::to_string(tempUser->GetPk()));

        if (em.first == 100) {
            std::cout << "GetUserMaterials Fail" << std::endl;
            tempUser->Reset(u_IOCPHandle);
            tempUser->PostAccept(userIOSkt, u_IOCPHandle);
            return;
        }

        MATERIALS_RESPONSE mtSend;
        mtSend.PacketId = (UINT16)SESSIONPACKET_ID::MATERIALS_RESPONSE;
        mtSend.PacketLength = sizeof(MATERIALS_RESPONSE);
        mtSend.mtCount = em.first;
        std::memcpy(mtSend.Materials, em.second, MAX_INVEN_SIZE + 1);
        tempUser->SendMaterials(mtSend);
        std::cout << "유저 재료 아이템 게임 서버에 업로드 성공" << std::endl;
        delete[] em.second;
    }

    void GameStart(User* tempUser, USER_GAMESTART_REQUEST* ugReq) {

        std::string token = jwt::create()
            .set_issuer("session_server")
            .set_subject("Login_check")
            .set_expires_at(std::chrono::system_clock::now() + 
             std::chrono::seconds{ 600 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        std::string tag = "{" + std::string(ugReq->userId) + "}";
        std::string key = "jwtcheck:" + tag;

        auto pipe = redis->pipeline(tag);
        pipe.hset(key, token, std::to_string(tempUser->GetPk()))
            .expire(key, 15); // set ttl 1 hour
        pipe.exec();

		USER_GAMESTART_RESPONSE ugRes; 
		ugRes.PacketId = (UINT16)SESSIONPACKET_ID::USER_GAMESTART_RESPONSE;
		ugRes.PacketLength = sizeof(USER_GAMESTART_RESPONSE);
		strncpy_s(ugRes.Token, token.c_str(), 256);
        tempUser->SendGameStart(ugRes);
    }

private:
    bool userProcRun = false;
    //std::atomic<uint16_t> sendQueueSize{ 0 };

    uint16_t threadCnt = 0;

	SOCKET userIOSkt;
    SOCKET userSkt;
    HANDLE u_IOCPHandle;

    std::thread userProcThread;

    std::shared_ptr<sw::redis::RedisCluster> redis;

    std::string userToken;

    MySQLManager* mysqlManager;

    User* user;

    //boost::lockfree::queue<OverlappedTCP*> SendQueue{ SEND_QUEUE_CNT };
    //boost::lockfree::queue<OverlappedTCP*> overLapPool{ MAX_OVERLAP_CNT };
};