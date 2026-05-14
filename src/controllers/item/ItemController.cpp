#include "ItemController.h"
#include "ItemRepository.h"
#include "controllers/player/PlayerRepository.h"
#include "libs/json.hpp"

#include "Player.h"

using json = nlohmann::json;

namespace ItemController
{
    CommandResult ModifyItem(ItemParams const& params)
    {
        Player* player = PlayerRepository::FindPlayerByName(params.playerName);
        if (!player)
            return CommandResult::Fail(404, "player not found or offline");

        if (params.count > 0)
        {
            json result = ItemRepository::GiveItem(player, params.itemEntry, params.count);
            if (result.empty())
                return CommandResult::Fail(409, "cannot store item: inventory full or invalid entry");
        }
        else if (params.count < 0)
        {
            json result = ItemRepository::RemoveItem(player, params.itemEntry, params.count);
            if (result.empty())
                return CommandResult::Fail(409, "player does not have enough of that item");
        }
        else
        {
            return CommandResult::Fail(400, "'count' must not be zero");
        }

        json j = {{"player", params.playerName}, {"itemEntry", params.itemEntry}, {"count", params.count}};
        return CommandResult::Ok(j.dump());
    }
}
