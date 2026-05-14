#pragma once

#include "common/CommandResult.h"
#include <cstdint>
#include <string>
#include <optional>

namespace ServerController
{
    CommandResult GetServerInfo();
    CommandResult GetTime();
    CommandResult GetGroundZ(uint32_t mapId, float x, float y);
    CommandResult GetZone(uint32_t mapId, float x, float y, float z);
}
