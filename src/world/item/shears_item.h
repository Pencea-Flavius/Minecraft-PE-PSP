#ifndef MCPSP_WORLD_ITEM_SHEARS_ITEM_H
#define MCPSP_WORLD_ITEM_SHEARS_ITEM_H

#include "world/item/item.h"

class ShearsItem : public Item {
public:
    ShearsItem(short id, int icon) : Item(id), icon(icon) {
        maxStackSize = 1;
        maxDamage    = 238;
    }
    virtual float getDestroySpeed(int blockId) const {
        if (blockId == BLOCK_COBWEB || blockId == BLOCK_LEAVES) return 15.0f;
        if (blockId == BLOCK_WOOL)                              return 5.0f;
        return Item::getDestroySpeed(blockId);
    }
    virtual bool canDestroySpecial(int blockId) const { return blockId == BLOCK_COBWEB; }

    virtual bool mineBlock(ItemInstance* item, World* world, int blockId, int x, int y, int z, Player* player);
    virtual int getIcon(short) const { return icon; }
private:
    int icon;
};

#endif
