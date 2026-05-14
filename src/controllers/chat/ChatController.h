#pragma once

#include "common/CommandResult.h"
#include "models/BroadcastParams.h"

namespace ChatController
{
    CommandResult Broadcast(BroadcastParams const& params);
}
