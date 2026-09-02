#include "world/item/item.h"
#include "world/item/tile_item.h"
#include "world/item/bucket_item.h"
#include "world/item/simple_item.h"
#include "world/item/digger_item.h"
#include "world/item/weapon_item.h"
#include "world/item/hoe_item.h"
#include "world/item/food_item.h"
#include "world/item/armor_item.h"
#include "world/item/seed_item.h"
#include "world/item/hanging_entity_item.h"
#include "world/item/sign_item.h"
#include "world/item/bonemeal_item.h"
#include "world/item/bow_item.h"
#include "world/item/spawn_egg_item.h"
#include "world/item/shears_item.h"
#include "world/item/clock_item.h"
#include "world/item/compass_item.h"
#include "world/item/minecart_item.h"
#include "world/entity/entity_types.h"

Item* Item::items[4096];

Item::Item(short id) : id(id), maxStackSize(64), maxDamage(0), category(-1), creativeTab(0) {
    items[id] = this;
}

const Item::Tier Item::Tier::WOOD   (0, 59,   2, 0);
const Item::Tier Item::Tier::STONE  (1, 131,  4, 1);
const Item::Tier Item::Tier::IRON   (2, 250,  6, 2);
const Item::Tier Item::Tier::EMERALD(3, 1561, 8, 3);
const Item::Tier Item::Tier::GOLD   (0, 32,  12, 0);

static inline int ic(int col, int row) { return col + row * 16; }

