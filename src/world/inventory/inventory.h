#ifndef MCPSP_WORLD_INVENTORY_INVENTORY_H
#define MCPSP_WORLD_INVENTORY_INVENTORY_H

#include "world/inventory/filling_container.h"

class Inventory : public FillingContainer {
public:
    static const int HOTBAR   = 7;

    static const int MAX_SLOTS = 256;
    static const int SURVIVAL_SLOTS = 36;

    explicit Inventory(bool creative);

    void          reinit(bool creative);

    void          setupDefault();

    void          consumeSelected();

    ItemInstance  removeSelected(int n);

    void          setSelectedIfEmpty(short id, unsigned char data);

    bool          hurtSelected(int amount);

    void          ensureHotbar(short id, short data);
    void          selectSlot(int slot) { selected = slot; }
    ItemInstance* getSelected() { return getLinked(selected); }

    void pickToHotbar(int gridIndex);

    bool linkHotbarTo(int slot, short id, unsigned char data);

    int gridSize() const { return _isCreative ? mainCount : SURVIVAL_SLOTS; }

    int           firstGridSlot() const { return numLinkedSlots; }
    ItemInstance* gridItem(int gridIndex) { return getItem(gridIndex + numLinkedSlots); }

    int selected;
    int mainCount;
};

#endif
