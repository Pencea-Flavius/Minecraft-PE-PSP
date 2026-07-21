#ifndef MCPSP_WORLD_ITEM_BOW_ITEM_H
#define MCPSP_WORLD_ITEM_BOW_ITEM_H

#include "world/item/item.h"

class BowItem : public Item {
public:
    BowItem(short id) : Item(id) {
        maxStackSize = 1;
        maxDamage = 384;
    }
    virtual bool isHandEquipped() const { return true; }
    virtual int  getIcon(short data) const { return 5 + 1 * 16; }
};

#endif
