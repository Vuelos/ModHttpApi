#include "CompanionAI.h"

#include "MotionMaster.h"
#include "Unit.h"

void CompanionAI::Update(Player* leader, Player* companion)
{
    if (!leader || !companion)
        return;

    if (!leader->IsAlive())
        return;

    Combat(leader, companion);
    Follow(leader, companion);
}

void CompanionAI::Follow(Player* leader, Player* companion)
{
    if (companion->IsInCombat())
        return;

    float dist = leader->GetDistance(companion);

    if (dist < 5.0f)
        return;

    // Don't re-issue follow if already following
    if (companion->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE)
        return;

    companion->GetMotionMaster()->MoveFollow(
        leader,
        1.5f,
        0.0f
    );
}

void CompanionAI::Combat(Player* leader, Player* companion)
{
    Unit* target = leader->GetVictim();

    if (!target)
    {
        // Stop attacking if leader has no target
        if (companion->IsInCombat())
            companion->AttackStop();
        return;
    }

    // Attack if not in combat or if target changed
    if (companion->GetVictim() != target)
        companion->Attack(target, true);
}
