#include "world/inventory/filling_container.h"
#include "world/item/item.h"
#include "world/item/crafting/recipe.h"

FillingContainer::FillingContainer(int numTotalSlots, int numLinkedSlots, int containerType, bool isCreative)
    : Container(containerType),
      numTotalSlots(numTotalSlots),
      numLinkedSlots(numLinkedSlots),
      _isCreative(isCreative) {
    items.assign(numTotalSlots, nullptr);
    linkedSlots = new LinkedSlot[numLinkedSlots];
}

FillingContainer::~FillingContainer() {
    clearInventory();
    delete[] linkedSlots;
}

ItemInstance* FillingContainer::getItem(int slot) {
    if (slot < 0 || slot >= (int)items.size()) return nullptr;
    return items[slot];
}

void FillingContainer::setItem(int slot, ItemInstance* item) {
    if (slot < 0 || slot >= (int)items.size()) return;
    delete items[slot];
    items[slot] = item;
}

ItemInstance FillingContainer::removeItem(int slot, int count) {
    ItemInstance* it = getItem(slot);
    if (!it) return ItemInstance();
    ItemInstance out = it->remove(count);
    if (it->isNull()) clearSlot(slot);
    return out;
}

void FillingContainer::clearSlot(int slot) {
    if (slot < 0 || slot >= (int)items.size()) return;
    delete items[slot];
    items[slot] = nullptr;

    compressLinkedSlotList(0);
}

bool FillingContainer::linkEmptySlot(int inventorySlot) {
    for (int i = 0; i < numLinkedSlots; ++i)
        if (linkedSlots[i].inventorySlot == inventorySlot) return true;
    for (int i = 0; i < numLinkedSlots; ++i)
        if (!getLinked(i)) { linkedSlots[i].inventorySlot = inventorySlot; return true; }
    return false;
}

void FillingContainer::compressLinkedSlotList(int slot) {
    int i = slot - 1, j = 0;
    while (++i < numLinkedSlots) {
        linkedSlots[i - j] = linkedSlots[i];
        if (!getLinked(i)) ++j;
    }
    for (int k = i - j; k < i; ++k)
        linkedSlots[k].inventorySlot = -1;
}

void FillingContainer::clearInventory() {
    for (int i = 0; i < (int)items.size(); i++) {
        delete items[i];
        items[i] = nullptr;
    }
}

int FillingContainer::getFreeSlot() const {
    for (int i = 0; i < (int)items.size(); i++)
        if (!items[i]) return i;
    return -1;
}

void FillingContainer::reconfigure(int newTotalSlots, bool creative) {
    clearInventory();
    items.assign(newTotalSlots, nullptr);
    numTotalSlots = newTotalSlots;
    _isCreative   = creative;
    for (int i = 0; i < numLinkedSlots; i++) linkedSlots[i].inventorySlot = -1;
}

int FillingContainer::addItem(ItemInstance* item) {
    int slot = getFreeSlot();
    if (slot < 0) { delete item; return -1; }
    items[slot] = item;
    return slot;
}

int FillingContainer::addResource(ItemInstance* item) {
    int max = item->getMaxStackSize();
    if (max < 1) max = 1;
    for (int i = 0; i < (int)items.size() && item->count > 0; i++) {
        ItemInstance* pile = items[i];
        if (pile && pile->matches(item) && pile->count < max) {
            int space = max - pile->count;
            int moved = item->count < space ? item->count : space;
            pile->count += moved;
            item->count -= moved;
            linkEmptySlot(i);
        }
    }
    return item->count;
}

bool FillingContainer::add(ItemInstance* item) {
    if (!item || item->isNull()) { delete item; return true; }
    if (_isCreative) { delete item; return true; }

    if (item->isStackable()) addResource(item);

    while (item->count > 0) {
        int slot = getFreeSlot();
        if (slot < 0) return false;
        int max = item->getMaxStackSize();
        if (max < 1) max = 1;
        int moved = item->count < max ? item->count : max;
        items[slot] = new ItemInstance(item->id, (short)moved, item->data);
        linkEmptySlot(slot);
        item->count -= moved;
    }
    delete item;
    return true;
}

int FillingContainer::removeResource(const ItemInstance& item, bool requireExactAux) {
    if (_isCreative) return 0;

    bool anyAux = !requireExactAux &&
                  (Recipe::isAnyAuxValue(&item) || item.data == Recipe::ANY_AUX_VALUE);
    int count = item.count;
    while (count > 0) {
        int slot = -1;
        for (int i = 0; i < (int)items.size(); i++) {
            ItemInstance* it = items[i];
            if (it && it->id == item.id && it->count > 0 &&
                (anyAux || it->data == item.data)) { slot = i; break; }
        }
        if (slot < 0) return count;

        ItemInstance* slotItem = items[slot];
        int toRemove = count < slotItem->count ? count : slotItem->count;
        slotItem->count -= toRemove;
        count -= toRemove;
        if (slotItem->count <= 0) clearSlot(slot);
    }
    return 0;
}

bool FillingContainer::linkSlot(int selectionSlot, int inventorySlot) {
    if (selectionSlot < 0 || selectionSlot >= numLinkedSlots) return false;
    linkedSlots[selectionSlot].inventorySlot = inventorySlot;
    return true;
}

ItemInstance* FillingContainer::getLinked(int selectionSlot) {
    if (selectionSlot < 0 || selectionSlot >= numLinkedSlots) return nullptr;
    return getItem(linkedSlots[selectionSlot].inventorySlot);
}
