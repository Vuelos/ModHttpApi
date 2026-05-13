#pragma once

#include "ObjectGuid.h"
#include "Player.h"
#include <unordered_map>
#include <vector>

class WorldSession;

class SquadManager
{
public:
    static SquadManager* instance();

    void LoadSquad(Player* leader);
    void DespawnSquad(Player* leader);
    void SwapTo(Player* player, ObjectGuid const& targetGuid);

    std::vector<Player*> GetSquad(Player* leader);

private:
    std::unordered_map<uint64, std::vector<Player*>> _squads;
};

#define sSquadMgr SquadManager::instance()
