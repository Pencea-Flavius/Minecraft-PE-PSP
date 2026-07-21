
#ifndef MCPSP_WORLD_ENTITY_PRIMED_TNT_H
#define MCPSP_WORLD_ENTITY_PRIMED_TNT_H

#include "world/entity/entity.h"

class PrimedTnt : public Entity {
    typedef Entity super;
public:
    PrimedTnt(Level* level);
    PrimedTnt(Level* level, float x, float y, float z, int fuse);

    virtual void tick();
    virtual int  getEntityTypeId() const;
    virtual float getShadowHeightOffs() { return 0.0f; }
    virtual bool  isPickable() { return !removed; }

    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

    int life;

private:
    void init();
};

#endif
