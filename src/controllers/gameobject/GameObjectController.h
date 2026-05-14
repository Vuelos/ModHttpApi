#pragma once

#include "common/CommandResult.h"
#include "models/SpawnParams.h"
#include <cstdint>

namespace GameObjectController
{
    CommandResult SpawnGameObject(SpawnParams const& params);
    CommandResult DespawnGameObject(uint64_t rawGuid);
}
