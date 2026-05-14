#include "ServerController.h"
#include "libs/json.hpp"

#include "MapMgr.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "WorldSessionMgr.h"

using json = nlohmann::json;

namespace ServerController
{
    CommandResult GetServerInfo()
    {
        json j = {
            {"uptime", GameTime::GetUptime().count()},
            {"gameTime", GameTime::GetGameTime().count()},
            {"startTime", GameTime::GetStartTime().count()},
            {"activeSessions", sWorldSessionMgr->GetActiveSessionCount()},
            {"queuedSessions", sWorldSessionMgr->GetQueuedSessionCount()},
            {"playerCount", sWorldSessionMgr->GetPlayerCount()},
            {"maxPlayerCount", sWorldSessionMgr->GetMaxPlayerCount()},
            {"maxActiveSessions", sWorldSessionMgr->GetMaxActiveSessionCount()},
            {"maxQueuedSessions", sWorldSessionMgr->GetMaxQueuedSessionCount()},
            {"playerLimit", sWorldSessionMgr->GetPlayerAmountLimit()}
        };
        return CommandResult::Ok(j.dump());
    }

    CommandResult GetTime()
    {
        json j = {
            {"uptime", GameTime::GetUptime().count()},
            {"gameTime", GameTime::GetGameTime().count()},
            {"startTime", GameTime::GetStartTime().count()}
        };
        return CommandResult::Ok(j.dump());
    }

    CommandResult GetGroundZ(uint32_t mapId, float x, float y)
    {
        Map* map = sMapMgr->CreateBaseMap(mapId);
        if (!map)
            return CommandResult::Fail(404, "invalid map");

        float z = map->GetHeight(x, y, MAX_HEIGHT);
        return CommandResult::Ok(json{{"z", z}}.dump());
    }

    CommandResult GetZone(uint32_t mapId, float x, float y, float z)
    {
        uint32_t zoneId = 0, areaId = 0;
        sMapMgr->GetZoneAndAreaId(1, zoneId, areaId, mapId, x, y, z);

        std::string zoneName, areaName;
        if (AreaTableEntry const* zoneEntry = sAreaTableStore.LookupEntry(zoneId))
            zoneName = zoneEntry->area_name[0] ? zoneEntry->area_name[0] : "";
        if (AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(areaId))
            areaName = areaEntry->area_name[0] ? areaEntry->area_name[0] : "";

        json j = {{"zoneId", zoneId}, {"zoneName", zoneName}, {"areaId", areaId}, {"areaName", areaName}};
        return CommandResult::Ok(j.dump());
    }
}
