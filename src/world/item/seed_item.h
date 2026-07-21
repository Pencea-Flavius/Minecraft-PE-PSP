#ifndef MCPSP_WORLD_ITEM_SEED_ITEM_H
#define MCPSP_WORLD_ITEM_SEED_ITEM_H

#include "world/item/item.h"

class SeedItem : public Item {
public:
    short resultTile;
    int   icon;

    SeedItem(short id, short resultTile, int icon) : Item(id), resultTile(resultTile), icon(icon) {
        maxStackSize = 64;
        maxDamage = 0;
    }
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face);
    virtual int  getIcon(short data) const { return icon; }
};

#endif
