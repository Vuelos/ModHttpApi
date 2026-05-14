#pragma once

#include "common/CommandResult.h"
#include <string>
#include <cstdint>

namespace NpcController
{
    CommandResult GetNpcInfo(uint32_t entry);
    CommandResult NpcSay(uint64_t guid, std::string const& text, std::string const& type);
}
