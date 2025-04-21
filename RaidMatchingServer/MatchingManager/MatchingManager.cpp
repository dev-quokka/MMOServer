#include "MatchingManager.h"

// ============================== INITIALIZATION ===============================

bool MatchingManager::Init(ConnServersManager* connServersManager_) {
    connServersManager = connServersManager_;

    for (int i = 1; i <= USER_LEVEL_GROUPS; i++) {
        matchingMap.emplace(i, std::set<MatchingRoom*, MatchingRoomComp>());
    }

    for (int i = 1; i <= MAX_ROOM; i++) { // Set room number
        roomNumQueue.push(i);
    }

    CreateMatchThread(MATCHING_THREAD_COUNT);

    return true;
}


// ============================= MATCHING MANAGEMENT =============================

bool MatchingManager::CreateMatchThread(uint16_t matchThreadCount_) {
    matchRun = true;

    if (matchThreadCount_ == 0 || USER_LEVEL_GROUPS % matchThreadCount_ != 0 || USER_LEVEL_GROUPS < matchThreadCount_) {
        std::cout << "Invalid matchThreadCount" << std::endl;
        return false;
    }

    uint16_t startIdx;
    uint16_t endIdx;

    try {
        for (int i = 0; i < MATCHING_THREAD_COUNT; i++) {
            startIdx = (USER_LEVEL_GROUPS * i) / matchThreadCount_ + 1;
            endIdx = (USER_LEVEL_GROUPS * (i + 1)) / matchThreadCount_;

            matchingThreads.emplace_back([this, startIdx, endIdx]() { MatchingThread(startIdx, endIdx); });
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Failed to Create Matching Threads : " << e.what() << std::endl;
        return false;
    }

    return true;
}

uint16_t MatchingManager::Insert(uint16_t userPk_, uint16_t userCenterObjNum_, uint16_t userGroupNum_) {
    MatchingRoom* tempRoom = new MatchingRoom(userPk_, userCenterObjNum_);

    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;
    uint16_t groupNum = userGroupNum_;

    if (matchingMap.find(accessor, groupNum)) { // Insert Success
        auto [iter, inserted] = accessor->second.insert(tempRoom);
        return userCenterObjNum_;
    }

    std::cout << "Fail to insert pk : " << userPk_ << std::endl;
    return 0;
}

uint16_t MatchingManager::CancelMatching(uint16_t userCenterObjNum_, uint16_t userGroupNum_) {
    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;
    uint16_t groupNum = userGroupNum_;
    matchingMap.find(accessor, groupNum);
    auto tempM = accessor->second;

    {
        std::lock_guard<std::mutex> guard(mDeleteMatch);
        for (auto iter = tempM.begin(); iter != tempM.end(); iter++) {
            if ((*iter)->userCenterObjNum == userCenterObjNum_) { // Find users who canceled matching
                delete* iter;
                tempM.erase(iter);
                return true;
            }
        }
    }

    return false;
}

void MatchingManager::InserRoomNum(uint16_t roomNum_) {
	roomNumQueue.push(roomNum_); // Insert room number
}

void MatchingManager::MatchingThread(uint16_t groupStartIdx_, uint16_t groupEndIdx_) {
    uint16_t cnt = groupStartIdx_;
    uint16_t tempRoomNum = 0;
    std::vector<MatchingRoom*> tempMatchedUser;

    while (matchRun) {
        if (tempRoomNum == 0) { // Need to select a new Room Number
            if (roomNumQueue.pop(tempRoomNum)) { // Select a new Room Number
                for (int i = cnt; i <= groupEndIdx_; i++) {
                    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor1;

                    if (matchingMap.find(accessor1, i)) { // Check the level group number i

                        if (accessor1->second.size() < MAX_RAID_ROOM_PLAYERS) continue;

                        for (int j = 0; j < MAX_RAID_ROOM_PLAYERS; j++) {
                            MatchingRoom* tempMatching = *accessor1->second.begin();
                            tempMatchedUser.emplace_back(tempMatching);
                            accessor1->second.erase(accessor1->second.begin());
                        }

                        for (int k = 0; k < tempMatchedUser.size(); k++) {
                            MATCHING_REQUEST_TO_GAME_SERVER rMatchingResPacket;
                            rMatchingResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_REQUEST_TO_GAME_SERVER;
                            rMatchingResPacket.PacketLength = sizeof(MATCHING_REQUEST_TO_GAME_SERVER);
                            rMatchingResPacket.roomNum = tempRoomNum;
                            rMatchingResPacket.userCenterObjNum = tempMatchedUser[k]->userCenterObjNum;
                            rMatchingResPacket.userPk = tempMatchedUser[k]->userPk;

                            connServersManager->GetGameServerObjNum(1)->  // Send matched user data to the game server
                                PushSendMsg(sizeof(MATCHING_REQUEST_TO_GAME_SERVER), (char*)&rMatchingResPacket);

                            delete tempMatchedUser[k];
                        }

						std::cout << "Matched Success Room Num : " << tempRoomNum << std::endl;

                        tempRoomNum = 0; // If roomNum is used, reset it to 0
                        tempMatchedUser.clear();

                        if (cnt == groupEndIdx_) cnt = 1; // Continue matchmaking check from the next group
                        else cnt++;
                        break;
                    }
                }
            }
            else { // Not Exist Room Num
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else { // Exist Room Num
            for (int i = cnt; i <= groupEndIdx_; i++) {
                tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor1;

                if (matchingMap.find(accessor1, i)) { // Check the level group number i

                    if (accessor1->second.size() < MAX_RAID_ROOM_PLAYERS) continue;

                    for (int j = 0; j < MAX_RAID_ROOM_PLAYERS; j++) {
                        MatchingRoom* tempMatching = *accessor1->second.begin();
                        tempMatchedUser.emplace_back(tempMatching);
                        accessor1->second.erase(accessor1->second.begin());
                    }

                    MATCHING_REQUEST_TO_GAME_SERVER rMatchingResPacket;
                    rMatchingResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_REQUEST_TO_GAME_SERVER;
                    rMatchingResPacket.PacketLength = sizeof(MATCHING_REQUEST_TO_GAME_SERVER);
                    rMatchingResPacket.roomNum = tempRoomNum;

                    for (int k = 0; k < tempMatchedUser.size(); k++) {
                        rMatchingResPacket.userCenterObjNum = tempMatchedUser[k]->userCenterObjNum;
                        rMatchingResPacket.userPk = tempMatchedUser[k]->userPk;

                        connServersManager->GetGameServerObjNum(1)->  // Send matched user data to the game server
                            PushSendMsg(sizeof(MATCHING_REQUEST_TO_GAME_SERVER), (char*)&rMatchingResPacket);

                        delete tempMatchedUser[k];
                    }

                    std::cout << "Matched Success Room Num : " << tempRoomNum << std::endl;

                    tempRoomNum = 0; // If roomNum is used, reset it to 0
                    tempMatchedUser.clear();

                    if (cnt == groupEndIdx_) cnt = 1; // Continue matchmaking check from the next group
                    else cnt++;
                    break;
                }
            }
        }
    }
}