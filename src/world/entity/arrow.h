
#ifndef MCPSP_WORLD_ENTITY_ARROW_H
#define MCPSP_WORLD_ENTITY_ARROW_H

#include "world/entity/entity.h"

class Arrow : public Entity {
    typedef Entity super;
public:
    Arrow(Level* level);
    Arrow(Level* level, float x, float y, float z,
          float yaw, float pitch, float speed, bool crit,
          bool fromPlayer = false);

    virtual void tick();
    virtual int  getEntityTypeId() const;

    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

    int  ownerId;
    bool inGround;
    int  life;
    int  shakeTime;
    bool critArrow;

    bool playerArrow;
    int  xTile, yTile, zTile;
    int  lastTile;
};

#endif
