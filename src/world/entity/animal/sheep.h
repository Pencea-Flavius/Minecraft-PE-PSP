
#ifndef MCPSP_WORLD_ENTITY_ANIMAL_SHEEP_H
#define MCPSP_WORLD_ENTITY_ANIMAL_SHEEP_H

#include "world/entity/animal/animal.h"

class Sheep : public Animal {
public:
    Sheep(Level* level);

    virtual int  getEntityTypeId() const;
    virtual int  getMaxHealth() { return 8; }
    virtual void dropDeathLoot();
    virtual bool playerInteract();
    virtual void aiStep();

    virtual const char* getAmbientSound() { return "mob.sheep"; }
    virtual const char* getHurtSound()    { return "mob.sheep"; }
    virtual const char* getDeathSound()   { return "mob.sheep"; }

    int  getColor() const { return woolColor; }
    bool isSheared() const { return sheared; }
    void setColor(int c) { woolColor = c & 0x0f; }
    void setSheared(bool v) { sheared = v; }

    static const int EAT_ANIMATION_TICKS = 40;
    int   getEatAnimationTick() const { return eatAnimationTick; }
    float getHeadEatPositionScale(float a) const;
    float getHeadEatAngleScale(float a) const;

    static int getSheepColor(Random& random);

protected:
    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

private:
    int  woolColor;
    bool sheared;
    int  eatAnimationTick = 0;
};

#endif
