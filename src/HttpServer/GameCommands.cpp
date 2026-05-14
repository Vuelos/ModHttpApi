#include "GameCommands.h"

#include "MapMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSessionMgr.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "DBCStores.h"
#include "Object.h"
#include "Chat.h"              // sWorld->SendWorldText / ChatHandler
#include "CreatureData.h"
#include "ObjectMgr.h"         // sObjectMgr->GetCreatureTemplate
#include "MotionMaster.h"

#include <sstream>
#include <iomanip>
#include <cmath>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

std::string GameCommands::JsonEscape(std::string const& s)
{
    std::ostringstream o;
    for (char c : s)
    {
        switch (c)
        {
            case '"':  o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b";  break;
            case '\f': o << "\\f";  break;
            case '\n': o << "\\n";  break;
            case '\r': o << "\\r";  break;
            case '\t': o << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                else
                    o << c;
        }
    }
    return o.str();
}

std::string GameCommands::BuildPlayerJson(Player const* player)
{
    Powers powerType = player->getPowerType();
    std::ostringstream json;
    json << std::setprecision(6) << std::fixed;
    json << "{"
        << "\"name\":\""        << JsonEscape(player->GetName())          << "\","
        << "\"guid\":\""        << player->GetGUID().ToString()            << "\","
        << "\"mapId\":"         << player->GetMapId()                      << ","
        << "\"x\":"             << player->GetPositionX()                  << ","
        << "\"y\":"             << player->GetPositionY()                  << ","
        << "\"z\":"             << player->GetPositionZ()                  << ","
        << "\"orientation\":"   << player->GetOrientation()                << ","
        << "\"level\":"         << static_cast<int>(player->GetLevel())    << ","
        << "\"race\":"          << static_cast<int>(player->getRace())     << ","
        << "\"class\":"         << static_cast<int>(player->getClass())    << ","
        << "\"gender\":"        << static_cast<int>(player->getGender())   << ","
        << "\"health\":"        << player->GetHealth()                     << ","
        << "\"maxHealth\":"     << player->GetMaxHealth()                  << ","
        << "\"powerType\":"     << static_cast<int>(powerType)             << ","
        << "\"power\":"         << player->GetPower(powerType)             << ","
        << "\"maxPower\":"      << player->GetMaxPower(powerType)          << ","
        << "\"zoneId\":"        << player->GetZoneId()                     << ","
        << "\"areaId\":"        << player->GetAreaId()                     << ","
        << "\"isGM\":"          << (player->IsGameMaster() ? "true" : "false") << ","
        << "\"guildId\":"       << player->GetGuildId()
        << "}";
    return json.str();
}

// ---------------------------------------------------------------------------
// World / server info
// ---------------------------------------------------------------------------

CommandResult GameCommands::GetServerInfo()
{
    std::ostringstream json;
    json << "{"
        << "\"uptime\":"            << GameTime::GetUptime().count()                        << ","
        << "\"gameTime\":"          << GameTime::GetGameTime().count()                      << ","
        << "\"startTime\":"         << GameTime::GetStartTime().count()                     << ","
        << "\"activeSessions\":"    << sWorldSessionMgr->GetActiveSessionCount()            << ","
        << "\"queuedSessions\":"    << sWorldSessionMgr->GetQueuedSessionCount()            << ","
        << "\"playerCount\":"       << sWorldSessionMgr->GetPlayerCount()                   << ","
        << "\"maxPlayerCount\":"    << sWorldSessionMgr->GetMaxPlayerCount()                << ","
        << "\"maxActiveSessions\":" << sWorldSessionMgr->GetMaxActiveSessionCount()         << ","
        << "\"maxQueuedSessions\":" << sWorldSessionMgr->GetMaxQueuedSessionCount()         << ","
        << "\"playerLimit\":"       << sWorldSessionMgr->GetPlayerAmountLimit()
        << "}";
    return CommandResult::Ok(json.str());
}

CommandResult GameCommands::GetTime()
{
    std::ostringstream json;
    json << "{"
        << "\"uptime\":"    << GameTime::GetUptime().count()    << ","
        << "\"gameTime\":"  << GameTime::GetGameTime().count()  << ","
        << "\"startTime\":" << GameTime::GetStartTime().count()
        << "}";
    return CommandResult::Ok(json.str());
}

// ---------------------------------------------------------------------------
// Player queries
// ---------------------------------------------------------------------------

CommandResult GameCommands::GetPlayer(std::string const& name)
{
    if (name.empty())
        return CommandResult::Fail(400, "missing 'name' parameter");

    Player* player = ObjectAccessor::FindPlayerByName(name, true);
    if (!player)
        return CommandResult::Fail(404, "player not found");

    return CommandResult::Ok(BuildPlayerJson(player));
}

