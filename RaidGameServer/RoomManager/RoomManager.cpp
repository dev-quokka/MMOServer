#include "RoomManager.h"

bool RoomManager::init() {
    udpSkt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (udpSkt == INVALID_SOCKET) {
        std::cout << "Server Socket 생성 실패" << std::endl;
        return false;
    }

    sockaddr_in serverUdpAddr{};
    serverUdpAddr.sin_family = AF_INET;
    serverUdpAddr.sin_addr.s_addr = INADDR_ANY;
    serverUdpAddr.sin_port = htons(UDP_PORT);

    if (bind(udpSkt, (sockaddr*)&serverUdpAddr, sizeof(serverUdpAddr))) {
        std::cout << "bind 함수 실패:" << WSAGetLastError() << std::endl;
        return false;
    }

    if (!CreateTimeCheckThread()) {
        std::cout << "TimeCheckThread Fail" << std::endl;
        return false;
    }

    if (!CreateTickRateThread()) {
        std::cout << "TickRateThread Fail" << std::endl;
        return false;
    }

    return true;
}

bool RoomManager::CreateTimeCheckThread() {
    timeChekcRun = true;
    timeCheckThread = std::thread([this]() {TimeCheckThread(); });
    std::cout << "TimeCheckThread Start" << std::endl;
    return true;
}

bool RoomManager::CreateTickRateThread() {
    tickRateRun = true;
    tickRateThread = std::thread([this]() {TickRateThread(); });
    std::cout << "TickRateThread Start" << std::endl;
    return true;
}

void RoomManager::TickRateThread() {
    auto tickRate = std::chrono::milliseconds(1000 / TICK_RATE);

    while (tickRateRun) {
        auto timeCheck = std::chrono::steady_clock::now() + tickRate;

        if (roomMap.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        for (auto iter = roomMap.begin(); iter != roomMap.end(); iter++) { // 게임 중인 방에 틱 레이트 동기화 메시지 전송
            iter->second->SendSyncMsg();
        }

        while (timeCheck > std::chrono::steady_clock::now()) { // 틱레이트 까지 대기
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

bool RoomManager::MakeRoom(uint16_t roomNum_, uint16_t mapNum_, uint16_t timer_, int mobHp_, RaidUserInfo* raidUserInfo1, RaidUserInfo* raidUserInfo2) {
    Room* room = new Room(&udpSkt);

    if (!room->Set(roomNum_, mapNum_, timer_, mobHp_, raidUserInfo1, raidUserInfo2)) {
        std::cout << "Making Room Fail in RoomManager" << std::endl;
        return false;
    }

    roomMap[roomNum_] = room;
    return true;
}

Room* RoomManager::GetRoom(uint16_t roomNum_) {
    return roomMap[roomNum_];
}

void RoomManager::DeleteRoom(uint16_t roomNum_) {
    Room* room = roomMap[roomNum_];
    delete room;
    roomMap.erase(roomNum_);
}

void RoomManager::DeleteMob(Room* room_) { // 몹 직접 죽였을때
    if (room_->TimeOverCheck()) { // 타임아웃 체크되서 이미 endRoomCheckSet에서 삭제 됬을때 (방 삭제만 처리)
        DeleteRoom(room_->GetRoomNum());
        return;
    }

    { // 타임아웃전에 몹을 잡아서 endRoomCheckSet 직접 삭제
        std::lock_guard<std::mutex> guard(mDeleteRoom);
        for (auto iter = endRoomCheckSet.begin(); iter != endRoomCheckSet.end(); iter++) {
            if (*iter == room_) {
                delete* iter;
                endRoomCheckSet.erase(iter);
                break;
            }
        }
    }

    DeleteRoom(room_->GetRoomNum());
}

void RoomManager::TimeCheckThread() {
    std::chrono::steady_clock::time_point now;
    Room* room_;
    while (timeChekcRun) {
        if (!endRoomCheckSet.empty()) { // Room Exist
            room_ = (*endRoomCheckSet.begin());
            if (room_->GetEndTime() <= std::chrono::steady_clock::now()) {
                std::cout << "타임 아웃. 레이드 종료" << std::endl;
                room_->TimeOver();
                endRoomCheckSet.erase(endRoomCheckSet.begin());
            }
            else {
                std::cout << "종료대기" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else { // Room Not Exist
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
