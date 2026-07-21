
#ifndef MCPSP_WORLD_ENTITY_MONSTER_SPIDER_H
#define MCPSP_WORLD_ENTITY_MONSTER_SPIDER_H

#include "world/entity/monster/monster.h"

class Spider : public Monster {
public:
    Spider(Level* level);

    virtual int  getMaxHealth() { return 8; }
    virtual void tick();
    virtual bool onLadder();
    virtual void makeStuckInWeb();
    virtual int  getEntityTypeId() const;

    virtual const char* getAmbientSound() { return "mob.spider"; }
    virtual const char* getHurtSound()    { return "mob.spider"; }
    virtual const char* getDeathSound()   { return "mob.spiderdeath"; }

    bool isClimbing() const { return climbing; }

protected:
    virtual Entity* findAttackTarget();
    virtual void    checkHurtTarget(Entity* target, float d);
    virtual int     getDeathLoot();

private:
    bool climbing;
};

#endif
