#pragma once

#include "libs/json_fwd.hpp"
#include <string>
#include <cstdint>

namespace NpcRepository
{
    nlohmann::json GetNpcInfo(uint32_t entry);
    bool NpcSpeak(uint64_t guid, std::string const& text, std::string const& type);
}
