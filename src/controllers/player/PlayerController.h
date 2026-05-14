#pragma once

#include "common/CommandResult.h"
#include <string>
#include <optional>
#include <cstdint>

namespace PlayerController
{
    CommandResult GetPlayer(std::string const& name);
    CommandResult GetPlayers(std::optional<uint32_t> mapId, std::optional<float> x, std::optional<float> y, std::optional<float> range);
    CommandResult GetPlayerStats(std::string const& name);
    CommandResult GetPlayerSkills(std::string const& name);
    CommandResult GetPlayerQuests(std::string const& name);
    CommandResult GetPlayerEquipment(std::string const& name);
    CommandResult GetPlayerInventory(std::string const& name);
    CommandResult TeleportPlayer(std::string const& name, uint32_t mapId, float x, float y, float z, float orientation);
    CommandResult SetVital(std::string const& name, std::string const& vitalType, int32_t value);
    CommandResult KickPlayer(std::string const& name, std::string const& reason);
    CommandResult ToggleGM(std::string const& name, bool gmOn);
    CommandResult ResetCooldowns(std::string const& name);
    CommandResult MorphPlayer(std::string const& name, uint32_t displayId);
    CommandResult CompleteQuest(std::string const& name, uint32_t questId);
}
