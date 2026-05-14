#pragma once

#include "common/CommandResult.h"
#include <string>
#include <cstdint>
#include <optional>

namespace WorldController
{
    CommandResult GetMaps();
    CommandResult GetNearby(uint32_t mapId, float x, float y, std::optional<float> range);
    CommandResult SetWeather(uint32_t zoneId, uint32_t weatherType, float intensity);
}
