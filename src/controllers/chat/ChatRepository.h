#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace ChatRepository
{
    bool SendSystemMessage(std::string const& message);
    bool SendPositionalMessage(std::string const& message, std::string const& type, uint32_t mapId, float x, float y, float z);
}
