#pragma once
#include <string>

static const std::string BuyItemScript = R"(
local currencyKey = KEYS[1]
local invenKey    = KEYS[2]

local moneyField  = ARGV[1]
local price       = tonumber(ARGV[2])
local pos         = ARGV[3]
local itemValue   = ARGV[4]

local moneyStr = redis.call('HGET', currencyKey, moneyField)
if not moneyStr then
  return -3
end

local money = tonumber(moneyStr)
if not money then
  return -4
end

if money < price then
  return -1
end

if redis.call('HEXISTS', invenKey, pos) == 1 then
  return -2
end

redis.call('HSET', invenKey, pos, itemValue)

-- 차감 후 남은 금액을 반환
local remain = redis.call('HINCRBY', currencyKey, moneyField, -price)
return remain
)";

