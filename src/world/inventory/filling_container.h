#ifndef MCPSP_WORLD_INVENTORY_FILLING_CONTAINER_H
#define MCPSP_WORLD_INVENTORY_FILLING_CONTAINER_H

#include "world/Container.h"
#include <vector>

class FillingContainer : public Container {
public:
    static const int MAX_INVENTORY_STACK_SIZE = 254;

    struct LinkedSlot {
        int inventorySlot;
        LinkedSlot() : inventorySlot(-1) {}
    };

    FillingContainer(int numTotalSlots, int numLinkedSlots, int containerType, bool isCreative);
    virtual ~FillingContainer();

    ItemInstance* getItem(int slot);
    void          setItem(int slot, ItemInstance* item);
    ItemInstance  removeItem(int slot, int count);
    int getContainerSize() const { return numTotalSlots; }
    int getMaxStackSize() const  { return Container::LARGE_MAX_STACK_SIZE; }

    virtual bool add(ItemInstance* item);

    int  addItem(ItemInstance* item);

    void clearSlot(int slot);
    void clearInventory();
    int  getFreeSlot() const;

    int removeResource(const ItemInstance& item, bool requireExactAux = false);

    bool isCreative() const { return _isCreative; }

    void reconfigure(int newTotalSlots, bool creative);

    bool          linkSlot(int selectionSlot, int inventorySlot);
    ItemInstance* getLinked(int selectionSlot);

    bool linkEmptySlot(int inventorySlot);
    void compressLinkedSlotList(int slot);

    int numTotalSlots;
    int numLinkedSlots;
    LinkedSlot* linkedSlots;

protected:

    int addResource(ItemInstance* item);

    std::vector<ItemInstance*> items;
    bool _isCreative;
};

#endif
