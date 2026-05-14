#pragma once

#include "libs/json_fwd.hpp"
#include <cstdint>
#include <string>

class Player;

namespace ItemRepository
{
    nlohmann::json GetItemData(Player* player, uint32_t itemEntry);
    nlohmann::json GiveItem(Player* player, uint32_t itemEntry, int32_t count);
    nlohmann::json RemoveItem(Player* player, uint32_t itemEntry, int32_t count);
}
