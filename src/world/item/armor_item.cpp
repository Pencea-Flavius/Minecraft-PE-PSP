#include "world/item/armor_item.h"

const int ArmorItem::healthPerSlot[4] = { 11, 16, 15, 13 };

ArmorItem::ArmorMaterial::ArmorMaterial(int durabilityMultiplier, int p0, int p1, int p2, int p3)
    : durabilityMultiplier(durabilityMultiplier) {
    slotProtections[0] = p0;
    slotProtections[1] = p1;
    slotProtections[2] = p2;
    slotProtections[3] = p3;
}

int ArmorItem::ArmorMaterial::getHealthForSlot(int slot) const {
    return healthPerSlot[slot] * durabilityMultiplier;
}

int ArmorItem::ArmorMaterial::getDefenseForSlot(int slot) const {
    return slotProtections[slot];
}

ArmorItem::ArmorItem(short id, const ArmorMaterial& material, int slot, int icon)
    : Item(id), slot(slot), defense(material.getDefenseForSlot(slot)), icon(icon) {
    maxStackSize = 1;
    maxDamage    = (short)material.getHealthForSlot(slot);
}

const ArmorItem::ArmorMaterial ArmorItem::CLOTH  (5,  1, 3, 2, 1);
const ArmorItem::ArmorMaterial ArmorItem::CHAIN  (15, 2, 5, 4, 1);
const ArmorItem::ArmorMaterial ArmorItem::IRON   (15, 2, 6, 5, 2);
const ArmorItem::ArmorMaterial ArmorItem::GOLD   (7,  2, 5, 3, 1);
const ArmorItem::ArmorMaterial ArmorItem::DIAMOND(33, 3, 8, 6, 3);
