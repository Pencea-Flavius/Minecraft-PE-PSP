
#ifndef MCPSP_WORLD_ENTITY_THROWABLE_H
#define MCPSP_WORLD_ENTITY_THROWABLE_H

#include "world/entity/entity.h"

class Throwable : public Entity {
    typedef Entity super;
public:
    Throwable(Level* level, int type);
    Throwable(Level* level, float x, float y, float z,
              float yaw, float pitch, int type);

    virtual void tick();
    virtual int  getEntityTypeId() const { return type; }
    virtual void addAdditonalSaveData(CompoundTag* ) {}
    virtual void readAdditionalSaveData(CompoundTag* ) {}

    int   type;
    short itemId;
    int   life;

private:
    void configure(int type);
    void shoot(float dx, float dy, float dz, float power);
    void onHit(float hx, float hy, float hz);
};

#endif
