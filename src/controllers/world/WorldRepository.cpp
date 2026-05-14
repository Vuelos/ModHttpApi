#include "WorldRepository.h"
#include "libs/json.hpp"

#include "DBCStores.h"
#include "MapMgr.h"
#include "Map.h"
#include "GridDefines.h"
#include "Grids/Cells/Cell.h"
#include "Creature.h"
#include "GameObject.h"

using json = nlohmann::json;

namespace
{
    class NearbyCollector
    {
    public:
        NearbyCollector(float x, float y, float range) : m_x(x), m_y(y), m_rangeSq(range* range) {}

        void Visit(CreatureMapType& m)
        {
            for (auto itr = m.begin(); itr != m.end(); ++itr)
            {
                Creature* c = itr->GetSource();
                float dx = c->GetPositionX() - m_x;
                float dy = c->GetPositionY() - m_y;
                if (dx * dx + dy * dy <= m_rangeSq)
                    creatures.push_back({
                        {"guid", c->GetGUID().ToString()},
                        {"entry", c->GetEntry()},
                        {"name", c->GetName()},
                        {"x", c->GetPositionX()},
                        {"y", c->GetPositionY()}
                    });
            }
        }

        void Visit(GameObjectMapType& m)
        {
            for (auto itr = m.begin(); itr != m.end(); ++itr)
            {
                GameObject* go = itr->GetSource();
                float dx = go->GetPositionX() - m_x;
                float dy = go->GetPositionY() - m_y;
                if (dx * dx + dy * dy <= m_rangeSq)
                    gameobjects.push_back({
                        {"guid", go->GetGUID().ToString()},
                        {"entry", go->GetEntry()},
                        {"name", go->GetGOInfo() ? go->GetGOInfo()->name : ""},
                        {"x", go->GetPositionX()},
                        {"y", go->GetPositionY()}
                    });
            }
        }

        template<class NOT_INTERESTED>
        void Visit(GridRefMgr<NOT_INTERESTED>&) {}

        json creatures = json::array();
        json gameobjects = json::array();

    private:
        float m_x, m_y, m_rangeSq;
    };
}

namespace WorldRepository
{
    json GetAllMaps()
    {
        json maps = json::array();

        for (auto const& mapEntry : sMapStore)
        {
            maps.push_back({
                {"id", mapEntry->MapID},
                {"name", mapEntry->name[0] ? mapEntry->name[0] : ""},
                {"type", mapEntry->map_type},
                {"isDungeon", mapEntry->IsDungeon()},
                {"isRaid", mapEntry->IsRaid()},
                {"isBattleground", mapEntry->IsBattleground()},
                {"isArena", mapEntry->IsBattleArena()},
                {"instanceable", mapEntry->Instanceable()},
                {"maxPlayers", mapEntry->maxPlayers},
                {"expansion", mapEntry->expansionID}
            });
        }

        return maps;
    }

    json GetNearbyObjects(uint32_t mapId, float x, float y, float range)
    {
        Map* map = sMapMgr->FindBaseNonInstanceMap(mapId);
        if (!map)
            return json();

        NearbyCollector collector(x, y, range);
        Cell::VisitObjects(x, y, map, collector, range);

        return json{
            {"creatures", collector.creatures},
            {"gameobjects", collector.gameobjects},
            {"total", collector.creatures.size() + collector.gameobjects.size()}
        };
    }

    bool SetZoneWeather(uint32_t zoneId, uint32_t weatherType, float intensity)
    {
        sMapMgr->DoForAllMaps([&](Map* map)
        {
            map->SetZoneWeather(zoneId, static_cast<WeatherState>(weatherType), intensity);
        });

        return true;
    }
}
