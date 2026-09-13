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

    virtual void use(ItemInstance* item, Player* player, World* world);
    virtual int  getMaxUseDuration() const { return 72000; }
    virtual int  getUseAnimation() const { return 4; }
    virtual void releaseUsing(ItemInstance* item, Player* player, int duration);

    float _getLaunchPower(int duration) const;

    static bool hasArrow(Player* player);
};

#endif
