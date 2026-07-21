
#ifndef MCPSP_WORLD_ENTITY_ANIMAL_CHICKEN_H
#define MCPSP_WORLD_ENTITY_ANIMAL_CHICKEN_H

#include "world/entity/animal/animal.h"

class Chicken : public Animal {
public:
    Chicken(Level* level);
    virtual int  getEntityTypeId() const;
    virtual int  getMaxHealth() { return 4; }
    virtual void aiStep();
    virtual void causeFallDamage(float) {}
    virtual void dropDeathLoot();

    virtual const char* getAmbientSound() { return "mob.chicken"; }
    virtual const char* getHurtSound()    { return "mob.chickenhurt"; }
    virtual const char* getDeathSound()   { return "mob.chickenhurt"; }

    float flap, oFlap;
    float flapSpeed, oFlapSpeed;
    float flapping;

private:
    int eggTime;
};

#endif
