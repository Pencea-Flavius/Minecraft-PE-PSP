#ifndef MCPSP_WORLD_ITEM_HOE_ITEM_H
#define MCPSP_WORLD_ITEM_HOE_ITEM_H

#include "world/item/item.h"

class HoeItem : public Item {
public:
    HoeItem(short id, const Tier& tier, int icon) : Item(id), icon(icon) {
        maxStackSize = 1;
        maxDamage    = (short)tier.getUses();
    }
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face, float, float, float);
    virtual bool isHandEquipped() const { return true; }
    virtual int  getIcon(short data) const { return icon; }
private:
    int icon;
};

#endif
