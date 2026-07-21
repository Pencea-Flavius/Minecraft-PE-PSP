
#ifndef MCPSP_WORLD_ENTITY_PAINTING_H
#define MCPSP_WORLD_ENTITY_PAINTING_H

#include "world/entity/hanging_entity.h"

class Painting : public HangingEntity {
    typedef HangingEntity super;
public:
    Painting(Level* level);
    Painting(Level* level, int xTile, int yTile, int zTile, int dir);
    Painting(Level* level, int x, int y, int z, int dir, const char* motiveName);

    void setRandomMotive(int dir);

    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

    virtual int getWidth();
    virtual int getHeight();
    virtual void dropItem();
    virtual int getEntityTypeId() const;
    virtual bool isPickable();

    int motive;
};

#endif
