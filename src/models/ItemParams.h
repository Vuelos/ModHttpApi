#pragma once

#include <string>
#include <cstdint>

struct ItemParams
{
    std::string playerName;
    uint32_t    itemEntry = 0;
    int32_t     count     = 1;
};
