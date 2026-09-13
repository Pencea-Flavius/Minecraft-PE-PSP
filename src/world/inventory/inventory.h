#ifndef MCPSP_WORLD_INVENTORY_INVENTORY_H
#define MCPSP_WORLD_INVENTORY_INVENTORY_H

#include "world/inventory/filling_container.h"

class Entity;

class Inventory : public FillingContainer {
public:
    static const int HOTBAR   = 8;

    static const int SURVIVAL_SLOTS = 36;

    explicit Inventory(bool creative);

    void          reinit(bool creative);

    void          setupDefault();

    void          consumeSelected();

    ItemInstance  removeSelected(int n);

    bool          replaceSelected(short id, unsigned char data);

    bool          hurtSelected(int amount);

    int           getAttackDamage(Entity* target);

    void          ensureHotbar(short id, short data);
    void          selectSlot(int slot) { selected = slot; }
    ItemInstance* getSelected() { return getLinked(selected); }

    int  getLinkedSlotForItem(int invSlot) const;

    int  survivalAddItem(int gridIndex);

    int  getLinkedSlotForItemAndAux(short id, short aux) const;

    int  creativeAddItem(short id, short count, short aux);

    int  classicSurvivalAddItem(int gridIndex);
    int  classicCreativeAddItem(short id, short count, short aux);

    int  ownableGridSlot(int h);

    int gridSize() const { return SURVIVAL_SLOTS; }

    int           firstGridSlot() const { return numLinkedSlots; }
    ItemInstance* gridItem(int gridIndex) { return getItem(gridIndex + numLinkedSlots); }

    int selected;
};

#endif
