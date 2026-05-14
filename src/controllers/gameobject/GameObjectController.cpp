#include "GameObjectController.h"
#include "GameObjectRepository.h"
#include "libs/json.hpp"

using json = nlohmann::json;

namespace GameObjectController
{
    CommandResult SpawnGameObject(SpawnParams const& params)
    {
        if (params.entry == 0)
            return CommandResult::Fail(400, "missing or invalid 'entry'");

        json result = GameObjectRepository::SpawnGameObject(params.entry, params.mapId, params.x, params.y, params.z, params.orientation, params.despawnSecs);
        if (result.empty())
            return CommandResult::Fail(500, "failed to spawn gameobject");

        return CommandResult::Ok(result.dump());
    }

    CommandResult DespawnGameObject(uint64_t rawGuid)
    {
        if (!GameObjectRepository::DespawnGameObject(rawGuid))
            return CommandResult::Fail(404, "gameobject not found");

        return CommandResult::Ok(json{{"despawned", true}}.dump());
    }
}
