#ifndef MCPSP_WORLD_ITEM_SIMPLE_ITEM_H
#define MCPSP_WORLD_ITEM_SIMPLE_ITEM_H

#include "world/item/item.h"

class SimpleItem : public Item {
public:
    SimpleItem(short id, int icon, int stackSize = 64, short dmg = 0, bool handEquipped = false)
        : Item(id), icon(icon), hand(handEquipped) {
        maxStackSize = stackSize;
        maxDamage    = dmg;
    }
    virtual int  getIcon(short data) const { return icon; }
    virtual bool isHandEquipped() const { return hand; }
private:
    int  icon;
    bool hand;
};

#endif
