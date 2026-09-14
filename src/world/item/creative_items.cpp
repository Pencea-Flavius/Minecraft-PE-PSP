#include "world/item/creative_items.h"
#include "world/item/item.h"
#include "world/level/chunk/chunk.h"
#include "world/entity/entity_types.h"

namespace {

const CreativeEntry kItems[] = {

    { BLOCK_RAIL,               1, 0 },
    { BLOCK_GOLDEN_RAIL,        1, 0 },
    { BLOCK_COBBLESTONE,        1, 0 },
    { BLOCK_STONE_BRICKS,       1, SB_NORMAL },
    { BLOCK_STONE_BRICKS,       1, SB_MOSSY },
    { BLOCK_STONE_BRICKS,       1, SB_CRACKED },
    { BLOCK_MOSSY_COBBLE,       1, 0 },

    { BLOCK_PLANKS,             1, PLANK_OAK },
    { BLOCK_PLANKS,             1, PLANK_SPRUCE },
    { BLOCK_PLANKS,             1, PLANK_BIRCH },
    { BLOCK_PLANKS,             1, PLANK_JUNGLE },
    { BLOCK_BRICKS,             1, 0 },
    { BLOCK_STONE,              1, 0 },
    { BLOCK_DIRT,               1, 0 },
    { BLOCK_GRASS,              1, 0 },
    { BLOCK_CLAY,               1, 0 },
    { BLOCK_SANDSTONE,          5, SS_DEFAULT },
    { BLOCK_SANDSTONE,          5, SS_CHISELED },
    { BLOCK_SANDSTONE,          5, SS_SMOOTH },
    { BLOCK_SAND,               1, 0 },
    { BLOCK_GRAVEL,             1, 0 },
    { BLOCK_COBBLE_WALL,        1, WALL_COBBLE },
    { BLOCK_COBBLE_WALL,        5, WALL_MOSSY },

    { BLOCK_LOG,                5, LOG_OAK },
    { BLOCK_LOG,                5, LOG_SPRUCE },
    { BLOCK_LOG,                5, LOG_BIRCH },
    { BLOCK_LOG,                5, LOG_JUNGLE },
    { BLOCK_NETHER_BRICK,       1, 0 },
    { BLOCK_NETHERRACK,         1, 0 },
    { BLOCK_BEDROCK,            1, 0 },
    { BLOCK_STAIRS_COBBLESTONE, 1, 0 },
    { BLOCK_STAIRS_PLANKS,      1, 0 },
    { BLOCK_STAIRS_SPRUCE,      1, 0 },
    { BLOCK_STAIRS_BIRCH,       1, 0 },
    { BLOCK_STAIRS_JUNGLE,      1, 0 },
    { BLOCK_STAIRS_BRICK,       1, 0 },
    { BLOCK_STAIRS_SANDSTONE,   1, 0 },
    { BLOCK_STAIRS_STONE_BRICK, 1, 0 },
    { BLOCK_STAIRS_NETHER_BRICK,1, 0 },
    { BLOCK_STAIRS_QUARTZ,      1, 0 },

    { BLOCK_SLAB,               1, DSLAB_STONE },
    { BLOCK_SLAB,               1, DSLAB_COBBLE },
    { BLOCK_WOOD_SLAB,          1, PLANK_OAK },
    { BLOCK_WOOD_SLAB,          1, PLANK_SPRUCE },
    { BLOCK_WOOD_SLAB,          1, PLANK_BIRCH },
    { BLOCK_WOOD_SLAB,          1, PLANK_JUNGLE },
    { BLOCK_SLAB,               1, DSLAB_BRICK },
    { BLOCK_SLAB,               1, DSLAB_SAND },
    { BLOCK_SLAB,               1, DSLAB_SMOOTHBRICK },
    { BLOCK_SLAB,               1, DSLAB_QUARTZ },
    { BLOCK_QUARTZ_BLOCK,       1, QZ_DEFAULT },
    { BLOCK_QUARTZ_BLOCK,       1, QZ_PILLAR },
    { BLOCK_QUARTZ_BLOCK,       1, QZ_CHISELED },
    { BLOCK_ORE_COAL,           1, 0 },
    { BLOCK_ORE_IRON,           1, 0 },
    { BLOCK_ORE_GOLD,           1, 0 },
    { BLOCK_ORE_DIAMOND,        1, 0 },
    { BLOCK_ORE_LAPIS,          1, 0 },
    { BLOCK_ORE_REDSTONE,       1, 0 },
    { BLOCK_GOLD_BLOCK,         1, 0 },
    { BLOCK_IRON_BLOCK,         1, 0 },
    { BLOCK_DIAMOND_BLOCK,      1, 0 },
    { BLOCK_LAPIS_BLOCK,        1, 0 },
    { BLOCK_COAL_BLOCK,         1, 0 },
    { BLOCK_OBSIDIAN,           1, 0 },

    { BLOCK_GLOWING_OBSIDIAN,   1, 0 },
    { BLOCK_ICE,                1, 0 },
    { BLOCK_SNOW_BLOCK,         1, 0 },
    { BLOCK_TOPSNOW,            1, 0 },
    { BLOCK_GLASS,              1, 0 },
    { BLOCK_GLOWSTONE,          1, 0 },
    { BLOCK_NETHER_REACTOR,     1, 0 },
    { BLOCK_LADDER,             1, 0 },
    { BLOCK_SPONGE,             1, 0 },
    { BLOCK_TORCH,              1, 0 },
    { BLOCK_GLASS_PANE,         1, 0 },

    { ITEM_BUCKET,              1, 0 },
    { ITEM_BUCKET,              1, BLOCK_WATER },
    { ITEM_BUCKET,              1, BLOCK_LAVA },
    { ITEM_DOOR_WOOD_ITEM,      1, 0 },
    { BLOCK_TRAPDOOR,           1, 0 },
    { BLOCK_FENCE,              1, 0 },
    { BLOCK_FENCE_GATE,         1, 0 },
    { BLOCK_IRON_BARS,          1, 0 },
    { ITEM_BED_ITEM,            1, 0 },
    { BLOCK_BOOKSHELF,          1, 0 },
    { ITEM_PAINTING,            1, 0 },
    { BLOCK_CRAFTING_TABLE,     1, 0 },
    { BLOCK_STONECUTTER,        1, 0 },
    { BLOCK_CHEST,              1, 0 },
    { BLOCK_FURNACE,            1, 0 },
    { BLOCK_TNT,                1, 0 },
    { BLOCK_FLOWER,             1, 0 },
    { BLOCK_ROSE,               1, 0 },
    { BLOCK_MUSHROOM_BROWN,     1, 0 },
    { BLOCK_MUSHROOM_RED,       1, 0 },
    { BLOCK_CACTUS,             1, 0 },
    { BLOCK_MELON,              1, 0 },
    { BLOCK_PUMPKIN,            1, 0 },
    { BLOCK_PUMPKIN_LIT,        1, 0 },
    { BLOCK_COBWEB,             1, 0 },
    { BLOCK_HAY_BLOCK,          1, 0 },
    { ITEM_REEDS,               1, 0 },
    { ITEM_WHEAT,               1, 0 },

    { BLOCK_TALLGRASS,          5, TG_TALL_GRASS },
    { BLOCK_TALLGRASS,          5, TG_FERN },
    { BLOCK_TALLGRASS,          1, TG_DEAD_SHRUB },
    { BLOCK_SAPLING,            1, LEAF_OAK },
    { BLOCK_SAPLING,            1, LEAF_SPRUCE },
    { BLOCK_SAPLING,            1, LEAF_BIRCH },

    { BLOCK_SAPLING,            1, LEAF_JUNGLE },
    { BLOCK_LEAVES,             1, LEAF_OAK },
    { BLOCK_LEAVES,             1, LEAF_SPRUCE },
    { BLOCK_LEAVES,             1, LEAF_BIRCH },
    { BLOCK_LEAVES,             1, LEAF_JUNGLE },
    { ITEM_SEEDS_WHEAT,         1, 0 },
    { ITEM_SEEDS_PUMPKIN,       1, 0 },
    { ITEM_SEEDS_MELON,         1, 0 },

    { ITEM_CARROT,              1, 0 },
    { ITEM_POTATO,              1, 0 },
    { ITEM_SEEDS_BEETROOT,      1, 0 },
    { ITEM_HOE_IRON,            1, 0 },
    { ITEM_CAKE,                1, 0 },
    { ITEM_EGG,                 1, 0 },
    { ITEM_SWORD_IRON,          1, 0 },
    { ITEM_BOW,                 1, 0 },
    { ITEM_FLINT_AND_STEEL,     1, 0 },
    { ITEM_SHEARS,              1, 0 },
    { ITEM_SIGN,                1, 0 },

    { ITEM_CLOCK,               1, 0 },
    { ITEM_COMPASS,             1, 0 },

    { ITEM_MINECART,            1, 0 },

    { ITEM_SPAWN_EGG,           1, EntityTypes::IdChicken },
    { ITEM_SPAWN_EGG,           1, EntityTypes::IdCow },
    { ITEM_SPAWN_EGG,           1, EntityTypes::IdPig },
    { ITEM_SPAWN_EGG,           1, EntityTypes::IdSheep },

    { BLOCK_WOOL, 1, 0x0 }, { BLOCK_WOOL, 1, 0x8 }, { BLOCK_WOOL, 1, 0x7 }, { BLOCK_WOOL, 1, 0xF },
    { BLOCK_WOOL, 1, 0xC }, { BLOCK_WOOL, 1, 0xE }, { BLOCK_WOOL, 1, 0x1 }, { BLOCK_WOOL, 1, 0x4 },
    { BLOCK_WOOL, 1, 0x5 }, { BLOCK_WOOL, 1, 0xD }, { BLOCK_WOOL, 1, 0x9 }, { BLOCK_WOOL, 1, 0x3 },
    { BLOCK_WOOL, 1, 0xB }, { BLOCK_WOOL, 1, 0xA }, { BLOCK_WOOL, 1, 0x2 }, { BLOCK_WOOL, 1, 0x6 },
    { BLOCK_CARPET, 1, 0x0 }, { BLOCK_CARPET, 1, 0x8 }, { BLOCK_CARPET, 1, 0x7 }, { BLOCK_CARPET, 1, 0xF },
    { BLOCK_CARPET, 1, 0xC }, { BLOCK_CARPET, 1, 0xE }, { BLOCK_CARPET, 1, 0x1 }, { BLOCK_CARPET, 1, 0x4 },
    { BLOCK_CARPET, 1, 0x5 }, { BLOCK_CARPET, 1, 0xD }, { BLOCK_CARPET, 1, 0x9 }, { BLOCK_CARPET, 1, 0x3 },
    { BLOCK_CARPET, 1, 0xB }, { BLOCK_CARPET, 1, 0xA }, { BLOCK_CARPET, 1, 0x2 }, { BLOCK_CARPET, 1, 0x6 },

    { ITEM_BONEMEAL, 1, 0x0 }, { ITEM_BONEMEAL, 1, 0x8 }, { ITEM_BONEMEAL, 1, 0x7 }, { ITEM_BONEMEAL, 1, 0xF },
    { ITEM_BONEMEAL, 1, 0xC }, { ITEM_BONEMEAL, 1, 0xE }, { ITEM_BONEMEAL, 1, 0x1 }, { ITEM_BONEMEAL, 1, 0x4 },
    { ITEM_BONEMEAL, 1, 0x5 }, { ITEM_BONEMEAL, 1, 0xD }, { ITEM_BONEMEAL, 1, 0x9 }, { ITEM_BONEMEAL, 1, 0x3 },
    { ITEM_BONEMEAL, 1, 0xB }, { ITEM_BONEMEAL, 1, 0xA }, { ITEM_BONEMEAL, 1, 0x2 }, { ITEM_BONEMEAL, 1, 0x6 },

    { ITEM_CAMERA,              1, 0 },

    { ITEM_SPAWN_EGG,           1, EntityTypes::IdZombie },
    { ITEM_SPAWN_EGG,           1, EntityTypes::IdCreeper },
    { ITEM_SPAWN_EGG,           1, EntityTypes::IdSkeleton },
    { ITEM_SPAWN_EGG,           1, EntityTypes::IdSpider },
    { ITEM_SPAWN_EGG,           1, EntityTypes::IdPigZombie },
};
enum { N_ITEMS = (int)(sizeof(kItems) / sizeof(kItems[0])) };

short s_tab[CREATIVE_TABS][N_ITEMS];
int   s_tabN[CREATIVE_TABS];
bool  s_built = false;

}

namespace CreativeItems {

void populate() {
    if (s_built) return;
    for (int t = 0; t < CREATIVE_TABS; t++) s_tabN[t] = 0;
    for (int i = 0; i < N_ITEMS; i++) {

        Item* it = (kItems[i].id > 0 && kItems[i].id < 4096) ? Item::items[kItems[i].id] : 0;
        int tab = it ? it->creativeTab : 0;

        if (tab < 1 || tab > CREATIVE_TABS) tab = 1;
        s_tab[tab - 1][s_tabN[tab - 1]++] = (short)i;
    }
    s_built = true;
}

int tabCount(int tab) {
    if (tab < 0 || tab >= CREATIVE_TABS) return 0;
    return s_tabN[tab];
}

const CreativeEntry* at(int tab, int i) {
    if (tab < 0 || tab >= CREATIVE_TABS || i < 0 || i >= s_tabN[tab]) return 0;
    return &kItems[s_tab[tab][i]];
}

int count() { return N_ITEMS; }

const CreativeEntry* entry(int i) {
    if (i < 0 || i >= N_ITEMS) return 0;
    return &kItems[i];
}

}
