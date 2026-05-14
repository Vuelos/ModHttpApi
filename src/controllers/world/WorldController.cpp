#include "WorldController.h"
#include "WorldRepository.h"
#include "libs/json.hpp"

using json = nlohmann::json;

namespace WorldController
{
    CommandResult GetMaps()
    {
        return CommandResult::Ok(WorldRepository::GetAllMaps().dump());
    }

    CommandResult GetNearby(uint32_t mapId, float x, float y, std::optional<float> range)
    {
        json result = WorldRepository::GetNearbyObjects(mapId, x, y, range.value_or(100.f));
        return CommandResult::Ok(result.dump());
    }

    CommandResult SetWeather(uint32_t zoneId, uint32_t weatherType, float intensity)
    {
        if (zoneId == 0)
            return CommandResult::Fail(400, "missing or invalid 'zoneId'");

        if (!WorldRepository::SetZoneWeather(zoneId, weatherType, intensity))
            return CommandResult::Fail(500, "failed to set weather");

        return CommandResult::Ok(json{{"weatherSet", true}, {"zoneId", zoneId}, {"weatherType", weatherType}, {"intensity", intensity}}.dump());
    }
}
