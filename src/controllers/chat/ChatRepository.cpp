#include "ChatRepository.h"

#include "Chat.h"
#include "MapMgr.h"
#include "Map.h"

namespace ChatRepository
{
    bool SendSystemMessage(std::string const& message)
    {
        ChatHandler(nullptr).SendWorldText(LANG_SYSTEMMESSAGE, message.c_str());
        return true;
    }

    bool SendPositionalMessage(std::string const& message, std::string const& type, uint32_t mapId, float x, float y, float z)
    {
        Map* map = sMapMgr->FindBaseNonInstanceMap(mapId);
        if (!map)
            return false;

        std::string prefixed = std::string("[") + (type == "yell" ? "YELL" : "SAY") + "] " + message;
        ChatHandler(nullptr).SendWorldText(LANG_SYSTEMMESSAGE, prefixed.c_str());
        return true;
    }
}
