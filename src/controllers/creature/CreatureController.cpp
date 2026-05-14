#include "CreatureController.h"
#include "CreatureRepository.h"
#include "libs/json.hpp"

using json = nlohmann::json;

namespace CreatureController
{
    CommandResult SpawnCreature(SpawnParams const& params)
    {
        if (params.entry == 0)
            return CommandResult::Fail(400, "missing or invalid 'entry'");

        json result = CreatureRepository::SpawnCreature(params.entry, params.mapId, params.x, params.y, params.z, params.orientation, params.despawnSecs);
        if (result.empty())
            return CommandResult::Fail(500, "failed to spawn creature");

        return CommandResult::Ok(result.dump());
    }

    CommandResult DespawnCreature(uint64_t rawGuid)
    {
        if (!CreatureRepository::DespawnCreature(rawGuid))
            return CommandResult::Fail(404, "creature not found");

        return CommandResult::Ok(json{{"despawned", true}}.dump());
    }
}