CommandResult GameCommands::GetPlayers(
    std::optional<uint32_t> mapId,
    std::optional<float>    x,
    std::optional<float>    y,
    std::optional<float>    range)
{
    bool hasFilter = mapId.has_value() && x.has_value() && y.has_value();
    float filterRange = range.value_or(0.f);

    std::ostringstream json;
    json << "[";
    bool first = true;

    sWorldSessionMgr->DoForAllOnlinePlayers([&](Player* player)
    {
        if (hasFilter)
        {
            if (player->GetMapId() != *mapId)
                return;

            if (filterRange > 0.f)
            {
                float dx = player->GetPositionX() - *x;
                float dy = player->GetPositionY() - *y;
                if (std::sqrt(dx * dx + dy * dy) > filterRange)
                    return;
            }
        }

        if (!first) json << ",";
        first = false;
        json << BuildPlayerJson(player);
    });

    json << "]";
    return CommandResult::Ok(json.str());
}

// ---------------------------------------------------------------------------
// World queries
// ---------------------------------------------------------------------------

CommandResult GameCommands::GetGroundZ(uint32_t mapId, float x, float y)
{
    Map* map = sMapMgr->CreateBaseMap(mapId);
    if (!map)
        return CommandResult::Fail(404, "invalid map");

    float z = map->GetHeight(x, y, MAX_HEIGHT);
    std::ostringstream json;
    json << std::setprecision(6) << std::fixed;
    json << "{\"z\":" << z << "}";
    return CommandResult::Ok(json.str());
}

CommandResult GameCommands::GetZone(uint32_t mapId, float x, float y, float z)
{
    uint32_t zoneId = 0, areaId = 0;
    sMapMgr->GetZoneAndAreaId(1, zoneId, areaId, mapId, x, y, z);

    std::string zoneName, areaName;
    if (AreaTableEntry const* zoneEntry = sAreaTableStore.LookupEntry(zoneId))
        zoneName = zoneEntry->area_name[0] ? zoneEntry->area_name[0] : "";
    if (AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(areaId))
        areaName = areaEntry->area_name[0] ? areaEntry->area_name[0] : "";

    std::ostringstream json;
    json << "{"
        << "\"zoneId\":"     << zoneId                          << ","
        << "\"zoneName\":\"" << JsonEscape(zoneName)            << "\","
        << "\"areaId\":"     << areaId                          << ","
        << "\"areaName\":\"" << JsonEscape(areaName)            << "\""
        << "}";
    return CommandResult::Ok(json.str());
}

// ---------------------------------------------------------------------------
// Broadcast / chat
// ---------------------------------------------------------------------------

CommandResult GameCommands::Broadcast(BroadcastParams const& params)
{
    if (params.message.empty())
        return CommandResult::Fail(400, "missing 'message' field");

    std::string const& type = params.type;

    if (type == "system" || type.empty())
    {
        // Blue server notification visible to all online players
        sWorld->SendWorldText(LANG_SYSTEMMESSAGE, params.message.c_str());
    }
    else if (type == "say" || type == "yell")
    {
        // Positional SAY / YELL requires a map and coordinates
        if (!params.mapId || !params.x || !params.y || !params.z)
            return CommandResult::Fail(400, "'say' and 'yell' types require mapId, x, y, z");

        Map* map = sMapMgr->FindBaseNonInstanceMap(*params.mapId);
        if (!map)
            return CommandResult::Fail(404, "map not found or not loaded");

        // Build a transient world object acting as the "speaker"
        WorldObject* announcer = nullptr; // TODO: use a temp object or route via GM session

        // Fallback: send as system message with a prefix tag
        std::string prefixed = "[" + (type == "yell" ? "YELL" : "SAY") + "] " + params.message;
        sWorld->SendWorldText(LANG_SYSTEMMESSAGE, prefixed.c_str());
    }
    else
    {
        return CommandResult::Fail(400, "unknown type; use 'system', 'say', or 'yell'");
    }

    return CommandResult::Ok("{\"sent\":true}");
}

// ---------------------------------------------------------------------------
// Creature / NPC spawning
// ---------------------------------------------------------------------------

CommandResult GameCommands::SpawnCreature(SpawnParams const& params)
{
    if (params.entry == 0)
        return CommandResult::Fail(400, "missing or invalid 'entry'");

    CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(params.entry);
    if (!creatureTemplate)
        return CommandResult::Fail(404, "creature template not found");

    Map* map = sMapMgr->CreateBaseMap(params.mapId);
    if (!map)
        return CommandResult::Fail(404, "invalid map");

    // Resolve ground height if z is not provided (z == 0 is the sentinel)
    float spawnZ = params.z;
    if (spawnZ == 0.f)
        spawnZ = map->GetHeight(params.x, params.y, MAX_HEIGHT);

    // Allocate a new GUID for the creature
    ObjectGuid::LowType guid = sObjectMgr->GetGenerator<HighGuid::Creature>().Generate();

    Creature* creature = new Creature();
    if (!creature->Create(guid, map, PHASEMASK_NORMAL, params.entry,
                          params.x, params.y, spawnZ, params.orientation))
    {
        delete creature;
        return CommandResult::Fail(500, "failed to create creature");
    }

    creature->SetHomePosition(params.x, params.y, spawnZ, params.orientation);

    if (params.despawnSecs > 0)
        creature->ForcedDespawn(params.despawnSecs * IN_MILLISECONDS);

    map->AddToMap(creature);

    std::ostringstream json;
    json << "{"
        << "\"guid\":\"" << creature->GetGUID().ToString() << "\","
        << "\"entry\":"  << params.entry                   << ","
        << "\"mapId\":"  << params.mapId                   << ","
        << "\"x\":"      << std::fixed << std::setprecision(4) << params.x << ","
        << "\"y\":"      << params.y                       << ","
        << "\"z\":"      << spawnZ
        << "}";
    return CommandResult::Ok(json.str());
}

