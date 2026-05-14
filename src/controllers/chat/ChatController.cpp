#include "ChatController.h"
#include "ChatRepository.h"
#include "libs/json.hpp"

using json = nlohmann::json;

namespace ChatController
{
    CommandResult Broadcast(BroadcastParams const& params)
    {
        if (params.message.empty())
            return CommandResult::Fail(400, "missing 'message' field");

        std::string const& type = params.type;

        if (type == "system" || type.empty())
        {
            ChatRepository::SendSystemMessage(params.message);
        }
        else if (type == "say" || type == "yell")
        {
            if (!params.mapId || !params.x || !params.y || !params.z)
                return CommandResult::Fail(400, "'say' and 'yell' types require mapId, x, y, z");

            if (!ChatRepository::SendPositionalMessage(params.message, type, *params.mapId, *params.x, *params.y, *params.z))
                return CommandResult::Fail(404, "map not found or not loaded");
        }
        else
        {
            return CommandResult::Fail(400, "unknown type; use 'system', 'say', or 'yell'");
        }

        return CommandResult::Ok(json{{"sent", true}}.dump());
    }
}
