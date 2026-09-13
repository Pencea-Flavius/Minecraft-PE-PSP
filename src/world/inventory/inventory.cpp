#include "world/inventory/inventory.h"
#include "world/item/item.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"

static const short kStarter[Inventory::HOTBAR] = {
    BLOCK_STONE, BLOCK_COBBLESTONE, BLOCK_BRICKS, BLOCK_TORCH, BLOCK_DIRT, BLOCK_PLANKS,
    BLOCK_STAIRS_BRICK, BLOCK_LOG,
};
static const int kStarterCount = (int)(sizeof(kStarter) / sizeof(kStarter[0]));
static_assert(kStarterCount == Inventory::HOTBAR, "starter set must fill the hotbar");

static const short CREATIVE_STACK = 5;

Inventory::Inventory(bool creative)
    : FillingContainer(SURVIVAL_SLOTS + HOTBAR, HOTBAR, ContainerType::INVENTORY, creative),
      selected(0) {
    setupDefault();
}

void Inventory::setupDefault() {
    clearInventory();
    selected = 0;

    if (!_isCreative) return;

    for (int i = 0; i < kStarterCount; i++) {
        int slot = i + firstGridSlot();
        ItemInstance stack(kStarter[i], CREATIVE_STACK, 0);
        setItem(slot, &stack);
        linkSlot(i, slot);
    }
}

void Inventory::reinit(bool creative) {
    reconfigure(SURVIVAL_SLOTS + HOTBAR, creative);
    selected = 0;
    setupDefault();
}

void Inventory::consumeSelected() {
    if (_isCreative) return;
    ItemInstance* held = getSelected();
    if (!held || held->isNull()) return;
    if (--held->count <= 0) clearSlot(selected);
}

ItemInstance Inventory::removeSelected(int n) {
    if (_isCreative) return ItemInstance();
    ItemInstance* held = getSelected();
    if (!held || held->isNull()) return ItemInstance();
    ItemInstance piece = held->remove(n);
    if (held->count <= 0) clearSlot(selected);
    return piece;
}

bool Inventory::replaceSelected(short id, unsigned char data) {
    if (_isCreative) return false;
    ItemInstance* held = getSelected();
    if (!held || held->isNull()) return false;
    *held = ItemInstance(id, 1, (short)data);
    return true;
}

bool Inventory::hurtSelected(int amount) {
    if (_isCreative) return false;
    ItemInstance* held = getSelected();
    if (!held || held->isNull()) return false;
    Item* it = Item::items[held->id];
    if (!it || it->maxDamage <= 0) return false;
    held->hurt(amount);

    if (held->count <= 0) {
        clearSlot(selected);

        if (g_level.player)
            g_level.playSound(g_level.player, "random.break", 1.0f, 0.9f);
        return true;
    }
    return false;
}

int Inventory::getAttackDamage(Entity* target) {
    ItemInstance* sel = getSelected();
    if (sel && !sel->isNull() && sel->getItem()) return sel->getItem()->getAttackDamage(target);
    return 1;
}

void Inventory::ensureHotbar(short id, short data) {
    for (int i = 0; i < HOTBAR; i++) {
        ItemInstance* h = getLinked(i);
        if (h && h->id == id && h->data == data) return;
    }
    int invSlot = -1;
    for (int s = 0; s < numTotalSlots; s++) {
        ItemInstance* it = getItem(s);
        if (it && it->id == id && it->data == data) { invSlot = s; break; }
    }
    if (invSlot < 0) return;
    for (int i = 0; i < HOTBAR; i++) {
        int ps = linkedSlots[i].inventorySlot;
        if (ps < 0 || !getItem(ps)) { linkSlot(i, invSlot); return; }
    }

}

int Inventory::getLinkedSlotForItem(int invSlot) const {
    if (invSlot < 0) return -1;
    for (int i = 0; i < numLinkedSlots; i++)
        if (linkedSlots[i].inventorySlot == invSlot) return i;
    return -1;
}

int Inventory::survivalAddItem(int gridIndex) {
    int invSlot = gridIndex + firstGridSlot();
    if (!getItem(invSlot)) return selected;

    int lsfi = getLinkedSlotForItem(invSlot);
    if (lsfi < 0 || lsfi >= HOTBAR) linkSlot(selected, invSlot);
    else                            selected = lsfi;
    return selected;
}

int g_classicPick = 0;

int Inventory::ownableGridSlot(int h) {
    int mine = getLinkedSlot(h);
    if (mine >= firstGridSlot()) return mine;

    const int upstream = h + firstGridSlot();
    if (getLinkedSlotForItem(upstream) < 0) return upstream;

    for (int s = firstGridSlot(); s < firstGridSlot() + SURVIVAL_SLOTS; s++)
        if (getLinkedSlotForItem(s) < 0) return s;
    return upstream;
}

int Inventory::classicSurvivalAddItem(int gridIndex) {
    int invSlot = gridIndex + firstGridSlot();
    if (!getItem(invSlot)) return selected;
    linkSlot(0, invSlot, true);
    selected = 0;
    return 0;
}

int Inventory::classicCreativeAddItem(short id, short count, short aux) {
    int hot = getLinkedSlotForItemAndAux(id, aux);
    int invSlot = (hot >= 0 && hot < HOTBAR) ? getLinkedSlot(hot) : -1;

    if (invSlot < 0) {

        invSlot = ownableGridSlot(HOTBAR - 1);
        ItemInstance stack(id, count, aux);
        setItem(invSlot, &stack);
    }

    linkSlot(0, invSlot, true);
    selected = 0;
    return 0;
}

int Inventory::getLinkedSlotForItemAndAux(short id, short aux) const {
    for (int i = 0; i < numLinkedSlots; i++) {
        const ItemInstance* linked = const_cast<Inventory*>(this)->getLinked(i);
        if (linked && linked->id == id && linked->data == aux) return i;
    }
    return -1;
}

int Inventory::creativeAddItem(short id, short count, short aux) {
    int slot = getLinkedSlotForItemAndAux(id, aux);
    if (slot >= 0 && slot < HOTBAR) {
        selected = slot;
        return selected;
    }
    int inv = ownableGridSlot(selected);
    ItemInstance stack(id, count, aux);
    setItem(inv, &stack);
    linkSlot(selected, inv);

    setItem(selected, &stack);
    return selected;
}
