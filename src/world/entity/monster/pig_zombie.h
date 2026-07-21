
#ifndef MCPSP_WORLD_ENTITY_MONSTER_PIG_ZOMBIE_H
#define MCPSP_WORLD_ENTITY_MONSTER_PIG_ZOMBIE_H

#include "world/entity/monster/zombie.h"

class PigZombie : public Zombie {
public:
    PigZombie(Level* level);

    virtual void tick();
    virtual bool hurt(Entity* source, int damage);
    virtual int  getEntityTypeId() const;
    virtual int  getAttackTime() { return 40; }

    virtual const char* getAmbientSound() { return "mob.zombiepig.zpig"; }
    virtual const char* getHurtSound()    { return "mob.zombiepig.zpighurt"; }
    virtual const char* getDeathSound()   { return "mob.zombiepig.zpigdeath"; }

    void alert(Entity* target);

protected:
    virtual Entity* findAttackTarget();
    virtual void dropDeathLoot();
    virtual int  getDeathLoot() { return 0; }

    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

private:
    int angerTime, playAngrySoundIn, stunedTime;
};

#endif
