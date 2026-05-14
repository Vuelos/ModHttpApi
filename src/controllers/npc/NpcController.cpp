#include "NpcController.h"
#include "NpcRepository.h"
#include "libs/json.hpp"

using json = nlohmann::json;

namespace NpcController
{
    CommandResult GetNpcInfo(uint32_t entry)
    {
        if (entry == 0)
            return CommandResult::Fail(400, "missing or invalid entry");

        json result = NpcRepository::GetNpcInfo(entry);
        if (result.empty())
            return CommandResult::Fail(404, "creature template not found");

        return CommandResult::Ok(result.dump());
    }

    CommandResult NpcSay(uint64_t guid, std::string const& text, std::string const& type)
    {
        if (text.empty())
            return CommandResult::Fail(400, "missing 'text' field");

        if (!NpcRepository::NpcSpeak(guid, text, type))
            return CommandResult::Fail(404, "creature not found");

        return CommandResult::Ok(json{{"spoken", true}}.dump());
    }
}
