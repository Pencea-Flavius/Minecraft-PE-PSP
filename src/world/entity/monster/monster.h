
#ifndef MCPSP_WORLD_ENTITY_MONSTER_MONSTER_H
#define MCPSP_WORLD_ENTITY_MONSTER_MONSTER_H

#include "world/entity/path_finder_mob.h"

class Monster : public PathfinderMob {
public:
    Monster(Level* level);

    virtual void tick();
    virtual bool hurt(Entity* source, int damage);
    virtual int  getCreatureBaseType() const;

    bool canSpawn();

    virtual bool doHurtTarget(Entity* target);
    virtual int  getAttackTime() { return 20; }

protected:
    virtual Entity* findAttackTarget();
    bool  isDarkEnoughToSpawn();
    virtual void  checkHurtTarget(Entity* target, float distance);
    virtual float getWalkTargetValue(int x, int y, int z);

    void updateSunburn();

    int attackDamage;
    int sunburnCounter;

    int frozenTicks;
};

#endif
