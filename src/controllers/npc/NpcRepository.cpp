#include "NpcRepository.h"
#include "libs/json.hpp"

#include "MapMgr.h"
#include "ObjectMgr.h"
#include "CreatureData.h"
#include "Creature.h"
#include "Map.h"

using json = nlohmann::json;

namespace NpcRepository
{
    json GetNpcInfo(uint32_t entry)
    {
        CreatureTemplate const* ct = sObjectMgr->GetCreatureTemplate(entry);
        if (!ct)
            return json();

        json models = json::array();
        for (auto const& model : ct->Models)
            models.push_back(model.CreatureDisplayID);

        return json{
            {"entry", ct->Entry},
            {"name", ct->Name},
            {"subname", ct->SubName},
            {"icon", ct->IconName},
            {"minlevel", ct->minlevel},
            {"maxlevel", ct->maxlevel},
            {"faction", ct->faction},
            {"npcflag", ct->npcflag},
            {"rank", ct->rank},
            {"type", ct->type},
            {"family", ct->family},
            {"unit_class", ct->unit_class},
            {"unit_flags", ct->unit_flags},
            {"unit_flags2", ct->unit_flags2},
            {"models", models},
            {"speed_walk", ct->speed_walk},
            {"speed_run", ct->speed_run},
            {"health_mod", ct->ModHealth},
            {"mana_mod", ct->ModMana},
            {"armor_mod", ct->ModArmor},
            {"experience_mod", ct->ModExperience},
            {"lootid", ct->lootid},
            {"pickpocket_loot", ct->pickpocketLootId},
            {"skinning_loot", ct->SkinLootId},
            {"mingold", ct->mingold},
            {"maxgold", ct->maxgold},
            {"ai_name", ct->AIName},
            {"movement_type", ct->MovementType},
            {"racial_leader", ct->RacialLeader},
            {"regenerates_health", ct->RegenHealth},
            {"flags_extra", ct->flags_extra}
        };
    }

    bool NpcSpeak(uint64_t rawGuid, std::string const& text, std::string const& type)
    {
        ObjectGuid objGuid(rawGuid);

        bool found = false;
        sMapMgr->DoForAllMaps([&](Map* map)
        {
            if (found) return;
            if (Creature* creature = map->GetCreature(objGuid))
            {
                if (type == "yell")
                    creature->Yell(text, LANG_UNIVERSAL);
                else if (type == "emote")
                    creature->TextEmote(text);
                else
                    creature->Say(text, LANG_UNIVERSAL);
                found = true;
            }
        });

        return found;
    }
}
