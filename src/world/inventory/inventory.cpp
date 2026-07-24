#include "world/inventory/inventory.h"
#include "world/item/item.h"

static const short kPalette[] = {

    BLOCK_COBBLESTONE, BLOCK_STONE_BRICKS, BLOCK_STONE_BRICKS, BLOCK_STONE_BRICKS, BLOCK_MOSSY_COBBLE, BLOCK_PLANKS, BLOCK_BRICKS,
    BLOCK_STONE, BLOCK_DIRT, BLOCK_GRASS, BLOCK_CLAY,
    BLOCK_SANDSTONE, BLOCK_SANDSTONE, BLOCK_SANDSTONE,
    BLOCK_SAND, BLOCK_GRAVEL,
    BLOCK_LOG, BLOCK_LOG, BLOCK_LOG,
    BLOCK_NETHER_BRICK, BLOCK_NETHERRACK,

    BLOCK_STAIRS_COBBLESTONE, BLOCK_STAIRS_PLANKS, BLOCK_STAIRS_BRICK, BLOCK_STAIRS_SANDSTONE,
    BLOCK_STAIRS_STONE_BRICK, BLOCK_STAIRS_NETHER_BRICK, BLOCK_STAIRS_QUARTZ,

    BLOCK_SLAB, BLOCK_SLAB, BLOCK_SLAB, BLOCK_SLAB,
    BLOCK_SLAB, BLOCK_SLAB,

    BLOCK_QUARTZ_BLOCK, BLOCK_QUARTZ_BLOCK, BLOCK_QUARTZ_BLOCK,

    BLOCK_ORE_COAL, BLOCK_ORE_IRON, BLOCK_ORE_GOLD,
    BLOCK_ORE_EMERALD, BLOCK_ORE_LAPIS, BLOCK_ORE_REDSTONE,

    BLOCK_GOLD_BLOCK, BLOCK_IRON_BLOCK, BLOCK_DIAMOND_BLOCK, BLOCK_LAPIS_BLOCK,
    BLOCK_OBSIDIAN, BLOCK_SNOW_BLOCK, BLOCK_GLASS, BLOCK_GLOWSTONE, BLOCK_NETHER_REACTOR,

    BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL,
    BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL, BLOCK_WOOL,

    BLOCK_LADDER,
    BLOCK_TORCH,
    BLOCK_GLASS_PANE,
    ITEM_DOOR_WOOD_ITEM, BLOCK_TRAPDOOR, BLOCK_FENCE, BLOCK_FENCE_GATE,

    ITEM_BED_ITEM,
    BLOCK_BOOKSHELF,
    ITEM_PAINTING,
    BLOCK_CRAFTING_TABLE, BLOCK_STONECUTTER, BLOCK_CHEST, BLOCK_FURNACE, BLOCK_TNT,

    BLOCK_FLOWER, BLOCK_ROSE, BLOCK_MUSHROOM_BROWN, BLOCK_MUSHROOM_RED,
    BLOCK_CACTUS, BLOCK_MELON, ITEM_REEDS, BLOCK_COBWEB,
    BLOCK_SAPLING, BLOCK_SAPLING, BLOCK_SAPLING,
    BLOCK_LEAVES, BLOCK_LEAVES, BLOCK_LEAVES,

    ITEM_SEEDS_WHEAT, ITEM_SEEDS_MELON, ITEM_BONEMEAL,
    ITEM_HOE_IRON, ITEM_SWORD_IRON, ITEM_BOW, ITEM_SIGN,
    ITEM_FLINT_AND_STEEL,
    ITEM_CAMERA,

    ITEM_SPAWN_EGG, ITEM_SPAWN_EGG, ITEM_SPAWN_EGG, ITEM_SPAWN_EGG,
    ITEM_SPAWN_EGG, ITEM_SPAWN_EGG, ITEM_SPAWN_EGG, ITEM_SPAWN_EGG, ITEM_SPAWN_EGG,
};
static const unsigned char kPaletteData[] = {

    0, SB_NORMAL, SB_MOSSY, SB_CRACKED, 0, 0, 0,
    0, 0, 0, 0,
    SS_DEFAULT, SS_CHISELED, SS_SMOOTH,
    0, 0,
    LOG_OAK, LOG_SPRUCE, LOG_BIRCH,
    0, 0,
    0, 0, 0, 0, 0, 0, 0,

    DSLAB_STONE, DSLAB_COBBLE, DSLAB_WOOD, DSLAB_BRICK, DSLAB_SAND, DSLAB_SMOOTHBRICK,
    QZ_DEFAULT, QZ_PILLAR, QZ_CHISELED,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 7, 6, 5, 4, 3, 2, 1,
    15, 14, 13, 12, 11, 10, 9, 8,
    0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2,
    0, 1, 2,
    0, 0, 15,
    0, 0, 0, 0,
    0,
    0,
    12, 11, 10, 13,
    32, 33, 34, 35, 36,
};
static const int kPaletteCount = (int)(sizeof(kPalette) / sizeof(kPalette[0]));
static_assert((sizeof(kPaletteData) / sizeof(kPaletteData[0])) == (sizeof(kPalette) / sizeof(kPalette[0])),
              "palette data must align with palette");

