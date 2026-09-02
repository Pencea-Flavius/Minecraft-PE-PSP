
#include "world/level/tile/entity/furnace_tile_entity.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/material.h"
#include "world/item/item.h"
#include "nbt/compound_tag.h"
#include "nbt/list_tag.h"

FurnaceTileEntity::FurnaceTileEntity()
    : super(TE_FURNACE), Container(ContainerType::FURNACE),
      litTime(0), litDuration(0), tickCount(0) {
    rendererId = TR_NO_RENDER;
}

ItemInstance* FurnaceTileEntity::getItem(int slot) {
    if (slot < 0 || slot >= NUM_ITEMS) return 0;
    return &items[slot];
}

void FurnaceTileEntity::setItem(int slot, ItemInstance* item) {
    if (!item || slot < 0 || slot >= NUM_ITEMS) return;
    items[slot] = *item;
    if (items[slot].count > getMaxStackSize()) items[slot].count = (short)getMaxStackSize();
}

ItemInstance FurnaceTileEntity::removeItem(int slot, int count) {
    if (slot < 0 || slot >= NUM_ITEMS) return ItemInstance();
    if (items[slot].isNull()) return ItemInstance();
    if (items[slot].count <= count) {
        ItemInstance item = items[slot];
        items[slot].setNull();
        return item;
    }
    ItemInstance i = items[slot].remove(count);
    if (items[slot].count == 0) items[slot].setNull();
    return i;
}

int FurnaceTileEntity::getBurnDuration(const ItemInstance& fuel) {
    if (fuel.isNull()) return 0;
    short id = fuel.id;
    if (id > 0 && id < 256 && &materialOf((unsigned char)id) == &Material::wood)
        return BURN_INTERVAL * 3 / 2;
    if (id == ITEM_STICK) return BURN_INTERVAL / 2;

    if (id == BLOCK_SAPLING) return BURN_INTERVAL / 2;
    if (id == ITEM_COAL)  return BURN_INTERVAL * 8;

    if (id == BLOCK_COAL_BLOCK) return BURN_INTERVAL * 80;

    if (id == ITEM_BUCKET && fuel.data == BLOCK_LAVA) return BURN_INTERVAL * 100;
    return 0;
}

bool FurnaceTileEntity::isFuel(const ItemInstance& item) {
    return getBurnDuration(item) > 0;
}

bool FurnaceTileEntity::isFurnaceItem(const ItemInstance& item) {
    return !item.isNull() && !furnaceResult(item.id).isNull();
}

bool FurnaceTileEntity::isSlotEmpty(int slot) const {
    return items[slot].isNull();
}

ItemInstance FurnaceTileEntity::furnaceResult(short ingredientId) {
    switch (ingredientId) {
        case BLOCK_ORE_IRON:      return ItemInstance(ITEM_IRON_INGOT, 1, 0);
        case BLOCK_ORE_GOLD:      return ItemInstance(ITEM_GOLD_INGOT, 1, 0);
        case BLOCK_ORE_EMERALD:   return ItemInstance(ITEM_DIAMOND, 1, 0);
        case BLOCK_SAND:          return ItemInstance(BLOCK_GLASS, 1, 0);
        case ITEM_PORKCHOP_RAW:   return ItemInstance(ITEM_PORKCHOP_COOKED, 1, 0);
        case ITEM_BEEF_RAW:       return ItemInstance(ITEM_BEEF_COOKED, 1, 0);
        case ITEM_CHICKEN_RAW:    return ItemInstance(ITEM_CHICKEN_COOKED, 1, 0);

        case ITEM_POTATO:         return ItemInstance(ITEM_POTATO_BAKED, 1, 0);
        case BLOCK_COBBLESTONE:   return ItemInstance(BLOCK_STONE, 1, 0);
        case ITEM_CLAY:           return ItemInstance(ITEM_BRICK, 1, 0);
        case BLOCK_CACTUS:        return ItemInstance(ITEM_BONEMEAL, 1, 2);
        case BLOCK_MUSHROOM_RED:  return ItemInstance(ITEM_BONEMEAL, 1, 1);
        case BLOCK_LOG:           return ItemInstance(ITEM_COAL, 1, 1);
        case BLOCK_NETHERRACK:    return ItemInstance(ITEM_NETHER_BRICK, 1, 0);
        default:                  return ItemInstance();
    }
}

bool FurnaceTileEntity::canBurn() const {
    if (items[SLOT_INGREDIENT].isNull()) return false;
    ItemInstance result = furnaceResult(items[SLOT_INGREDIENT].id);
    if (result.isNull()) return false;
    const ItemInstance& out = items[SLOT_RESULT];
    if (out.isNull()) return true;
    if (out.id != result.id || out.data != result.data) return false;
    return out.count + result.count <= out.getMaxStackSize();
}

void FurnaceTileEntity::burn() {
    if (!canBurn()) return;
    ItemInstance result = furnaceResult(items[SLOT_INGREDIENT].id);
    if (items[SLOT_RESULT].isNull()) items[SLOT_RESULT] = result;
    else                             items[SLOT_RESULT].count += result.count;
    if (--items[SLOT_INGREDIENT].count <= 0) items[SLOT_INGREDIENT].setNull();
}

void FurnaceTileEntity::tick() {
    bool wasLit = litTime > 0;
    if (litTime > 0) --litTime;

    if (litTime == 0 && canBurn()) {
        litDuration = litTime = getBurnDuration(items[SLOT_FUEL]);
        if (litTime > 0) {

            items[SLOT_FUEL].useAsFuel();
        }
    }
    if (isLit() && canBurn()) {
        if (++tickCount >= BURN_INTERVAL) { tickCount = 0; burn(); }
    } else {
        tickCount = 0;
    }
    if (wasLit != (litTime > 0))
        furnaceSetLitBlock(level, x, y, z, litTime > 0);
}

bool FurnaceTileEntity::shouldSave() {
    if (litTime > 0) return true;
    for (int i = 0; i < NUM_ITEMS; ++i)
        if (!items[i].isNull()) return true;
    return false;
}

bool FurnaceTileEntity::save(CompoundTag* tag) {
    if (!super::save(tag)) return false;
    ListTag* list = new ListTag("Items");
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (items[i].isNull()) continue;
        CompoundTag* c = new CompoundTag();
        c->putByte("Slot", (char)i);
        c->putShort("id", items[i].id);
        c->putByte("Count", (char)items[i].count);
        c->putShort("Damage", items[i].data);
        list->add(c);
    }
    tag->put("Items", list);
    tag->putShort("BurnTime", (short)litTime);
    tag->putShort("CookTime", (short)tickCount);
    return true;
}

void FurnaceTileEntity::load(CompoundTag* tag) {
    super::load(tag);
    for (int i = 0; i < NUM_ITEMS; i++) items[i].setNull();
    ListTag* list = tag->getList("Items");
    for (int i = 0; i < list->size(); i++) {
        CompoundTag* c = (CompoundTag*)list->get(i);
        if (!c) continue;
        int slot = (unsigned char)c->getByte("Slot");
        if (slot < 0 || slot >= NUM_ITEMS) continue;
        items[slot] = ItemInstance(c->getShort("id"),
                                   (short)(unsigned char)c->getByte("Count"),
                                   c->getShort("Damage"));
    }
    litTime = tag->getShort("BurnTime");
    tickCount = tag->getShort("CookTime");
    litDuration = getBurnDuration(items[SLOT_FUEL]);
}
