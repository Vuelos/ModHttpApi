#include "ItemRepository.h"
#include "libs/json.hpp"

#include "Player.h"
#include "Item.h"

using json = nlohmann::json;

namespace ItemRepository
{
    json GetItemData(Player* player, uint32_t itemEntry)
    {
        if (!player)
            return json::object();

        return json{
            {"player", player->GetName()},
            {"itemEntry", itemEntry},
            {"count", player->GetItemCount(itemEntry)}
        };
    }

    json GiveItem(Player* player, uint32_t itemEntry, int32_t count)
    {
        ItemPosCountVec dest;
        InventoryResult result = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemEntry, count);
        if (result != EQUIP_ERR_OK)
            return json();

        Item* item = player->StoreNewItem(dest, itemEntry, true);
        if (!item)
            return json();

        player->SendNewItem(item, count, true, false);
        return json{{"success", true}};
    }

    json RemoveItem(Player* player, uint32_t itemEntry, int32_t count)
    {
        uint32_t removeCount = static_cast<uint32_t>(-count);
        if (!player->HasItemCount(itemEntry, removeCount))
            return json();

        player->DestroyItemCount(itemEntry, removeCount, true);
        return json{{"success", true}};
    }
}
