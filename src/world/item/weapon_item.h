#ifndef MCPSP_WORLD_ITEM_WEAPON_ITEM_H
#define MCPSP_WORLD_ITEM_WEAPON_ITEM_H

#include "world/item/item.h"

class WeaponItem : public Item {
public:
    WeaponItem(short id, const Tier& tier, int icon)
        : Item(id), damage(4 + tier.getAttackDamageBonus()), icon(icon) {
        maxStackSize = 1;
        maxDamage    = (short)tier.getUses();
    }

    virtual float getDestroySpeed(int blockId) const { return blockId == BLOCK_COBWEB ? 15.0f : 1.5f; }
    virtual int   getAttackDamage() const { return damage; }
    virtual bool  canDestroySpecial(int blockId) const { return blockId == BLOCK_COBWEB; }
    virtual int   getMineDurabilityCost() const { return 2; }
    virtual int   getHurtEnemyDurabilityCost() const { return 1; }
    virtual bool  isHandEquipped() const { return true; }
    virtual int   getIcon(short data) const { return icon; }
private:
    int damage;
    int icon;
};

#endif
