
#ifndef MCPSP_WORLD_ENTITY_ANIMAL_COW_H
#define MCPSP_WORLD_ENTITY_ANIMAL_COW_H

#include "world/entity/animal/animal.h"

class Cow : public Animal {
public:
    Cow(Level* level);
    virtual int  getEntityTypeId() const;
    virtual int  getMaxHealth() { return 10; }
    virtual void dropDeathLoot();

    virtual const char* getAmbientSound() { return "mob.cow"; }
    virtual const char* getHurtSound()    { return "mob.cowhurt"; }
    virtual const char* getDeathSound()   { return "mob.cowhurt"; }
    virtual float getSoundVolume() { return 0.4f; }
};

#endif
