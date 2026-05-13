#include "CompanionAI.h"
#include "Player.h"
#include "PlayerScript.h"
#include "SquadManager.h"

class SquadPlayerScript : public PlayerScript
{
public:
    SquadPlayerScript() : PlayerScript("SquadPlayerScript")
    {
    }

    void OnPlayerLogin(Player* player) override
    {
        sSquadMgr->LoadSquad(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        sSquadMgr->DespawnSquad(player);
    }

    void OnPlayerMapChanged(Player* player) override
    {
        if (sSquadMgr->GetSquad(player).empty())
            return;

        sSquadMgr->DespawnSquad(player);
        sSquadMgr->LoadSquad(player);
    }

    void OnPlayerUpdate(Player* player, uint32 /*diff*/) override
    {
        auto squad = sSquadMgr->GetSquad(player);

        for (Player* member : squad)
        {
            CompanionAI::Update(player, member);
        }
    }
};

void AddSquadPlayerScript()
{
    new SquadPlayerScript();
}
