#ifndef MCPSP_WORLD_ITEM_BONEMEAL_ITEM_H
#define MCPSP_WORLD_ITEM_BONEMEAL_ITEM_H

#include "world/item/item.h"

class BonemealItem : public Item {
public:
    BonemealItem(short id) : Item(id) {
        maxStackSize = 64;
        maxDamage = 0;
    }
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face);

    virtual int  getIcon(short data) const {
        int c = data; if (c < 0) c = 0; if (c > 15) c = 15;
        return 78 + (c % 8) * 16 + (c / 8);
    }
};

#endif
