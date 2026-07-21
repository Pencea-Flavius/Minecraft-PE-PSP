
#ifndef MCPSP_WORLD_ENTITY_MONSTER_CREEPER_H
#define MCPSP_WORLD_ENTITY_MONSTER_CREEPER_H

#include "world/entity/monster/monster.h"

class Creeper : public Monster {
public:
    Creeper(Level* level);

    virtual int  getMaxHealth() { return 16; }
    virtual void tick();
    virtual int  getEntityTypeId() const;

    virtual const char* getHurtSound()  { return "mob.creeper"; }
    virtual const char* getDeathSound() { return "mob.creeperdeath"; }

    float getSwelling(float a) const;
    static const int MAX_SWELL = 30;

protected:
    virtual int  getDeathLoot();
    virtual void checkHurtTarget(Entity* target, float d);
    virtual void checkCantSeeTarget(Entity* target, float d);

private:
    int swell, oldSwell, swellDir;
};

#endif
