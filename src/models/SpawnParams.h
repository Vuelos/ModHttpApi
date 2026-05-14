#pragma once

#include <string>
#include <cstdint>

struct SpawnParams
{
    uint32_t    entry       = 0;
    uint32_t    mapId       = 0;
    float       x           = 0.f;
    float       y           = 0.f;
    float       z           = 0.f;
    float       orientation = 0.f;
    uint32_t    despawnSecs = 0;
    std::string name;
};
