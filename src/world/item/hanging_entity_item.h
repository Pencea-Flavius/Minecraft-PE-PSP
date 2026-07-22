
#ifndef MCPSP_WORLD_ITEM_HANGING_ENTITY_ITEM_H
#define MCPSP_WORLD_ITEM_HANGING_ENTITY_ITEM_H

#include "world/item/item.h"

class HangingEntity;

class HangingEntityItem : public Item {
public:
    HangingEntityItem(short id, int entityType, int icon);
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face, float, float, float);
    virtual int  getIcon(short data) const { return icon; }

private:
    HangingEntity* createEntity(class Level* level, int x, int y, int z, int dir);
    int entityType;
    int icon;
};

#endif
