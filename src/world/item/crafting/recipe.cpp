
#include "world/item/item.h"
#include "world/item/crafting/recipe.h"
#include "world/level/chunk/chunk.h"

void ItemPack::add(int id, int count ) {
    Map::iterator it = items.find(id);
    if (it == items.end()) items.insert(std::make_pair(id, count));
    else                   it->second += count;
}

int ItemPack::getCount(int id) const {
    Map::const_iterator it = items.find(id);
    if (it == items.end()) return 0;
    return it->second;
}

int ItemPack::getMaxMultipliesOf(const ItemPack& v) const {
    if (v.items.empty()) return 0;
    int minCount = 99;

    Map::const_iterator it = v.items.begin();
    while (it != v.items.end()) {
        if (it->first <= 0) { ++it; continue; }

        Map::const_iterator jt = items.find(it->first);
        if (jt == items.end()) return 0;
        if (it->second == 0)   return 0;

        int count = jt->second / it->second;
        if (count == 0) return 0;
        if (count < minCount) minCount = count;
        ++it;
    }
    return minCount;
}

std::vector<ItemInstance> ItemPack::getItemInstances() const {
    std::vector<ItemInstance> out;
    Map::const_iterator it = items.begin();
    while (it != items.end()) {
        ItemInstance item = getItemInstanceForId(it->first);
        item.count = (short)it->second;
        out.push_back(item);
        ++it;
    }
    return out;
}

int ItemPack::getIdForItemInstance(const ItemInstance* ii) {
    if (Recipe::isAnyAuxValue(ii)) return ii->id * 512 - 1;

    Item* it = Item::items[ii->id];
    if (it && it->isTool()) return ii->id * 512 - 1;
    return ii->id * 512 + ii->data;
}

ItemInstance ItemPack::getItemInstanceForId(int id) {
    id += 256;
    return ItemInstance((short)(id / 512), 1, (short)((id & 511) - 256));
}

bool Recipe::isAnyAuxValue(int id) {
    bool isTile = id < 256;
    if (!isTile) return false;
    if (id == BLOCK_WOOL || id == BLOCK_SLAB || id == BLOCK_SANDSTONE)
        return false;
    return true;
}
