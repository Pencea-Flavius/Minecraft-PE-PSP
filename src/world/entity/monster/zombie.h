
#ifndef MCPSP_WORLD_ENTITY_MONSTER_ZOMBIE_H
#define MCPSP_WORLD_ENTITY_MONSTER_ZOMBIE_H

#include "world/entity/monster/monster.h"

class Zombie : public Monster {
public:
    Zombie(Level* level);

    virtual int  getMaxHealth() { return 12; }
    virtual void aiStep();
    virtual int  getEntityTypeId() const;
    virtual void dropDeathLoot();

    virtual const char* getAmbientSound() { return "mob.zombie"; }
    virtual const char* getHurtSound()    { return "mob.zombiehurt"; }
    virtual const char* getDeathSound()   { return "mob.zombiedeath"; }

protected:
    Zombie(Level* level, int rendererId);
};

#endif
