#pragma once

#include "common/CommandResult.h"
#include "models/ItemParams.h"

namespace ItemController
{
    CommandResult ModifyItem(ItemParams const& params);
}
