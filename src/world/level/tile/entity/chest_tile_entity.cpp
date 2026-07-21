#include "world/level/tile/entity/chest_tile_entity.h"
#include "world/item/item_instance.h"
#include "nbt/compound_tag.h"
#include "nbt/list_tag.h"

ChestTileEntity::ChestTileEntity()
    : super(TE_CHEST),
      container(CONTAINER_SIZE, 0, ContainerType::CONTAINER, false) {
    rendererId = TR_NO_RENDER;
}

bool ChestTileEntity::save(CompoundTag* tag) {
    if (!super::save(tag)) return false;
    ListTag* items = new ListTag("Items");
    for (int i = 0; i < CONTAINER_SIZE; i++) {
        ItemInstance* it = container.getItem(i);
        if (it && !it->isNull()) {
            CompoundTag* c = new CompoundTag();
            c->putByte("Slot", (char)i);
            c->putShort("id", it->id);
            c->putByte("Count", (char)it->count);
            c->putShort("Damage", it->data);
            items->add(c);
        }
    }
    tag->put("Items", items);
    return true;
}

void ChestTileEntity::load(CompoundTag* tag) {
    super::load(tag);
    container.clearInventory();
    ListTag* items = tag->getList("Items");
    for (int i = 0; i < items->size(); i++) {
        CompoundTag* c = (CompoundTag*)items->get(i);
        if (!c) continue;
        int slot = (unsigned char)c->getByte("Slot");
        if (slot < 0 || slot >= CONTAINER_SIZE) continue;
        container.setItem(slot, new ItemInstance(
            c->getShort("id"),
            (unsigned char)c->getByte("Count"),
            c->getShort("Damage")));
    }
}
