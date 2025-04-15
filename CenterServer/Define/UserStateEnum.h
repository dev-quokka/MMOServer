#pragma once

#include <cstdint>

enum class UserState : uint16_t {
	offline = 1,
	online = 2,

	serverSwitching = 5,

	raidMatching = 11,
	inRaid = 12,
};