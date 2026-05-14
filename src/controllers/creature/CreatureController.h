#pragma once

#include "common/CommandResult.h"
#include "models/SpawnParams.h"
#include <cstdint>

namespace CreatureController
{
    CommandResult SpawnCreature(SpawnParams const& params);
    CommandResult DespawnCreature(uint64_t rawGuid);
}
