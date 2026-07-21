#include "world/item/item_instance.h"
#include "world/item/item.h"

Item* ItemInstance::getItem() const {
    if (id >= 0 && id < 4096) return Item::items[id];
    return nullptr;
}

int ItemInstance::getMaxStackSize() const {
    Item* it = getItem();
    return it ? it->maxStackSize : 64;
}

void ItemInstance::hurt(int amount) {
    Item* it = getItem();
    if (!it || it->maxDamage <= 0) return;
    data += amount;
    if (data > it->maxDamage) { count--; data = 0; }
}

ItemInstance ItemInstance::remove(int n) {
    if (n > count) n = count;
    ItemInstance out(id, (short)n, data);
    count -= n;
    if (count <= 0) setNull();
    return out;
}
