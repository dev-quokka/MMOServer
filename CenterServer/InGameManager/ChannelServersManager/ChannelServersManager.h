#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <thread>

#include "ChannelServer.h"

class ChannelServersManager{
public:
	bool init();
	bool EnterChannelServer(uint16_t channelNum_); // 유저 해당 서버 입장
	void LeaveChannelServer(uint16_t channelNum_); // 유저 해당 서버 퇴장 or 해당 서버 입장 실패 응답
	std::vector<uint16_t> GetServerCounts(); // 채널 인원 수를 반환하는 함수

private:
	std::vector<ChannelServer*> servers;
};
