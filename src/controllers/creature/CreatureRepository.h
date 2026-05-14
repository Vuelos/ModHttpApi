#pragma once

#include "libs/json_fwd.hpp"
#include <cstdint>

namespace CreatureRepository
{
    nlohmann::json SpawnCreature(uint32_t entry, uint32_t mapId, float x, float y, float z, float orientation, uint32_t despawnSecs);
    bool DespawnCreature(uint64_t rawGuid);
}
