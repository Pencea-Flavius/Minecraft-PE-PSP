
#ifndef MCPSP_WORLD_ENTITY_FALLING_TILE_H
#define MCPSP_WORLD_ENTITY_FALLING_TILE_H

#include "world/entity/entity.h"

class FallingTile : public Entity {
    typedef Entity super;
public:
    FallingTile(Level* level);
    FallingTile(Level* level, float x, float y, float z, int tile, int data);

    virtual void tick();
    virtual int  getEntityTypeId() const;
    virtual float getShadowHeightOffs() { return 0.0f; }
    virtual bool  isPickable() { return !removed; }

    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

    int tile;
    int data;
    int time;

private:
    void init();
};

#endif