CommandResult GameCommands::DespawnCreature(uint64_t guid)
{
    // NOTE: This searches all maps – may be expensive on large servers.
    // A production implementation should accept mapId to narrow the search.
    ObjectGuid objGuid = ObjectGuid::Create<HighGuid::Creature>(guid);

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

    if (!found)
        return CommandResult::Fail(404, "creature not found");

    return CommandResult::Ok("{\"despawned\":true}");
}

// ---------------------------------------------------------------------------
// Player management
// ---------------------------------------------------------------------------

CommandResult GameCommands::TeleportPlayer(TeleportParams const& params)
{
    Player* player = ObjectAccessor::FindPlayerByName(params.playerName, true);
    if (!player)
        return CommandResult::Fail(404, "player not found or offline");

    if (!MapMgr::IsValidMapCoord(params.mapId, params.x, params.y, params.z, params.orientation))
        return CommandResult::Fail(400, "invalid map coordinates");

    player->TeleportTo(params.mapId, params.x, params.y, params.z, params.orientation);

    return CommandResult::Ok("{\"teleported\":true}");
}

CommandResult GameCommands::ModifyItem(ItemParams const& params)
{
    Player* player = ObjectAccessor::FindPlayerByName(params.playerName, true);
    if (!player)
        return CommandResult::Fail(404, "player not found or offline");

    if (params.count > 0)
    {
        // Give items
        ItemPosCountVec dest;
        InventoryResult result = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, params.itemEntry, params.count);
        if (result != EQUIP_ERR_OK)
            return CommandResult::Fail(409, "cannot store item: inventory full or invalid entry");

        Item* item = player->StoreNewItem(dest, params.itemEntry, true);
        if (!item)
            return CommandResult::Fail(500, "failed to create item");

        player->SendNewItem(item, params.count, true, false);
    }
    else if (params.count < 0)
    {
        // Remove items
        uint32_t removeCount = static_cast<uint32_t>(-params.count);
        if (!player->HasItemCount(params.itemEntry, removeCount))
            return CommandResult::Fail(409, "player does not have enough of that item");

        player->DestroyItemCount(params.itemEntry, removeCount, true);
    }
    else
    {
        return CommandResult::Fail(400, "'count' must not be zero");
    }

    std::ostringstream json;
    json << "{"
        << "\"player\":\"" << JsonEscape(params.playerName) << "\","
        << "\"itemEntry\":" << params.itemEntry             << ","
        << "\"count\":"     << params.count
        << "}";
    return CommandResult::Ok(json.str());
}

CommandResult GameCommands::SetVital(SetVitalParams const& params)
{
    Player* player = ObjectAccessor::FindPlayerByName(params.playerName, true);
    if (!player)
        return CommandResult::Fail(404, "player not found or offline");

    std::string const& vt = params.vitalType;

    if (vt == "health")
    {
        uint32_t val = (params.value < 0) ? player->GetMaxHealth()
                                           : static_cast<uint32_t>(params.value);
        player->SetHealth(std::min(val, player->GetMaxHealth()));
    }
    else
    {
        // Map string → Powers enum
        static const std::unordered_map<std::string, Powers> powerMap =
        {
            { "mana",        POWER_MANA        },
            { "rage",        POWER_RAGE        },
            { "focus",       POWER_FOCUS       },
            { "energy",      POWER_ENERGY      },
            { "runic_power", POWER_RUNIC_POWER },
        };

        auto it = powerMap.find(vt);
        if (it == powerMap.end())
            return CommandResult::Fail(400, "unknown vitalType; use health/mana/rage/energy/focus/runic_power");

        Powers power = it->second;
        uint32_t val = (params.value < 0) ? player->GetMaxPower(power)
                                           : static_cast<uint32_t>(params.value);
        player->SetPower(power, std::min(val, player->GetMaxPower(power)));
    }

    return CommandResult::Ok("{\"set\":true}");
}

CommandResult GameCommands::KickPlayer(std::string const& name, std::string const& reason)
{
    Player* player = ObjectAccessor::FindPlayerByName(name, true);
    if (!player)
        return CommandResult::Fail(404, "player not found or offline");

    player->GetSession()->KickPlayer(reason.empty() ? "Kicked via API" : reason);

    return CommandResult::Ok("{\"kicked\":true}");
}