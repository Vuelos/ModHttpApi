#include "PlayerController.h"
#include "PlayerRepository.h"
#include "libs/json.hpp"

#include "Player.h"
#include "ObjectAccessor.h"
#include "MapMgr.h"
#include "WorldSessionMgr.h"
#include <cmath>
#include <unordered_map>

using json = nlohmann::json;

namespace
{
    json BuildPlayerSimple(Player const* player)
    {
        Powers powerType = player->getPowerType();
        return json{
            {"name", player->GetName()},
            {"guid", player->GetGUID().ToString()},
            {"mapId", player->GetMapId()},
            {"x", player->GetPositionX()},
            {"y", player->GetPositionY()},
            {"z", player->GetPositionZ()},
            {"orientation", player->GetOrientation()},
            {"level", static_cast<int>(player->GetLevel())},
            {"race", static_cast<int>(player->getRace())},
            {"class", static_cast<int>(player->getClass())},
            {"gender", static_cast<int>(player->getGender())},
            {"health", player->GetHealth()},
            {"maxHealth", player->GetMaxHealth()},
            {"powerType", static_cast<int>(powerType)},
            {"power", player->GetPower(powerType)},
            {"maxPower", player->GetMaxPower(powerType)},
            {"zoneId", player->GetZoneId()},
            {"areaId", player->GetAreaId()},
            {"isGM", player->IsGameMaster()},
            {"guildId", player->GetGuildId()}
        };
    }
}

namespace PlayerController
{
    CommandResult GetPlayer(std::string const& name)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found");

        return CommandResult::Ok(BuildPlayerSimple(player).dump());
    }

    CommandResult GetPlayers(std::optional<uint32_t> mapId, std::optional<float> x, std::optional<float> y, std::optional<float> range)
    {
        bool hasFilter = mapId.has_value() && x.has_value() && y.has_value();
        float filterRange = range.value_or(0.f);

        json players = json::array();

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
            players.push_back(BuildPlayerSimple(player));
        });

        return CommandResult::Ok(players.dump());
    }

    CommandResult GetPlayerStats(std::string const& name)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found");

        return CommandResult::Ok(PlayerRepository::GetPlayerStats(player).dump());
    }

    CommandResult GetPlayerSkills(std::string const& name)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found");

        return CommandResult::Ok(PlayerRepository::GetPlayerSkillsFull(player).dump());
    }

    CommandResult GetPlayerQuests(std::string const& name)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found");

        return CommandResult::Ok(PlayerRepository::GetPlayerQuests(player).dump());
    }

    CommandResult GetPlayerEquipment(std::string const& name)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found");

        return CommandResult::Ok(PlayerRepository::GetPlayerEquipment(player).dump());
    }

    CommandResult TeleportPlayer(std::string const& name, uint32_t mapId, float x, float y, float z, float orientation)
    {
        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found or offline");

        if (!MapMgr::IsValidMapCoord(mapId, x, y, z, orientation))
            return CommandResult::Fail(400, "invalid map coordinates");

        player->TeleportTo(mapId, x, y, z, orientation);

        return CommandResult::Ok(json{{"teleported", true}}.dump());
    }

    CommandResult SetVital(std::string const& name, std::string const& vitalType, int32_t value)
    {
        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found or offline");

        if (vitalType == "health")
        {
            uint32_t val = (value < 0) ? player->GetMaxHealth() : static_cast<uint32_t>(value);
            player->SetHealth(std::min(val, player->GetMaxHealth()));
        }
        else
        {
            static std::unordered_map<std::string, Powers> const powerMap = {
                {"mana", POWER_MANA}, {"rage", POWER_RAGE}, {"focus", POWER_FOCUS},
                {"energy", POWER_ENERGY}, {"runic_power", POWER_RUNIC_POWER}
            };

            auto it = powerMap.find(vitalType);
            if (it == powerMap.end())
                return CommandResult::Fail(400, "unknown vitalType; use health/mana/rage/energy/focus/runic_power");

            Powers power = it->second;
            uint32_t val = (value < 0) ? player->GetMaxPower(power) : static_cast<uint32_t>(value);
            player->SetPower(power, std::min(val, player->GetMaxPower(power)));
        }

        return CommandResult::Ok(json{{"set", true}}.dump());
    }

    CommandResult KickPlayer(std::string const& name, std::string const& reason)
    {
        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found or offline");

        player->GetSession()->KickPlayer(reason.empty() ? "Kicked via API" : reason);

        return CommandResult::Ok(json{{"kicked", true}}.dump());
    }

    CommandResult GetPlayerInventory(std::string const& name)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found");

        return CommandResult::Ok(PlayerRepository::GetPlayerInventory(player).dump());
    }

    CommandResult ToggleGM(std::string const& name, bool gmOn)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found or offline");

        PlayerRepository::SetGameMaster(player, gmOn);
        return CommandResult::Ok(json{{"gm", gmOn}, {"name", name}}.dump());
    }

    CommandResult ResetCooldowns(std::string const& name)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found or offline");

        PlayerRepository::ResetCooldowns(player);
        return CommandResult::Ok(json{{"cooldownsReset", true}, {"name", name}}.dump());
    }

    CommandResult MorphPlayer(std::string const& name, uint32_t displayId)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found or offline");

        PlayerRepository::SetDisplayId(player, displayId);
        return CommandResult::Ok(json{{"morphed", true}, {"name", name}, {"displayId", displayId}}.dump());
    }

    CommandResult CompleteQuest(std::string const& name, uint32_t questId)
    {
        if (name.empty())
            return CommandResult::Fail(400, "missing 'name' parameter");
        if (questId == 0)
            return CommandResult::Fail(400, "missing or invalid 'questId'");

        Player* player = PlayerRepository::FindPlayerByName(name);
        if (!player)
            return CommandResult::Fail(404, "player not found or offline");

        if (!PlayerRepository::CompleteQuest(player, questId))
            return CommandResult::Fail(500, "failed to complete quest");

        return CommandResult::Ok(json{{"completed", true}, {"questId", questId}, {"name", name}}.dump());
    }
}
