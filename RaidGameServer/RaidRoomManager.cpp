#include "RaidRoomManager.h"

bool RaidRoomManager::CreateTimeCheckThread() {
    timeChekcRun = true;
    timeCheckThread = std::thread([this]() {TimeCheckThread(); });
    std::cout << "TimeCheckThread Start" << std::endl;
    return true;
}

bool RaidRoomManager::CreateTickRateThread() {

}

void RaidRoomManager::TickRateThread() {

}

void RaidRoomManager::DeleteMob(Room* room_) { // 몹 직접 죽였을때

    if (room_->TimeOverCheck()) { // 타임아웃 체크되서 이미 endRoomCheckSet에서 삭제 됬을때 (방 삭제만 처리)
        roomManager->DeleteRoom(room_->GetRoomNum());
        roomNumQueue.push(room_->GetRoomNum());
        return;
    }

    {    // 타임아웃전에 몹을 잡아서 endRoomCheckSet 직접 삭제
        std::lock_guard<std::mutex> guard(mDeleteRoom);
        for (auto iter = endRoomCheckSet.begin(); iter != endRoomCheckSet.end(); iter++) {
            if (*iter == room_) {
                delete* iter;
                endRoomCheckSet.erase(iter);
                break;
            }
        }
    }

    roomManager->DeleteRoom(room_->GetRoomNum());
    roomNumQueue.push(room_->GetRoomNum());
}

void RaidRoomManager::TimeCheckThread() {
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
