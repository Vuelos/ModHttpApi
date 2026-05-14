#include "PlayerRepository.h"
#include "libs/json.hpp"

#include <map>

#include "WorldSessionMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SharedDefines.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Bag.h"
#include "ObjectMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Group.h"
#include "QuestDef.h"
#include "DBCStores.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

using json = nlohmann::json;

namespace PlayerRepository
{
    json GetItemData(Item* item)
    {
        if (!item)
            return json::object();

        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (!itemTemplate)
            return json{
                {"entry", item->GetEntry()},
                {"count", item->GetCount()},
                {"durability", item->GetUInt32Value(ITEM_FIELD_DURABILITY)},
                {"max_durability", item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY)}
            };

        json data = {
            {"entry", item->GetEntry()},
            {"count", item->GetCount()},
            {"name", itemTemplate->Name1},
            {"quality", itemTemplate->Quality},
            {"item_level", itemTemplate->ItemLevel},
            {"required_level", itemTemplate->RequiredLevel},
            {"class", itemTemplate->Class},
            {"subclass", itemTemplate->SubClass},
            {"inventory_type", itemTemplate->InventoryType},
            {"durability", item->GetUInt32Value(ITEM_FIELD_DURABILITY)},
            {"max_durability", item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY)},
            {"resistances", {
                {"armor", itemTemplate->Armor},
                {"holy", itemTemplate->HolyRes},
                {"fire", itemTemplate->FireRes},
                {"nature", itemTemplate->NatureRes},
                {"frost", itemTemplate->FrostRes},
                {"shadow", itemTemplate->ShadowRes},
                {"arcane", itemTemplate->ArcaneRes}
            }},
            {"item_set", itemTemplate->ItemSet},
            {"bonding", itemTemplate->Bonding},
            {"stack_size", itemTemplate->GetMaxStackSize()},
            {"sell_price", itemTemplate->SellPrice},
            {"buy_price", itemTemplate->BuyPrice}
        };

        json stats = json::array();
        for (uint32 i = 0; i < itemTemplate->StatsCount && i < MAX_ITEM_PROTO_STATS; ++i)
            if (itemTemplate->ItemStat[i].ItemStatValue != 0)
                stats.push_back({{"type", itemTemplate->ItemStat[i].ItemStatType}, {"value", itemTemplate->ItemStat[i].ItemStatValue}});
        data["stats"] = stats;

        if (itemTemplate->Class == ITEM_CLASS_WEAPON)
        {
            json damages = json::array();
            for (uint32 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
                if (itemTemplate->Damage[i].DamageMin > 0 || itemTemplate->Damage[i].DamageMax > 0)
                    damages.push_back({{"min", itemTemplate->Damage[i].DamageMin}, {"max", itemTemplate->Damage[i].DamageMax}, {"type", itemTemplate->Damage[i].DamageType}});

            data["weapon_data"] = {{"delay", itemTemplate->Delay}, {"dps", itemTemplate->getDPS()}, {"damages", damages}};
        }

        json sockets = json::array();
        for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
            if (itemTemplate->Socket[i].Color)
                sockets.push_back({{"color", itemTemplate->Socket[i].Color}, {"content", itemTemplate->Socket[i].Content}});
        if (!sockets.empty())
        {
            data["sockets"] = sockets;
            if (itemTemplate->socketBonus)
                data["socket_bonus"] = itemTemplate->socketBonus;
        }

        json spells = json::array();
        for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            if (itemTemplate->Spells[i].SpellId > 0)
                spells.push_back({{"spell_id", itemTemplate->Spells[i].SpellId}, {"trigger", itemTemplate->Spells[i].SpellTrigger}, {"charges", itemTemplate->Spells[i].SpellCharges}, {"cooldown", itemTemplate->Spells[i].SpellCooldown}});
        if (!spells.empty())
            data["spells"] = spells;

        return data;
    }

    json GetPlayerEquipment(Player* player)
    {
        if (!player)
            return json::object();

        json equipment;
        std::map<uint8, std::string> const slotNames = {
            {EQUIPMENT_SLOT_HEAD, "head"}, {EQUIPMENT_SLOT_NECK, "neck"},
            {EQUIPMENT_SLOT_SHOULDERS, "shoulders"}, {EQUIPMENT_SLOT_BODY, "body"},
            {EQUIPMENT_SLOT_CHEST, "chest"}, {EQUIPMENT_SLOT_WAIST, "waist"},
            {EQUIPMENT_SLOT_LEGS, "legs"}, {EQUIPMENT_SLOT_FEET, "feet"},
            {EQUIPMENT_SLOT_WRISTS, "wrists"}, {EQUIPMENT_SLOT_HANDS, "hands"},
            {EQUIPMENT_SLOT_FINGER1, "finger1"}, {EQUIPMENT_SLOT_FINGER2, "finger2"},
            {EQUIPMENT_SLOT_TRINKET1, "trinket1"}, {EQUIPMENT_SLOT_TRINKET2, "trinket2"},
            {EQUIPMENT_SLOT_BACK, "back"}, {EQUIPMENT_SLOT_MAINHAND, "mainhand"},
            {EQUIPMENT_SLOT_OFFHAND, "offhand"}, {EQUIPMENT_SLOT_RANGED, "ranged"},
            {EQUIPMENT_SLOT_TABARD, "tabard"}
        };

        for (auto const& [slot, name] : slotNames)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                equipment[name] = GetItemData(item);
            else
                equipment[name] = nullptr;
        }

        return equipment;
    }

    json GetPlayerStats(Player* player)
    {
        if (!player)
            return json::object();

        return json{
            {"level", player->GetLevel()},
            {"experience", {{"current", player->GetUInt32Value(PLAYER_XP)}, {"next_level", player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP)}}},
            {"attributes", {{"strength", player->GetStat(STAT_STRENGTH)}, {"agility", player->GetStat(STAT_AGILITY)}, {"stamina", player->GetStat(STAT_STAMINA)}, {"intellect", player->GetStat(STAT_INTELLECT)}, {"spirit", player->GetStat(STAT_SPIRIT)}}},
            {"health", {{"current", player->GetHealth()}, {"max", player->GetMaxHealth()}}},
            {"power", {
                {"mana", {{"current", player->GetPower(POWER_MANA)}, {"max", player->GetMaxPower(POWER_MANA)}}},
                {"rage", {{"current", player->GetPower(POWER_RAGE)}, {"max", player->GetMaxPower(POWER_RAGE)}}},
                {"energy", {{"current", player->GetPower(POWER_ENERGY)}, {"max", player->GetMaxPower(POWER_ENERGY)}}},
                {"focus", {{"current", player->GetPower(POWER_FOCUS)}, {"max", player->GetMaxPower(POWER_FOCUS)}}},
                {"happiness", {{"current", player->GetPower(POWER_HAPPINESS)}, {"max", player->GetMaxPower(POWER_HAPPINESS)}}},
                {"runes", {{"current", player->GetPower(POWER_RUNE)}, {"max", player->GetMaxPower(POWER_RUNE)}}},
                {"runic_power", {{"current", player->GetPower(POWER_RUNIC_POWER)}, {"max", player->GetMaxPower(POWER_RUNIC_POWER)}}}
            }},
            {"combat", {
                {"attack_power", player->GetTotalAttackPowerValue(BASE_ATTACK)},
                {"ranged_attack_power", player->GetTotalAttackPowerValue(RANGED_ATTACK)},
                {"spell_power", player->GetBaseSpellPowerBonus()},
                {"critical_chance", {{"melee", player->GetFloatValue(PLAYER_CRIT_PERCENTAGE)}, {"ranged", player->GetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE)}, {"spell", player->GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1)}}},
                {"hit_chance", {{"melee", player->GetFloatValue(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE)}, {"spell", player->GetFloatValue(PLAYER_FIELD_MOD_TARGET_RESISTANCE)}}}
            }},
            {"resistances", {{"armor", player->GetArmor()}, {"holy", player->GetResistance(SPELL_SCHOOL_HOLY)}, {"fire", player->GetResistance(SPELL_SCHOOL_FIRE)}, {"nature", player->GetResistance(SPELL_SCHOOL_NATURE)}, {"frost", player->GetResistance(SPELL_SCHOOL_FROST)}, {"shadow", player->GetResistance(SPELL_SCHOOL_SHADOW)}, {"arcane", player->GetResistance(SPELL_SCHOOL_ARCANE)}}},
            {"status", {{"alive", player->IsAlive()}, {"in_combat", player->IsInCombat()}, {"resting", player->HasPlayerFlag(PLAYER_FLAGS_RESTING)}, {"ghost", player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST)}, {"player_vs_player", player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_PVP_TIMER)}, {"away", player->isAFK()}, {"dnd", player->isDND()}}},
            {"average_item_level", player->GetAverageItemLevel()}
        };
    }

    json GetPlayerTalentInfo(Player* player)
    {
        if (!player)
            return json::object();

        uint32 totalTalentPoints = player->CalculateTalentsPoints();
        uint32 freeTalentPoints = player->GetFreeTalentPoints();
        uint32 usedTalentPoints = (totalTalentPoints >= freeTalentPoints) ? (totalTalentPoints - freeTalentPoints) : 0;

        return json{
            {"active_spec", player->GetActiveSpec()},
            {"specs_count", player->GetSpecsCount()},
            {"talent_points", {{"available", freeTalentPoints}, {"used", usedTalentPoints}, {"total", totalTalentPoints}}}
        };
    }

    json GetPlayerData(Player* player, bool includeEquipment)
    {
        if (!player)
            return json::object();

        WorldSession* session = player->GetSession();

        json data = {
            {"name", player->GetName()},
            {"level", player->GetLevel()},
            {"class", player->getClass()},
            {"race", player->getRace()},
            {"gender", player->getGender()},
            {"guid", player->GetGUID().GetCounter()},
            {"zone_id", player->GetZoneId()},
            {"area_id", player->GetAreaId()},
            {"map_id", player->GetMapId()},
            {"online", true},
            {"money", player->GetMoney()},
            {"played_time", {{"total", player->GetTotalPlayedTime()}, {"level", player->GetLevelPlayedTime()}}},
            {"honor_points", player->GetHonorPoints()},
            {"arena_points", player->GetArenaPoints()},
            {"position", {{"x", player->GetPositionX()}, {"y", player->GetPositionY()}, {"z", player->GetPositionZ()}, {"orientation", player->GetOrientation()}}},
            {"health", {{"current", player->GetHealth()}, {"max", player->GetMaxHealth()}}},
            {"stats", {{"strength", player->GetStat(STAT_STRENGTH)}, {"agility", player->GetStat(STAT_AGILITY)}, {"stamina", player->GetStat(STAT_STAMINA)}, {"intellect", player->GetStat(STAT_INTELLECT)}, {"spirit", player->GetStat(STAT_SPIRIT)}, {"average_item_level", player->GetAverageItemLevel()}}},
            {"status", {{"alive", player->IsAlive()}, {"in_combat", player->IsInCombat()}, {"ghost", player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST)}, {"resting", player->HasPlayerFlag(PLAYER_FLAGS_RESTING)}, {"away", player->isAFK()}, {"dnd", player->isDND()}, {"gm", player->IsGameMaster()}}}
        };

        Powers primaryPower = player->getPowerType();
        data["power"] = {{"type", static_cast<uint32>(primaryPower)}, {"current", player->GetPower(primaryPower)}, {"max", player->GetMaxPower(primaryPower)}};

        if (session)
        {
            data["account_id"] = session->GetAccountId();
            data["account_name"] = session->GetPlayerName();
            data["latency"] = session->GetLatency();
            data["security_level"] = static_cast<uint32>(session->GetSecurity());
        }

        if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId()))
            data["guild"] = {{"id", player->GetGuildId()}, {"name", guild->GetName()}, {"rank", player->GetRank()}};
        else
            data["guild"] = nullptr;

        if (Group* group = player->GetGroup())
            data["group"] = {{"id", group->GetGUID().GetCounter()}, {"leader_guid", group->GetLeaderGUID().GetCounter()}, {"members_count", group->GetMembersCount()}, {"is_leader", group->IsLeader(player->GetGUID())}, {"is_assistant", group->IsAssistant(player->GetGUID())}, {"loot_method", static_cast<uint32>(group->GetLootMethod())}, {"is_raid", group->isRaidGroup()}, {"is_bg_group", group->isBGGroup()}, {"is_lfg_group", group->isLFGGroup()}};
        else
            data["group"] = nullptr;

        if (includeEquipment)
            data["equipment"] = GetPlayerEquipment(player);

        return data;
    }

    json GetAllPlayersData(bool includeEquipment)
    {
        json players = json::array();

        sWorldSessionMgr->DoForAllOnlinePlayers([&](Player* player)
        {
            if (player->IsInWorld())
                players.push_back(GetPlayerData(player, includeEquipment));
        });

        return players;
    }

    Player* FindPlayerByName(std::string const& name)
    {
        return ObjectAccessor::FindPlayerByName(name);
    }

    json GetPlayerSkills(Player* player)
    {
        if (!player)
            return json::object();

        json castableSpells = json::array();

        PlayerSpellMap const& spellMap = player->GetSpellMap();
        for (auto const& spellPair : spellMap)
        {
            uint32 spellId = spellPair.first;
            PlayerSpell const* playerSpell = spellPair.second;

            if (playerSpell->State == PLAYERSPELL_REMOVED)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo || spellInfo->IsPassive())
                continue;

            castableSpells.push_back({
                {"spell_id", spellId},
                {"name", spellInfo->SpellName[0] ? spellInfo->SpellName[0] : "Unknown"},
                {"rank", spellInfo->Rank[0] ? spellInfo->Rank[0] : ""},
                {"school", spellInfo->SchoolMask},
                {"cast_time", spellInfo->CastTimeEntry ? spellInfo->CastTimeEntry->CastTime : 0},
                {"cooldown", spellInfo->RecoveryTime},
                {"range", spellInfo->RangeEntry ? spellInfo->RangeEntry->RangeMax[0] : 0.0f}
            });
        }

        return json{{"castable_spells", castableSpells}, {"spell_count", castableSpells.size()}};
    }

    json GetPlayerSkillsFull(Player* player)
    {
        if (!player)
            return json::object();

        json skillsArray = json::array();

        for (uint32 i = 0; i < PLAYER_MAX_SKILLS; ++i)
        {
            uint32 skill = player->GetUInt32Value(PLAYER_SKILL_INFO_1_1 + i * 3);
            if (skill == 0)
                continue;

            uint16 skillId = SKILL_VALUE(skill);
            uint16 skillStep = SKILL_MAX(skill);
            if (skillId == 0)
                continue;

            json skillData = {
                {"skill_id", skillId},
                {"skill_step", skillStep},
                {"current_value", player->GetSkillValue(skillId)},
                {"max_value", player->GetMaxSkillValue(skillId)},
                {"pure_value", player->GetPureSkillValue(skillId)},
                {"permanent_bonus", player->GetSkillPermBonusValue(skillId)},
                {"temporary_bonus", player->GetSkillTempBonusValue(skillId)}
            };

            if (SkillLineEntry const* skillLineEntry = sSkillLineStore.LookupEntry(skillId))
                skillData["name"] = skillLineEntry->name[0];

            skillsArray.push_back(skillData);
        }

        json castableData = GetPlayerSkills(player);

        return json{
            {"passive_skills", skillsArray},
            {"talents", GetPlayerTalentInfo(player)},
            {"castable_spells", castableData["castable_spells"]},
            {"spell_count", castableData["spell_count"]}
        };
    }

    json GetPlayerQuests(Player* player)
    {
        if (!player)
            return json::object();

        json activeQuests = json::array();
        json completedQuests = json::array();

        QuestStatusMap& questStatusMap = player->getQuestStatusMap();

        for (auto const& questStatusPair : questStatusMap)
        {
            uint32 questId = questStatusPair.first;
            QuestStatusData const& questStatus = questStatusPair.second;

            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            json questData = {
                {"quest_id", questId},
                {"title", quest->GetTitle()},
                {"description", quest->GetDetails()},
                {"level", quest->GetQuestLevel()},
                {"min_level", quest->GetMinLevel()},
                {"quest_type", quest->GetType()},
                {"suggested_players", quest->GetSuggestedPlayers()},
                {"time_limit", quest->GetTimeAllowed()},
                {"is_daily", quest->IsDaily()},
                {"is_weekly", quest->IsWeekly()},
                {"is_repeatable", quest->IsRepeatable()},
                {"status", static_cast<uint32>(questStatus.Status)}
            };

            if (questStatus.Status == QUEST_STATUS_INCOMPLETE)
            {
                json itemObjectives = json::array();
                for (uint32 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
                    if (quest->RequiredItemId[i] > 0)
                        itemObjectives.push_back({{"item_id", quest->RequiredItemId[i]}, {"required_count", quest->RequiredItemCount[i]}, {"current_count", questStatus.ItemCount[i]}});

                json creatureObjectives = json::array();
                for (uint32 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
                    if (quest->RequiredNpcOrGo[i] != 0)
                        creatureObjectives.push_back({{"npc_or_go_id", quest->RequiredNpcOrGo[i]}, {"required_count", quest->RequiredNpcOrGoCount[i]}, {"current_count", questStatus.CreatureOrGOCount[i]}});

                questData["item_objectives"] = itemObjectives;
                questData["creature_objectives"] = creatureObjectives;
                questData["explored"] = questStatus.Explored;
                questData["timer"] = questStatus.Timer;

                activeQuests.push_back(questData);
            }
            else if (questStatus.Status == QUEST_STATUS_COMPLETE)
            {
                questData["ready_to_turn_in"] = true;
                activeQuests.push_back(questData);
            }
            else if (questStatus.Status == QUEST_STATUS_REWARDED)
            {
                completedQuests.push_back(questData);
            }
        }

        return json{
            {"active_quests", activeQuests},
            {"completed_quests", completedQuests},
            {"active_count", activeQuests.size()},
            {"completed_count", completedQuests.size()}
        };
    }

    json GetPlayerInventory(Player* player)
    {
        if (!player)
            return json::object();

        json backpack = json::array();
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (item)
                backpack.push_back({{"slot", slot - INVENTORY_SLOT_ITEM_START}, {"item", GetItemData(item)}});
        }

        json bags = json::array();
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        {
            Bag* bag = player->GetBagByPos(bagSlot);
            if (!bag)
                continue;

            json contents = json::array();
            for (uint8 slot = 0; slot < bag->GetBagSize(); ++slot)
            {
                Item* item = bag->GetItemByPos(slot);
                if (item)
                    contents.push_back({{"slot", slot}, {"item", GetItemData(item)}});
            }

            bags.push_back({
                {"bagSlot", bagSlot - INVENTORY_SLOT_BAG_START},
                {"bagEntry", bag->GetEntry()},
                {"bagSlots", bag->GetBagSize()},
                {"contents", contents}
            });
        }

        json bankItems = json::array();
        for (uint8 slot = BANK_SLOT_ITEM_START; slot < BANK_SLOT_ITEM_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (item)
                bankItems.push_back({{"slot", slot - BANK_SLOT_ITEM_START}, {"item", GetItemData(item)}});
        }

        json bankBags = json::array();
        for (uint8 slot = BANK_SLOT_BAG_START; slot < BANK_SLOT_BAG_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (item)
                bankBags.push_back({{"slot", slot - BANK_SLOT_BAG_START}, {"item", GetItemData(item)}});
        }

        return json{
            {"backpack", backpack},
            {"bags", bags},
            {"bank", {{"items", bankItems}, {"bags", bankBags}}}
        };
    }

    bool SetGameMaster(Player* player, bool on)
    {
        if (!player)
            return false;

        player->SetGameMaster(on);
        return true;
    }

    void ResetCooldowns(Player* player)
    {
        if (player)
            player->RemoveAllSpellCooldown();
    }

    void SetDisplayId(Player* player, uint32_t displayId)
    {
        if (player)
            player->SetDisplayId(displayId);
    }

    bool CompleteQuest(Player* player, uint32_t questId)
    {
        if (!player)
            return false;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        if (player->CanAddQuest(quest, false))
            player->AddQuest(quest, nullptr);

        player->CompleteQuest(questId);
        return true;
    }
}
