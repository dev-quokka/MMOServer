#pragma once

std::unordered_map<ServerType, ServerAddress> ServerAddressMap = { // Set server addresses
    { ServerType::CashServer,     { "127.0.0.1", 5050 } },
    { ServerType::CenterServer,     { "127.0.0.1", 9090 } },
    { ServerType::ChannelServer01, { "127.0.0.1", 9211 } },
    { ServerType::ChannelServer02, { "127.0.0.1", 9221 } },
    { ServerType::RaidGameServer01, { "127.0.0.1", 9510 } },
    { ServerType::LoginServer,   { "127.0.0.1", 9091 } },
    { ServerType::MatchingServer,   { "127.0.0.1", 9131 } },
};