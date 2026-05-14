#pragma once

#include "libs/json_fwd.hpp"
#include <cstdint>

namespace WorldRepository
{
    nlohmann::json GetAllMaps();
    nlohmann::json GetNearbyObjects(uint32_t mapId, float x, float y, float range);
    bool SetZoneWeather(uint32_t zoneId, uint32_t weatherType, float intensity);
}
