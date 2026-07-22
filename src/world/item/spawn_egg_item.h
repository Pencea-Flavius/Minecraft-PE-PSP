
#ifndef MCPSP_WORLD_ITEM_SPAWN_EGG_ITEM_H
#define MCPSP_WORLD_ITEM_SPAWN_EGG_ITEM_H

#include "world/item/item.h"

class SpawnEggItem : public Item {
public:
    SpawnEggItem();
    virtual int  getIcon(short data) const;
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face, float, float, float);
};

#endif
