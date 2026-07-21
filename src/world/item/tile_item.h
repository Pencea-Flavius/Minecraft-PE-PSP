#ifndef MCPSP_WORLD_ITEM_TILE_ITEM_H
#define MCPSP_WORLD_ITEM_TILE_ITEM_H

#include "world/item/item.h"

class TileItem : public Item {
public:
    TileItem(short id) : Item(id) {
        maxStackSize = 64;
        maxDamage = 0;
    }
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face);
};

#endif
