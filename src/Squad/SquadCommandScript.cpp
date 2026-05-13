#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SquadManager.h"

class SquadCommandScript : public CommandScript
{
public:
    SquadCommandScript() : CommandScript("SquadCommandScript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> squadCommandTable =
        {
            { "swap", rbac::RBAC_PERM_COMMAND_GM, true, &HandleSquadSwapCommand, "" },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "squad", rbac::RBAC_PERM_COMMAND_GM, false, nullptr, "", squadCommandTable },
        };

        return commandTable;
    }

    static bool HandleSquadSwapCommand(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player || !*args)
            return false;

        std::string targetName = strtok((char*)args, " ");
        if (targetName.empty())
            return false;

        Player* target = ObjectAccessor::FindPlayerByName(targetName, false);
        if (target)
        {
            sSquadMgr->SwapTo(player, target->GetGUID());
            return true;
        }

        // Check companions by name
        auto squad = sSquadMgr->GetSquad(player);
        for (Player* member : squad)
        {
            if (member && member->GetName() == targetName)
            {
                sSquadMgr->SwapTo(player, member->GetGUID());
                return true;
            }
        }

        handler->PSendSysMessage("Squad member not found: {}", targetName);
        return true;
    }
};

void AddSquadCommandScript()
{
    new SquadCommandScript();
}
