
#ifndef MCPSP_WORLD_ENTITY_ITEM_ENTITY_H
#define MCPSP_WORLD_ENTITY_ITEM_ENTITY_H

#include "world/entity/entity.h"
#include "world/item/item_instance.h"

class ItemEntity : public Entity {
    typedef Entity super;
public:
    ItemEntity(Level* level);
    ItemEntity(Level* level, float x, float y, float z, const ItemInstance& item);

    virtual void tick();
    virtual bool hurt(Entity* source, int damage);
    virtual bool isItemEntity();
    virtual int  getEntityTypeId() const;
    int getLifeTime() const;

    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

    ItemInstance item;
    int   age;
    int   throwTime;
    float bobOffs;

protected:
    virtual void burn(int dmg);

private:
    bool checkInTile(float x, float y, float z);
    void tryPlayerPickup();
    void tryMergeNearby();

    int tickCount;
    int health;
    int lifeTime;
};

#endif
