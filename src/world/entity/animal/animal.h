
#ifndef MCPSP_WORLD_ENTITY_ANIMAL_ANIMAL_H
#define MCPSP_WORLD_ENTITY_ANIMAL_ANIMAL_H

#include "world/entity/path_finder_mob.h"

class CompoundTag;

class Animal : public PathfinderMob {
public:

    virtual bool removeWhenFarAway() { return !despawnProtected; }
    Animal(Level* level);

    void setDespawnProtected();

    virtual void aiStep();

    virtual bool  hurt(Entity* source, int damage);
    virtual float getWalkTargetValue(int x, int y, int z);
    virtual int   getCreatureBaseType() const;

    virtual int   getAmbientSoundInterval() { return 12 * TicksPerSecond; }

    virtual bool  isBaby() { return age < 0; }
    virtual void  baseTick();
    int  getAge() const { return age; }
    void setAge(int a) { age = a; }

protected:
    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

    void updateDespawnProtectedState();

    static const int MAX_WANDER_DISTANCE = 20;

    int age = 0;
    bool despawnProtected = true;
    int  minWanderX = 0, maxWanderX = 0, minWanderZ = 0, maxWanderZ = 0;
};

#endif
