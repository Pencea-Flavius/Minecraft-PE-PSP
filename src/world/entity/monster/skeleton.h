
#ifndef MCPSP_WORLD_ENTITY_MONSTER_SKELETON_H
#define MCPSP_WORLD_ENTITY_MONSTER_SKELETON_H

#include "world/entity/monster/monster.h"

class Skeleton : public Monster {
public:
    Skeleton(Level* level);

    virtual int  getMaxHealth() { return 10; }
    virtual void aiStep();
    virtual int  getEntityTypeId() const;
    virtual void dropDeathLoot();

    virtual const char* getAmbientSound() { return "mob.skeleton"; }
    virtual const char* getHurtSound()    { return "mob.skeletonhurt"; }
    virtual const char* getDeathSound()   { return "mob.skeletonhurt"; }

protected:
    virtual void checkHurtTarget(Entity* target, float d);
};

#endif
