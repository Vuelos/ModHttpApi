#pragma once

#include "libs/json_fwd.hpp"
#include <string>

class Player;
class Item;

namespace PlayerRepository
{
    nlohmann::json GetItemData(Item* item);
    nlohmann::json GetPlayerEquipment(Player* player);
    nlohmann::json GetPlayerStats(Player* player);
    nlohmann::json GetPlayerTalentInfo(Player* player);
    nlohmann::json GetPlayerData(Player* player, bool includeEquipment = false);
    nlohmann::json GetAllPlayersData(bool includeEquipment = false);
    nlohmann::json GetPlayerSkills(Player* player);
    nlohmann::json GetPlayerSkillsFull(Player* player);
    nlohmann::json GetPlayerQuests(Player* player);
    nlohmann::json GetPlayerInventory(Player* player);
    Player* FindPlayerByName(std::string const& name);
    bool SetGameMaster(Player* player, bool on);
    void ResetCooldowns(Player* player);
    void SetDisplayId(Player* player, uint32_t displayId);
    bool CompleteQuest(Player* player, uint32_t questId);
}
