#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct BroadcastParams
{
    std::string message;
    std::string type = "system";
    std::optional<uint32_t> mapId;
    std::optional<float>    x, y, z;
};
