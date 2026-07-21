
#ifndef MCPSP_WORLD_ENTITY_ANIMAL_PIG_H
#define MCPSP_WORLD_ENTITY_ANIMAL_PIG_H

#include "world/entity/animal/animal.h"

class Pig : public Animal {
public:
    Pig(Level* level);

    virtual int  getEntityTypeId() const;
    virtual int  getMaxHealth() { return 10; }
    virtual int  getDeathLoot();

    virtual const char* getAmbientSound() { return "mob.pig"; }
    virtual const char* getHurtSound()    { return "mob.pig"; }
    virtual const char* getDeathSound()   { return "mob.pigdeath"; }
};

#endif
