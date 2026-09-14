
#ifndef MCPSP_WORLD_ENTITY_AI_GOALS_BREAK_DOOR_GOAL_H
#define MCPSP_WORLD_ENTITY_AI_GOALS_BREAK_DOOR_GOAL_H

#include "world/entity/ai/goals/door_interact_goal.h"

class BreakDoorGoal : public DoorInteractGoal {
public:
    BreakDoorGoal(Mob* mob) : DoorInteractGoal(mob), ticksLeft(0), lastBreakProgress(-1) {}
    virtual bool canUse();
    virtual bool canContinueToUse();
    virtual void start();
    virtual void stop();
    virtual void tick();
private:
    int ticksLeft;
    int lastBreakProgress;
};

#endif