void Item::initItems() {
    for (int i = 0; i < 4096; i++) items[i] = nullptr;

    for (int i = 1; i < 256; i++) new TileItem(i);

    new ShovelItem (ITEM_SHOVEL_IRON,     Tier::IRON,    ic(2, 5));
    new PickaxeItem(ITEM_PICKAXE_IRON,    Tier::IRON,    ic(2, 6));
    new HatchetItem(ITEM_HATCHET_IRON,    Tier::IRON,    ic(2, 7));
    new WeaponItem (ITEM_SWORD_IRON,      Tier::IRON,    ic(2, 4));
    new ShovelItem (ITEM_SHOVEL_WOOD,     Tier::WOOD,    ic(0, 5));
    new PickaxeItem(ITEM_PICKAXE_WOOD,    Tier::WOOD,    ic(0, 6));
    new HatchetItem(ITEM_HATCHET_WOOD,    Tier::WOOD,    ic(0, 7));
    new WeaponItem (ITEM_SWORD_WOOD,      Tier::WOOD,    ic(0, 4));
    new ShovelItem (ITEM_SHOVEL_STONE,    Tier::STONE,   ic(1, 5));
    new PickaxeItem(ITEM_PICKAXE_STONE,   Tier::STONE,   ic(1, 6));
    new HatchetItem(ITEM_HATCHET_STONE,   Tier::STONE,   ic(1, 7));
    new WeaponItem (ITEM_SWORD_STONE,     Tier::STONE,   ic(1, 4));
    new ShovelItem (ITEM_SHOVEL_DIAMOND,  Tier::EMERALD, ic(3, 5));
    new PickaxeItem(ITEM_PICKAXE_DIAMOND, Tier::EMERALD, ic(3, 6));
    new HatchetItem(ITEM_HATCHET_DIAMOND, Tier::EMERALD, ic(3, 7));
    new WeaponItem (ITEM_SWORD_DIAMOND,   Tier::EMERALD, ic(3, 4));
    new ShovelItem (ITEM_SHOVEL_GOLD,     Tier::GOLD,    ic(4, 5));
    new PickaxeItem(ITEM_PICKAXE_GOLD,    Tier::GOLD,    ic(4, 6));
    new HatchetItem(ITEM_HATCHET_GOLD,    Tier::GOLD,    ic(4, 7));
    new WeaponItem (ITEM_SWORD_GOLD,      Tier::GOLD,    ic(4, 4));
    new HoeItem(ITEM_HOE_WOOD,    Tier::WOOD,    ic(0, 8));
    new HoeItem(ITEM_HOE_STONE,   Tier::STONE,   ic(1, 8));
    new HoeItem(ITEM_HOE_IRON,    Tier::IRON,    ic(2, 8));
    new HoeItem(ITEM_HOE_DIAMOND, Tier::EMERALD, ic(3, 8));
    new HoeItem(ITEM_HOE_GOLD,    Tier::GOLD,    ic(4, 8));

    new ArmorItem(ITEM_HELMET_CLOTH,     ArmorItem::CLOTH,   ArmorItem::SLOT_HEAD,  ic(0, 0));
    new ArmorItem(ITEM_CHESTPLATE_CLOTH, ArmorItem::CLOTH,   ArmorItem::SLOT_TORSO, ic(0, 1));
    new ArmorItem(ITEM_LEGGINGS_CLOTH,   ArmorItem::CLOTH,   ArmorItem::SLOT_LEGS,  ic(0, 2));
    new ArmorItem(ITEM_BOOTS_CLOTH,      ArmorItem::CLOTH,   ArmorItem::SLOT_FEET,  ic(0, 3));
    new ArmorItem(ITEM_HELMET_CHAIN,     ArmorItem::CHAIN,   ArmorItem::SLOT_HEAD,  ic(1, 0));
    new ArmorItem(ITEM_CHESTPLATE_CHAIN, ArmorItem::CHAIN,   ArmorItem::SLOT_TORSO, ic(1, 1));
    new ArmorItem(ITEM_LEGGINGS_CHAIN,   ArmorItem::CHAIN,   ArmorItem::SLOT_LEGS,  ic(1, 2));
    new ArmorItem(ITEM_BOOTS_CHAIN,      ArmorItem::CHAIN,   ArmorItem::SLOT_FEET,  ic(1, 3));
    new ArmorItem(ITEM_HELMET_IRON,      ArmorItem::IRON,    ArmorItem::SLOT_HEAD,  ic(2, 0));
    new ArmorItem(ITEM_CHESTPLATE_IRON,  ArmorItem::IRON,    ArmorItem::SLOT_TORSO, ic(2, 1));
    new ArmorItem(ITEM_LEGGINGS_IRON,    ArmorItem::IRON,    ArmorItem::SLOT_LEGS,  ic(2, 2));
    new ArmorItem(ITEM_BOOTS_IRON,       ArmorItem::IRON,    ArmorItem::SLOT_FEET,  ic(2, 3));
    new ArmorItem(ITEM_HELMET_DIAMOND,   ArmorItem::DIAMOND, ArmorItem::SLOT_HEAD,  ic(3, 0));
    new ArmorItem(ITEM_CHESTPLATE_DIAMOND,ArmorItem::DIAMOND,ArmorItem::SLOT_TORSO, ic(3, 1));
    new ArmorItem(ITEM_LEGGINGS_DIAMOND, ArmorItem::DIAMOND, ArmorItem::SLOT_LEGS,  ic(3, 2));
    new ArmorItem(ITEM_BOOTS_DIAMOND,    ArmorItem::DIAMOND, ArmorItem::SLOT_FEET,  ic(3, 3));
    new ArmorItem(ITEM_HELMET_GOLD,      ArmorItem::GOLD,    ArmorItem::SLOT_HEAD,  ic(4, 0));
    new ArmorItem(ITEM_CHESTPLATE_GOLD,  ArmorItem::GOLD,    ArmorItem::SLOT_TORSO, ic(4, 1));
    new ArmorItem(ITEM_LEGGINGS_GOLD,    ArmorItem::GOLD,    ArmorItem::SLOT_LEGS,  ic(4, 2));
    new ArmorItem(ITEM_BOOTS_GOLD,       ArmorItem::GOLD,    ArmorItem::SLOT_FEET,  ic(4, 3));

    new FoodItem(ITEM_APPLE,           4, false, ic(10, 0));
    new FoodItem(ITEM_BREAD,           5, false, ic(9,  2));
    new BowlFoodItem(ITEM_MUSHROOM_STEW, 8,       ic(8,  4));
    new FoodItem(ITEM_PORKCHOP_RAW,    3, true,  ic(7,  5));
    new FoodItem(ITEM_PORKCHOP_COOKED, 8, true,  ic(8,  5));
    new FoodItem(ITEM_MELON,           2, false, ic(13, 6));
    new FoodItem(ITEM_BEEF_RAW,        3, true,  ic(9,  6));
    new FoodItem(ITEM_BEEF_COOKED,     8, true,  ic(10, 6));
    new FoodItem(ITEM_CHICKEN_RAW,     2, true,  ic(9,  7));
    new FoodItem(ITEM_CHICKEN_COOKED,  6, true,  ic(10, 7));

    new SeedFoodItem(ITEM_CARROT,      4, BLOCK_CARROTS,  ic(8, 7));
    new SeedFoodItem(ITEM_POTATO,      1, BLOCK_POTATOES, ic(7, 7));
    new FoodItem(ITEM_POTATO_BAKED,    6, false, ic(6, 7));
    new FoodItem(ITEM_PUMPKIN_PIE,     8, false, ic(8, 9));
    new FoodItem(ITEM_BEETROOT,        1, false, ic(1, 12));

    new BowlFoodItem(ITEM_BEETROOT_SOUP, 8,   ic(3, 12));

    new SimpleItem(ITEM_ARROW,          ic(5, 2));
    new SimpleItem(ITEM_COAL,           ic(7, 0));
    new SimpleItem(ITEM_DIAMOND,        ic(7, 3));
    new SimpleItem(ITEM_IRON_INGOT,     ic(7, 1));
    new SimpleItem(ITEM_GOLD_INGOT,     ic(7, 2));
    new SimpleItem(ITEM_STICK,          ic(5, 3), 64, 0, true);
    new SimpleItem(ITEM_BOWL,           ic(7, 4));
    new SimpleItem(ITEM_STRING,         ic(8, 0));
    new SimpleItem(ITEM_FEATHER,        ic(8, 1));
    new SimpleItem(ITEM_GUNPOWDER,      ic(8, 2));
    new SimpleItem(ITEM_WHEAT,          ic(9, 1));
    new SimpleItem(ITEM_FLINT,          ic(6, 0));
    new SimpleItem(ITEM_LEATHER,        ic(7, 6));
    new SimpleItem(ITEM_BRICK,          ic(6, 1));
    new SimpleItem(ITEM_CLAY,           ic(9, 3));
    new SlabItem(BLOCK_SLAB, BLOCK_DOUBLE_SLAB);
    new SlabItem(BLOCK_WOOD_SLAB, BLOCK_WOOD_SLAB_DOUBLE);
    new TileItem(ITEM_REEDS, BLOCK_REEDS, ic(11, 1));
    new SimpleItem(ITEM_PAPER,          ic(10, 3));
    new SimpleItem(ITEM_BOOK,           ic(11, 3));
    new SimpleItem(ITEM_SLIMEBALL,      ic(14, 1));

    new MinecartItem(ITEM_MINECART);

    new CompassItem(ITEM_COMPASS);
    new ClockItem(ITEM_CLOCK);
    new SimpleItem(ITEM_GLOWSTONE_DUST, ic(9, 4));

    new SimpleItem(ITEM_REDSTONE,       ic(8, 3));
    new SimpleItem(ITEM_BONE,           ic(12, 1), 64, 0, true);
    new BucketItem(ITEM_BUCKET);
    new SimpleItem(ITEM_SUGAR,          ic(13, 0));

    (new TileItem(ITEM_CAKE, BLOCK_CAKE, ic(13, 1)))->maxStackSize = 1;
    new SimpleItem(ITEM_NETHER_BRICK,   ic(5, 9));
    new SimpleItem(ITEM_NETHER_QUARTZ,  ic(5, 10));

    new FlintAndSteelItem(ITEM_FLINT_AND_STEEL, ic(5, 0));
    new ShearsItem(ITEM_SHEARS,          ic(13, 5));
    new SimpleItem(ITEM_EGG,             ic(12, 0), 16);
    new SimpleItem(ITEM_SNOWBALL,        ic(14, 0), 16);
    new DoorItem(ITEM_DOOR_WOOD_ITEM, BLOCK_DOOR_WOOD, ic(11, 2));
    new DoorItem(ITEM_DOOR_IRON_ITEM, BLOCK_DOOR_IRON, ic(12, 2));
    new BedItem (ITEM_BED_ITEM,       BLOCK_BED,       ic(13, 2));
    new SimpleItem(ITEM_CAMERA,          ic(0, 14), 1);

    new SeedItem(ITEM_SEEDS_WHEAT, BLOCK_WHEAT, ic(9, 0));
    new SeedItem(ITEM_SEEDS_MELON, BLOCK_MELON_STEM, ic(14, 3));

    new SeedItem(ITEM_SEEDS_PUMPKIN,   BLOCK_PUMPKIN_STEM, ic(13, 3));
    new SeedItem(ITEM_SEEDS_BEETROOT,  BLOCK_BEETROOT,     ic(2, 12));
    new HangingEntityItem(ITEM_PAINTING, EntityTypes::IdPainting, ic(10, 1));
    new SignItem(ITEM_SIGN, ic(10, 2));
    new BonemealItem(ITEM_BONEMEAL);
    new BowItem(ITEM_BOW);
    new SpawnEggItem();

    {
        static const struct { short id; signed char cat; } kCat[] = {

            { ITEM_SHOVEL_IRON, 2 }, { ITEM_PICKAXE_IRON, 2 }, { ITEM_HATCHET_IRON, 2 },
            { ITEM_SWORD_IRON, 2 },  { ITEM_SWORD_WOOD, 2 },   { ITEM_SHOVEL_WOOD, 2 },
            { ITEM_PICKAXE_WOOD, 2 },{ ITEM_HATCHET_WOOD, 2 }, { ITEM_SWORD_STONE, 2 },
            { ITEM_SHOVEL_STONE, 2 },{ ITEM_PICKAXE_STONE, 2 },{ ITEM_HATCHET_STONE, 2 },
            { ITEM_SWORD_DIAMOND, 2 },{ ITEM_SHOVEL_DIAMOND, 2 },{ ITEM_PICKAXE_DIAMOND, 2 },
            { ITEM_HATCHET_DIAMOND, 2 },{ ITEM_SWORD_GOLD, 2 },{ ITEM_SHOVEL_GOLD, 2 },
            { ITEM_PICKAXE_GOLD, 2 },{ ITEM_HATCHET_GOLD, 2 },
            { ITEM_HOE_WOOD, 2 }, { ITEM_HOE_STONE, 2 }, { ITEM_HOE_IRON, 2 },
            { ITEM_HOE_DIAMOND, 2 }, { ITEM_HOE_GOLD, 2 },
            { ITEM_FLINT_AND_STEEL, 2 }, { ITEM_BOW, 2 }, { ITEM_ARROW, 2 }, { ITEM_COAL, 2 },
            { ITEM_BUCKET, 2 },
            { ITEM_STRING, 2 }, { ITEM_FEATHER, 2 }, { ITEM_GUNPOWDER, 2 }, { ITEM_FLINT, 2 },
            { ITEM_LEATHER, 2 }, { ITEM_BONE, 2 }, { ITEM_SHEARS, 2 },
            { ITEM_COMPASS, 2 }, { ITEM_CLOCK, 2 },
            { ITEM_MINECART, 2 }, { BLOCK_RAIL, 2 }, { BLOCK_GOLDEN_RAIL, 2 },
            { BLOCK_TNT, 2 }, { BLOCK_TORCH, 2 },

            { ITEM_CAMERA, 2 },

            { ITEM_APPLE, 4 }, { ITEM_BOWL, 4 }, { ITEM_MUSHROOM_STEW, 4 },
            { ITEM_WHEAT, 4 }, { ITEM_BREAD, 4 }, { ITEM_SUGAR, 4 }, { ITEM_REEDS, 4 },
            { ITEM_CAKE, 4 },
            { ITEM_SEEDS_WHEAT, 4 }, { ITEM_SEEDS_MELON, 4 }, { ITEM_MELON, 4 },

            { ITEM_SEEDS_PUMPKIN, 4 }, { ITEM_CARROT, 4 }, { ITEM_POTATO, 4 },
            { ITEM_POTATO_BAKED, 4 }, { ITEM_PUMPKIN_PIE, 4 },
            { ITEM_BEETROOT, 4 }, { ITEM_SEEDS_BEETROOT, 4 }, { ITEM_BEETROOT_SOUP, 4 },
            { ITEM_PORKCHOP_RAW, 4 }, { ITEM_PORKCHOP_COOKED, 4 },
            { ITEM_HELMET_CLOTH, 4 }, { ITEM_CHESTPLATE_CLOTH, 4 }, { ITEM_LEGGINGS_CLOTH, 4 }, { ITEM_BOOTS_CLOTH, 4 },
            { ITEM_HELMET_CHAIN, 4 }, { ITEM_CHESTPLATE_CHAIN, 4 }, { ITEM_LEGGINGS_CHAIN, 4 }, { ITEM_BOOTS_CHAIN, 4 },
            { ITEM_HELMET_IRON, 4 }, { ITEM_CHESTPLATE_IRON, 4 }, { ITEM_LEGGINGS_IRON, 4 }, { ITEM_BOOTS_IRON, 4 },
            { ITEM_HELMET_DIAMOND, 4 }, { ITEM_CHESTPLATE_DIAMOND, 4 }, { ITEM_LEGGINGS_DIAMOND, 4 }, { ITEM_BOOTS_DIAMOND, 4 },
            { ITEM_HELMET_GOLD, 4 }, { ITEM_CHESTPLATE_GOLD, 4 }, { ITEM_LEGGINGS_GOLD, 4 }, { ITEM_BOOTS_GOLD, 4 },
            { BLOCK_MELON, 4 },

            { ITEM_STICK, 1 }, { ITEM_BRICK, 1 }, { ITEM_DOOR_WOOD_ITEM, 1 },
            { ITEM_DOOR_IRON_ITEM, 1 }, { ITEM_BED_ITEM, 1 }, { ITEM_NETHER_BRICK, 1 },
            { BLOCK_STONE, 1 }, { BLOCK_PLANKS, 1 }, { BLOCK_COBBLESTONE, 1 },
            { BLOCK_SAND, 1 }, { BLOCK_SANDSTONE, 1 }, { BLOCK_WOOL, 1 }, { BLOCK_SLAB, 1 }, { BLOCK_WOOD_SLAB, 1 },
            { BLOCK_SPONGE, 1 },

            { BLOCK_PUMPKIN, 1 }, { BLOCK_PUMPKIN_LIT, 1 },
            { BLOCK_BRICKS, 1 }, { BLOCK_GLASS_PANE, 1 }, { BLOCK_FENCE, 1 },
            { BLOCK_COBBLE_WALL, 1 },
            { BLOCK_FENCE_GATE, 1 }, { BLOCK_SNOW_BLOCK, 1 }, { BLOCK_CLAY, 1 },
            { BLOCK_GLOWSTONE, 1 }, { BLOCK_LADDER, 1 }, { BLOCK_TRAPDOOR, 1 },
            { BLOCK_STONE_BRICKS, 1 }, { BLOCK_STAIRS_PLANKS, 1 }, { BLOCK_STAIRS_COBBLESTONE, 1 },

            { BLOCK_STAIRS_SPRUCE, 1 }, { BLOCK_STAIRS_BIRCH, 1 }, { BLOCK_STAIRS_JUNGLE, 1 },
            { BLOCK_STAIRS_BRICK, 1 }, { BLOCK_STAIRS_STONE_BRICK, 1 }, { BLOCK_STAIRS_NETHER_BRICK, 1 },
            { BLOCK_STAIRS_QUARTZ, 1 }, { BLOCK_STAIRS_SANDSTONE, 1 },
            { BLOCK_CHEST, 1 }, { BLOCK_FURNACE, 1 }, { BLOCK_CRAFTING_TABLE, 1 },
            { BLOCK_STONECUTTER, 1 }, { BLOCK_NETHER_BRICK, 1 }, { BLOCK_QUARTZ_BLOCK, 1 },
            { BLOCK_NETHER_REACTOR, 1 },

            { ITEM_DIAMOND, 8 }, { ITEM_IRON_INGOT, 8 }, { ITEM_GOLD_INGOT, 8 },
            { ITEM_PAPER, 8 }, { ITEM_BOOK, 8 }, { ITEM_PAINTING, 8 }, { ITEM_SIGN, 8 },
            { ITEM_SNOWBALL, 8 }, { ITEM_BONEMEAL, 8 },
            { ITEM_REDSTONE, 8 },
            { BLOCK_GOLD_BLOCK, 8 }, { BLOCK_IRON_BLOCK, 8 }, { BLOCK_DIAMOND_BLOCK, 8 },
            { BLOCK_LAPIS_BLOCK, 8 }, { BLOCK_BOOKSHELF, 8 }, { BLOCK_COAL_BLOCK, 8 },
            { BLOCK_CARPET, 8 },
            { BLOCK_HAY_BLOCK, 8 },
            { BLOCK_IRON_BARS, 8 },

            { ITEM_NETHER_QUARTZ, 16 },
        };
        for (unsigned int i = 0; i < sizeof(kCat) / sizeof(kCat[0]); i++)
            if (items[kCat[i].id]) items[kCat[i].id]->category = kCat[i].cat;
    }

    {
        static const struct { short id; unsigned char tab; } kTab[] = {
            { BLOCK_STONE, 1 }, { BLOCK_GRASS, 1 }, { BLOCK_DIRT, 1 }, { BLOCK_COBBLESTONE, 1 },
            { BLOCK_PLANKS, 1 }, { BLOCK_SAPLING, 2 }, { BLOCK_BEDROCK, 1 }, { BLOCK_WATER, 1 },
            { BLOCK_CALM_WATER, 1 }, { BLOCK_LAVA, 1 }, { BLOCK_CALM_LAVA, 1 }, { BLOCK_SAND, 1 },
            { BLOCK_GRAVEL, 1 }, { BLOCK_ORE_GOLD, 1 }, { BLOCK_ORE_IRON, 1 }, { BLOCK_ORE_COAL, 1 },
            { BLOCK_LOG, 1 }, { BLOCK_LEAVES, 2 }, { BLOCK_GLASS, 2 }, { BLOCK_ORE_LAPIS, 1 },
            { BLOCK_LAPIS_BLOCK, 2 }, { BLOCK_SANDSTONE, 1 }, { BLOCK_BED, 2 }, { BLOCK_COBWEB, 2 },
            { BLOCK_COAL_BLOCK, 2 },
            { BLOCK_CARPET, 2 },
            { BLOCK_HAY_BLOCK, 2 },
            { BLOCK_IRON_BARS, 2 },
            { BLOCK_TALLGRASS, 2 }, { BLOCK_WOOL, 2 }, { BLOCK_FLOWER, 2 }, { BLOCK_ROSE, 2 },
            { BLOCK_SPONGE, 2 },
            { BLOCK_MUSHROOM_BROWN, 2 }, { BLOCK_MUSHROOM_RED, 2 }, { BLOCK_GOLD_BLOCK, 2 },
            { BLOCK_IRON_BLOCK, 2 }, { BLOCK_DOUBLE_SLAB, 1 }, { BLOCK_SLAB, 1 }, { BLOCK_BRICKS, 1 },
            { BLOCK_TNT, 3 }, { BLOCK_BOOKSHELF, 2 }, { BLOCK_MOSSY_COBBLE, 1 }, { BLOCK_OBSIDIAN, 1 },
            { BLOCK_TORCH, 3 }, { BLOCK_FIRE, 1 }, { BLOCK_STAIRS_PLANKS, 1 }, { BLOCK_CHEST, 2 },
            { BLOCK_STAIRS_SPRUCE, 1 }, { BLOCK_STAIRS_BIRCH, 1 }, { BLOCK_STAIRS_JUNGLE, 1 },
            { BLOCK_ORE_EMERALD, 1 }, { BLOCK_DIAMOND_BLOCK, 2 }, { BLOCK_CRAFTING_TABLE, 2 },
            { BLOCK_WHEAT, 4 }, { BLOCK_FARMLAND, 1 }, { BLOCK_FURNACE, 2 }, { BLOCK_FURNACE_LIT, 2 },

            { BLOCK_CARROTS, 4 }, { BLOCK_POTATOES, 4 }, { BLOCK_BEETROOT, 4 },
            { BLOCK_PUMPKIN_STEM, 4 }, { BLOCK_PUMPKIN, 1 }, { BLOCK_PUMPKIN_LIT, 1 },
            { BLOCK_SIGN, 2 }, { BLOCK_DOOR_WOOD, 2 }, { BLOCK_LADDER, 2 },
            { BLOCK_STAIRS_COBBLESTONE, 1 }, { BLOCK_WALL_SIGN, 2 }, { BLOCK_DOOR_IRON, 2 },
            { BLOCK_ORE_REDSTONE, 1 }, { BLOCK_ORE_REDSTONE_LIT, 2 }, { BLOCK_TOPSNOW, 2 },
            { BLOCK_ICE, 1 }, { BLOCK_SNOW_BLOCK, 1 }, { BLOCK_CACTUS, 2 }, { BLOCK_CLAY, 1 },
            { BLOCK_REEDS, 2 }, { BLOCK_FENCE, 2 }, { BLOCK_NETHERRACK, 1 }, { BLOCK_GLOWSTONE, 2 },
            { BLOCK_COBBLE_WALL, 2 },
            { BLOCK_CAKE, 2 }, { BLOCK_TRAPDOOR, 2 }, { BLOCK_STONE_BRICKS, 1 },
            { BLOCK_GLASS_PANE, 2 }, { BLOCK_MELON, 2 }, { BLOCK_MELON_STEM, 2 },
            { BLOCK_FENCE_GATE, 2 }, { BLOCK_STAIRS_BRICK, 1 }, { BLOCK_STAIRS_STONE_BRICK, 1 },
            { BLOCK_NETHER_BRICK, 1 }, { BLOCK_STAIRS_NETHER_BRICK, 1 }, { BLOCK_STAIRS_SANDSTONE, 1 },
            { BLOCK_QUARTZ_BLOCK, 1 }, { BLOCK_STAIRS_QUARTZ, 1 }, { BLOCK_WOOD_SLAB_DOUBLE, 1 },
            { BLOCK_WOOD_SLAB, 1 }, { BLOCK_STONECUTTER, 2 }, { BLOCK_GLOWING_OBSIDIAN, 2 },
            { BLOCK_NETHER_REACTOR, 2 }, { BLOCK_UPDATE1, 1 }, { BLOCK_UPDATE2, 1 },
            { ITEM_SHOVEL_IRON, 3 }, { ITEM_PICKAXE_IRON, 3 }, { ITEM_HATCHET_IRON, 3 },
            { ITEM_FLINT_AND_STEEL, 3 }, { ITEM_APPLE, 4 }, { ITEM_BOW, 3 }, { ITEM_ARROW, 4 },
            { ITEM_COAL, 4 }, { ITEM_DIAMOND, 4 }, { ITEM_IRON_INGOT, 4 }, { ITEM_GOLD_INGOT, 4 },
            { ITEM_SWORD_IRON, 3 }, { ITEM_SWORD_WOOD, 3 }, { ITEM_SHOVEL_WOOD, 3 },
            { ITEM_PICKAXE_WOOD, 3 }, { ITEM_HATCHET_WOOD, 3 }, { ITEM_SWORD_STONE, 3 },
            { ITEM_SHOVEL_STONE, 3 }, { ITEM_PICKAXE_STONE, 3 }, { ITEM_HATCHET_STONE, 3 },
            { ITEM_SWORD_DIAMOND, 3 }, { ITEM_SHOVEL_DIAMOND, 3 }, { ITEM_PICKAXE_DIAMOND, 3 },
            { ITEM_HATCHET_DIAMOND, 3 }, { ITEM_STICK, 4 }, { ITEM_BOWL, 4 },
            { ITEM_MUSHROOM_STEW, 4 }, { ITEM_SHOVEL_GOLD, 3 }, { ITEM_PICKAXE_GOLD, 3 },
            { ITEM_HATCHET_GOLD, 3 }, { ITEM_STRING, 4 }, { ITEM_FEATHER, 4 }, { ITEM_GUNPOWDER, 4 },
            { ITEM_HOE_WOOD, 3 }, { ITEM_HOE_STONE, 3 }, { ITEM_HOE_IRON, 3 }, { ITEM_HOE_DIAMOND, 3 },
            { ITEM_HOE_GOLD, 3 }, { ITEM_SEEDS_WHEAT, 4 }, { ITEM_WHEAT, 4 }, { ITEM_BREAD, 4 },
            { ITEM_HELMET_CLOTH, 3 }, { ITEM_CHESTPLATE_CLOTH, 3 }, { ITEM_LEGGINGS_CLOTH, 3 },
            { ITEM_BOOTS_CLOTH, 3 }, { ITEM_HELMET_CHAIN, 3 }, { ITEM_CHESTPLATE_CHAIN, 3 },
            { ITEM_LEGGINGS_CHAIN, 3 }, { ITEM_BOOTS_CHAIN, 3 }, { ITEM_HELMET_IRON, 3 },
            { ITEM_CHESTPLATE_IRON, 3 }, { ITEM_LEGGINGS_IRON, 3 }, { ITEM_BOOTS_IRON, 3 },
            { ITEM_HELMET_DIAMOND, 3 }, { ITEM_CHESTPLATE_DIAMOND, 3 }, { ITEM_LEGGINGS_DIAMOND, 3 },
            { ITEM_BOOTS_DIAMOND, 3 }, { ITEM_HELMET_GOLD, 3 }, { ITEM_CHESTPLATE_GOLD, 3 },
            { ITEM_LEGGINGS_GOLD, 3 }, { ITEM_BOOTS_GOLD, 3 }, { ITEM_FLINT, 3 },
            { ITEM_PORKCHOP_RAW, 4 }, { ITEM_PORKCHOP_COOKED, 4 }, { ITEM_PAINTING, 2 },
            { ITEM_SIGN, 2 }, { ITEM_DOOR_WOOD_ITEM, 2 }, { ITEM_BUCKET, 3 },
            { ITEM_DOOR_IRON_ITEM, 2 }, { ITEM_SNOWBALL, 3 }, { ITEM_LEATHER, 4 }, { ITEM_BRICK, 4 },
            { ITEM_CLAY, 4 }, { ITEM_REEDS, 4 }, { ITEM_PAPER, 4 }, { ITEM_BOOK, 4 },
            { ITEM_SLIMEBALL, 4 }, { ITEM_EGG, 4 }, { ITEM_COMPASS, 3 }, { ITEM_CLOCK, 3 },
            { ITEM_REDSTONE, 3 }, { ITEM_MINECART, 3 },

            { BLOCK_RAIL, 3 }, { BLOCK_GOLDEN_RAIL, 3 },
            { ITEM_GLOWSTONE_DUST, 4 }, { ITEM_BONEMEAL, 4 }, { ITEM_BONE, 4 }, { ITEM_SUGAR, 4 },
            { ITEM_CAKE, 2 }, { ITEM_BED_ITEM, 2 }, { ITEM_SHEARS, 3 }, { ITEM_MELON, 2 },
            { ITEM_SEEDS_MELON, 4 }, { ITEM_BEEF_RAW, 4 }, { ITEM_BEEF_COOKED, 4 },
            { ITEM_CHICKEN_RAW, 4 }, { ITEM_CHICKEN_COOKED, 4 }, { ITEM_SPAWN_EGG, 3 },
            { ITEM_NETHER_BRICK, 4 }, { ITEM_NETHER_QUARTZ, 4 }, { ITEM_CAMERA, 3 },

            { ITEM_SEEDS_PUMPKIN, 4 }, { ITEM_CARROT, 4 }, { ITEM_POTATO, 4 },
            { ITEM_POTATO_BAKED, 4 }, { ITEM_PUMPKIN_PIE, 4 },
            { ITEM_BEETROOT, 4 }, { ITEM_SEEDS_BEETROOT, 4 }, { ITEM_BEETROOT_SOUP, 4 },

            { BLOCK_PUMPKIN, 2 }, { BLOCK_PUMPKIN_LIT, 2 }, { BLOCK_PUMPKIN_STEM, 2 },
            { BLOCK_CARROTS, 2 }, { BLOCK_POTATOES, 2 }, { BLOCK_BEETROOT, 2 },
        };
        for (unsigned int i = 0; i < sizeof(kTab) / sizeof(kTab[0]); i++)
            if (items[kTab[i].id]) items[kTab[i].id]->creativeTab = kTab[i].tab;
    }
}
