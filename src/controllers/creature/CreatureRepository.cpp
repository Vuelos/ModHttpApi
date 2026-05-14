#include "CreatureRepository.h"
#include "libs/json.hpp"

#include "MapMgr.h"
#include "ObjectMgr.h"
#include "Creature.h"
#include "CreatureData.h"
#include "Map.h"

using json = nlohmann::json;

namespace CreatureRepository
{
    json SpawnCreature(uint32_t entry, uint32_t mapId, float x, float y, float z, float orientation, uint32_t despawnSecs)
    {
        CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(entry);
        if (!creatureTemplate)
            return json();

        Map* map = sMapMgr->CreateBaseMap(mapId);
        if (!map)
            return json();

        float spawnZ = z;
        if (spawnZ == 0.f)
            spawnZ = map->GetHeight(x, y, MAX_HEIGHT);

        Creature* creature = new Creature();
        if (!creature->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, PHASEMASK_NORMAL, entry, 0, x, y, spawnZ, orientation))
        {
            delete creature;
            return json();
        }

        creature->SetHomePosition(x, y, spawnZ, orientation);

        if (despawnSecs > 0)
            creature->DespawnOrUnsummon(Milliseconds(despawnSecs * IN_MILLISECONDS));

        map->AddToMap(creature);

        return json{
            {"guid", creature->GetGUID().ToString()},
            {"guid_raw", creature->GetGUID().GetRawValue()},
            {"entry", entry},
            {"mapId", mapId},
            {"x", x},
            {"y", y},
            {"z", spawnZ}
        };
    }

    bool DespawnCreature(uint64_t rawGuid)
    {
        ObjectGuid objGuid(rawGuid);

        bool found = false;
        sMapMgr->DoForAllMaps([&](Map* map)
        {
            if (found) return;
            if (Creature* creature = map->GetCreature(objGuid))
            {
                creature->DespawnOrUnsummon();
                found = true;
            }
        });

        return found;
    }
}
