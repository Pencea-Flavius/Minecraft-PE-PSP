
#ifndef MCPSP_WORLD_ENTITY_PATH_FINDER_MOB_H
#define MCPSP_WORLD_ENTITY_PATH_FINDER_MOB_H

#include "world/entity/mob.h"
#include "world/level/pathfinder/path.h"

class PathfinderMob : public Mob {
public:
    PathfinderMob(Level* level);

    virtual void  updateAi();
    virtual float getWalkingSpeedModifier();
    virtual float getWalkTargetValue(int x, int y, int z) { return 0.0f; }
    virtual Entity* findAttackTarget() { return 0; }

    virtual void checkHurtTarget(Entity* target, float dist) {}
    virtual void checkCantSeeTarget(Entity* target, float dist);

protected:
    void findRandomStrollLocation();
    bool isPathFinding();

    Path  path;
    int   attackTargetId;
    int   fleeTime;
    float runSpeed;

    bool  holdGround;
    static const int MAX_TURN = 30;
};

#endif
