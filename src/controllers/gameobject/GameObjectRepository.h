#pragma once

#include "libs/json_fwd.hpp"
#include <cstdint>

namespace GameObjectRepository
{
    nlohmann::json SpawnGameObject(uint32_t entry, uint32_t mapId, float x, float y, float z, float orientation, uint32_t despawnSecs);
    bool DespawnGameObject(uint64_t rawGuid);
}
