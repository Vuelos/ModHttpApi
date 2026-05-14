#pragma once

#include "libs/json_fwd.hpp"
#include <cstdint>
#include <string>

struct ZoneInfo
{
    uint32_t zoneId;
    uint32_t areaId;
    std::string zoneName;
    std::string areaName;
};

namespace ServerRepository
{
    nlohmann::json GetServerData();
    nlohmann::json GetTimeData();
    float GetGroundHeight(uint32_t mapId, float x, float y);
    ZoneInfo GetZoneData(uint32_t mapId, float x, float y, float z);
}
