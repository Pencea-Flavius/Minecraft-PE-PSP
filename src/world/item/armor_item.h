#ifndef MCPSP_WORLD_ITEM_ARMOR_ITEM_H
#define MCPSP_WORLD_ITEM_ARMOR_ITEM_H

#include "world/item/item.h"

class ArmorItem : public Item {
public:
    enum { SLOT_HEAD = 0, SLOT_TORSO = 1, SLOT_LEGS = 2, SLOT_FEET = 3 };

    static const int healthPerSlot[4];

    class ArmorMaterial {
    public:
        ArmorMaterial(int durabilityMultiplier, int p0, int p1, int p2, int p3);
        int getHealthForSlot(int slot) const;
        int getDefenseForSlot(int slot) const;
    private:
        int durabilityMultiplier;
        int slotProtections[4];
    };

    static const ArmorMaterial CLOTH, CHAIN, IRON, GOLD, DIAMOND;

    ArmorItem(short id, const ArmorMaterial& material, int slot, int icon);

    virtual bool isArmor() const { return true; }
    virtual int  getIcon(short data) const { return icon; }
    int  getSlot() const { return slot; }
    int  getDefense() const { return defense; }

private:
    int slot;
    int defense;
    int icon;
};

#endif
