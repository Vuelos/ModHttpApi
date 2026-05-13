#pragma once

#include "Player.h"

class CompanionAI
{
public:
    static void Update(Player* leader, Player* companion);

private:
    static void Follow(Player* leader, Player* companion);
    static void Combat(Player* leader, Player* companion);
};