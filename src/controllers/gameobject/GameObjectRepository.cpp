#include "GameObjectRepository.h"
#include "libs/json.hpp"

#include "MapMgr.h"
#include "ObjectMgr.h"
#include "GameObject.h"
#include "Map.h"
#include <G3D/Quat.h>
#include <cmath>

using json = nlohmann::json;

namespace GameObjectRepository
{
    json SpawnGameObject(uint32_t entry, uint32_t mapId, float x, float y, float z, float orientation, uint32_t despawnSecs)
    {
        GameObjectTemplate const* goInfo = sObjectMgr->GetGameObjectTemplate(entry);
        if (!goInfo)
            return json();

        Map* map = sMapMgr->CreateBaseMap(mapId);
        if (!map)
            return json();

        float spawnZ = z;
        if (spawnZ == 0.f)
            spawnZ = map->GetHeight(x, y, MAX_HEIGHT);

        GameObject* go = new GameObject();
        ObjectGuid::LowType guidLow = map->GenerateLowGuid<HighGuid::GameObject>();

        float half = orientation * 0.5f;
        G3D::Quat rotation(0.f, 0.f, std::sin(half), std::cos(half));

        if (!go->Create(guidLow, entry, map, PHASEMASK_NORMAL, x, y, spawnZ, orientation, rotation, 100, GO_STATE_READY))
        {
            delete go;
            return json();
        }

        if (despawnSecs > 0)
            go->DespawnOrUnsummon(Milliseconds(despawnSecs * IN_MILLISECONDS));

        map->AddToMap(go);

        return json{
            {"guid", go->GetGUID().ToString()},
            {"guid_raw", go->GetGUID().GetRawValue()},
            {"entry", entry},
            {"mapId", mapId},
            {"x", x},
            {"y", y},
            {"z", spawnZ}
        };
    }

    bool DespawnGameObject(uint64_t rawGuid)
    {
        ObjectGuid objGuid(rawGuid);

        bool found = false;
        sMapMgr->DoForAllMaps([&](Map* map)
        {
            if (found) return;
            if (GameObject* go = map->GetGameObject(objGuid))
            {
                go->DespawnOrUnsummon();
                found = true;
            }
        });

        return found;
    }
}
