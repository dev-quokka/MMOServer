#include "MatchingManager.h"

bool MatchingManager::Init() {
    for (int i = 1; i <= USER_MAX_LEVEL / 3 + 1; i++) { // Max i = MaxLevel/3 + 1 (Level Check Set)
        matchingMap.emplace(i, std::set<MatchingRoom*, MatchingRoomComp>());
    }

    for (int i = 1; i <= MAX_ROOM; i++) { // Room Number Set
        roomNumQueue.push(i);
    }

    CreateMatchThread();

    return true;
}

bool MatchingManager::CreateMatchThread() {
    matchRun = true;
    matchingThread = std::thread([this]() {MatchingThread(); });
    std::cout << "MatchingThread Start" << std::endl;
    return true;
}

bool MatchingManager::Insert(uint16_t userPk_, uint16_t userCenterObjNum_, uint16_t userGroupNum_) {
    MatchingRoom* tempRoom = new MatchingRoom(userPk_, userCenterObjNum_);

    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;
    uint16_t groupNum = userGroupNum_;

    if (matchingMap.find(accessor, groupNum)) { // Insert Success
        accessor->second.insert(tempRoom);
        std::cout << "Insert Group " << groupNum << std::endl;
        return true;
    }

    // Match Queue Full || Insert Fail
    return false;
}

uint16_t MatchingManager::CancelMatching(uint16_t userCenterObjNum_, uint16_t userGroupNum_) {
    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;
    uint16_t groupNum = userGroupNum_;
    matchingMap.find(accessor, groupNum);
    auto tempM = accessor->second;

    {
        std::lock_guard<std::mutex> guard(mDeleteMatch);
        for (auto iter = tempM.begin(); iter != tempM.end(); iter++) {
            if ((*iter)->userCenterObjNum == userCenterObjNum_) { // 매칭 취소한 유저 찾기
                delete* iter;
                tempM.erase(iter);
                return true;
            }
        }
    }

    return false;
}

void MatchingManager::MatchingThread() {
    uint16_t cnt = 1;
    uint16_t tempRoomNum = 0;
    MatchingRoom* tempMatching1;
    MatchingRoom* tempMatching2;

    while (matchRun) {
        if (tempRoomNum == 0) { // 룸넘 한번 뽑아서 새로 뽑아야함
            if (roomNumQueue.pop(tempRoomNum)) { // Exist Room Num
                for (int i = cnt; i <= 6; i++) {
                    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor1;
                    if (matchingMap.find(accessor1, i)) { // i번째 레벨 그룹 넘버 체크

                        if (!accessor1->second.empty()) { // 유저 한명이라도 있음
                            tempMatching1 = *accessor1->second.begin();

                            accessor1->second.erase(accessor1->second.begin());

                            if (!accessor1->second.empty()) { // 두번째 대기 유저가 있음
                                tempMatching2 = *accessor1->second.begin();

                                accessor1->second.erase(accessor1->second.begin());

                                { // 두명 유저 방 만들어서 넣어주기
                                    MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER rMatchingResPacket;

                                    // Send to User1 With User2 Info
                                    rMatchingResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER;
                                    rMatchingResPacket.PacketLength = sizeof(MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER);
                                    rMatchingResPacket.roomNum = tempRoomNum;
									rMatchingResPacket.userObjNum1 = tempMatching1->userCenterObjNum;
									rMatchingResPacket.userObjNum2 = tempMatching2->userCenterObjNum;

									connServersManager->FindUser(0)->  // 중앙 서버로 매칭된 유저 정보 전달
                                        PushSendMsg(sizeof(MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER), (char*)&rMatchingResPacket);

                                    //connServersManager->FindUser(1)-> // 매칭된 게임 서버로 매칭된 유저 정보 전달
                                    //    PushSendMsg(sizeof(MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER), (char*)&rMatchingResPacket);
                                }

                                delete tempMatching1;
                                delete tempMatching2;

                                if (cnt == 6) cnt = 1; // 방금 매칭한 다음 번호 그룹부터 매칭을 위해 cnt 체크
                                else cnt++;
                                tempRoomNum = 0;
                                break;
                            }
                            else { // 현재 레벨에 대기 유저 한명이라 다시 넣기
                                accessor1->second.insert(tempMatching1);
                            }
                        }
                    }
                }
            }
            else { // Not Exist Room Num
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else { // 뽑아둔 룸넘 있음
            for (int i = 1; i <= 6; i++) {
                tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor1;
                if (matchingMap.find(accessor1, i)) { // i번째 레벨 그룹 넘버 체크

                    if (!accessor1->second.empty()) { // 유저 한명이라도 있음
                        tempMatching1 = *accessor1->second.begin();

                        accessor1->second.erase(accessor1->second.begin());

                        if (!accessor1->second.empty()) { // 두번째 대기 유저가 있음
                            tempMatching2 = *accessor1->second.begin();

                            accessor1->second.erase(accessor1->second.begin());

                            { // 두명 유저 방 만들어서 넣어주기
                                MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER rMatchingResPacket;

                                // Send to User1 With User2 Info
                                rMatchingResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER;
                                rMatchingResPacket.PacketLength = sizeof(MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER);
                                rMatchingResPacket.roomNum = tempRoomNum;
                                rMatchingResPacket.userObjNum1 = tempMatching1->userCenterObjNum;
                                rMatchingResPacket.userObjNum2 = tempMatching2->userCenterObjNum;

                                connServersManager->FindUser(0)->  // 중앙 서버로 매칭된 유저 정보 전달
                                    PushSendMsg(sizeof(MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER), (char*)&rMatchingResPacket);

                                //connServersManager->FindUser(1)-> // 매칭된 게임 서버로 매칭된 유저 정보 전달
                                //    PushSendMsg(sizeof(MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER), (char*)&rMatchingResPacket);
                            }

                            delete tempMatching1;
                            delete tempMatching2;

                            if (cnt == 6) cnt = 1;
                            else cnt++;
                            tempRoomNum = 0;
                            break;
                        }
                        else { // 현재 레벨에 대기 유저 한명이라 다시 넣기
                            accessor1->second.insert(tempMatching1);
                        }
                    }
                }
            }
        }
    }
}