static const short kStarter[Inventory::HOTBAR] = {
    BLOCK_STONE, BLOCK_COBBLESTONE, BLOCK_BRICKS, BLOCK_TORCH, BLOCK_DIRT, BLOCK_PLANKS,
    BLOCK_STAIRS_BRICK,
};
static_assert((sizeof(kStarter) / sizeof(kStarter[0])) == Inventory::HOTBAR,
              "starter set must fill the hotbar");

Inventory::Inventory(bool creative)
    : FillingContainer(MAX_SLOTS + HOTBAR, HOTBAR, ContainerType::INVENTORY, creative),
      selected(0), mainCount(0) {
    setupDefault();
}

void Inventory::setupDefault() {
    clearInventory();
    selected = 0;
    mainCount = 0;

    if (!_isCreative) return;

    for (int i = 0; i < kPaletteCount && i < MAX_SLOTS; i++) {
        addItem(new ItemInstance(kPalette[i], 1, kPaletteData[i]));
        mainCount = i + 1;
    }

    for (int i = 0; i < HOTBAR; i++) {
        int found = firstGridSlot();
        for (int s = 0; s < mainCount; s++) {
            ItemInstance* it = gridItem(s);
            if (it && it->id == kStarter[i] && it->data == 0) { found = s + firstGridSlot(); break; }
        }
        linkSlot(i, found);
    }
}

void Inventory::reinit(bool creative) {
    reconfigure((creative ? MAX_SLOTS : SURVIVAL_SLOTS) + HOTBAR, creative);
    selected = 0;
    mainCount = 0;
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

void Inventory::setSelectedIfEmpty(short id, unsigned char data) {
    if (_isCreative) return;
    ItemInstance* held = getSelected();
    if (held && !held->isNull()) return;
    ItemInstance* stack = new ItemInstance(id, 1, data);
    if (!add(stack)) delete stack;
}

bool Inventory::hurtSelected(int amount) {
    if (_isCreative) return false;
    ItemInstance* held = getSelected();
    if (!held || held->isNull()) return false;
    Item* it = Item::items[held->id];
    if (!it || it->maxDamage <= 0) return false;
    held->hurt(amount);

    if (held->count <= 0) { clearSlot(selected); return true; }
    return false;
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

bool Inventory::linkHotbarTo(int slot, short id, unsigned char data) {
    if (slot < 0 || slot >= HOTBAR) return false;

    for (int s = 0; s < mainCount; s++) {
        ItemInstance* it = gridItem(s);
        if (it && it->id == id && it->data == data) return linkSlot(slot, s + firstGridSlot());
    }
    return false;
}

void Inventory::pickToHotbar(int gridIndex) {
    int invSlot = gridIndex + firstGridSlot();
    if (!getItem(invSlot)) return;
    if (linkedSlots[0].inventorySlot == invSlot) { selected = 0; return; }

    int i = 0;
    for (; i < HOTBAR - 1; i++)
        if (linkedSlots[i].inventorySlot == invSlot) break;
    for (; i > 0; i--)
        linkedSlots[i].inventorySlot = linkedSlots[i - 1].inventorySlot;
    linkedSlots[0].inventorySlot = invSlot;
    selected = 0;
}
