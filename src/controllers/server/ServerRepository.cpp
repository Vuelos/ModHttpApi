#include "ServerRepository.h"
#include "libs/json.hpp"

#include "MapMgr.h"
#include "WorldSessionMgr.h"
#include "GameTime.h"
#include "DBCStores.h"
#include <fmt/format.h>

using json = nlohmann::json;

namespace ServerRepository
{
    json GetServerData()
    {
        Seconds uptime = GameTime::GetUptime();
        Seconds currentTime = GameTime::GetGameTime();
        Seconds startTime = GameTime::GetStartTime();

        uint32 uptimeSeconds = static_cast<uint32>(uptime.count());
        uint32 days = uptimeSeconds / 86400;
        uint32 hours = (uptimeSeconds % 86400) / 3600;
        uint32 minutes = (uptimeSeconds % 3600) / 60;
        uint32 seconds = uptimeSeconds % 60;

        return json{
            {"uptime_seconds", uptime.count()},
            {"current_time", currentTime.count()},
            {"start_time", startTime.count()},
            {"player_count", sWorldSessionMgr->GetPlayerCount()},
            {"max_player_count", sWorldSessionMgr->GetMaxPlayerCount()},
            {"active_sessions", sWorldSessionMgr->GetActiveSessionCount()},
            {"queued_sessions", sWorldSessionMgr->GetQueuedSessionCount()},
            {"total_sessions", sWorldSessionMgr->GetActiveAndQueuedSessionCount()},
            {"uptime_formatted", fmt::format("{}d {}h {}m {}s", days, hours, minutes, seconds)}
        };
    }

    json GetTimeData()
    {
        return json{
            {"uptime", GameTime::GetUptime().count()},
            {"gameTime", GameTime::GetGameTime().count()},
            {"startTime", GameTime::GetStartTime().count()}
        };
    }

    float GetGroundHeight(uint32_t mapId, float x, float y)
    {
        Map* map = sMapMgr->CreateBaseMap(mapId);
        if (!map)
            return 0.0f;
        return map->GetHeight(x, y, MAX_HEIGHT);
    }

    ZoneInfo GetZoneData(uint32_t mapId, float x, float y, float z)
    {
        ZoneInfo info = {};
        sMapMgr->GetZoneAndAreaId(1, info.zoneId, info.areaId, mapId, x, y, z);

        if (AreaTableEntry const* zoneEntry = sAreaTableStore.LookupEntry(info.zoneId))
            info.zoneName = zoneEntry->area_name[0] ? zoneEntry->area_name[0] : "";
        if (AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(info.areaId))
            info.areaName = areaEntry->area_name[0] ? areaEntry->area_name[0] : "";

        return info;
    }
}
