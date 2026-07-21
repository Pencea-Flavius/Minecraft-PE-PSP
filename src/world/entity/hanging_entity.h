
#ifndef MCPSP_WORLD_ENTITY_HANGING_ENTITY_H
#define MCPSP_WORLD_ENTITY_HANGING_ENTITY_H

#include "world/entity/entity.h"

class HangingEntity : public Entity {
    typedef Entity super;
public:
    HangingEntity(Level* level);
    HangingEntity(Level* level, int xTile, int yTile, int zTile, int dir);

    void init();
    void setDir(int dir);
    void setPosition(int x, int y, int z);

    virtual void tick();
    virtual bool survives();
    virtual bool isPickable();
    virtual bool interact();
    virtual void move(float xa, float ya, float za);
    virtual void push(float xa, float ya, float za);
    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);
    virtual float getBrightness(float a);
    int getRawBrightness();
    virtual bool hurt(Entity* source, int damage);
    virtual bool isHangingEntity();

    virtual int getWidth() = 0;
    virtual int getHeight() = 0;
    virtual void dropItem() = 0;

private:
    float offs(int w);

public:
    int dir;
    int xTile, yTile, zTile;
private:
    int checkInterval;
};

#endif
