#include "world/item/item.h"
#include "world/item/tile_item.h"
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
#include "world/entity/entity_types.h"

Item* Item::items[4096];

Item::Item(short id) : id(id), maxStackSize(64), maxDamage(0), category(-1) {
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
    new SimpleItem(ITEM_REEDS,          ic(11, 1));
    new SimpleItem(ITEM_PAPER,          ic(10, 3));
    new SimpleItem(ITEM_BOOK,           ic(11, 3));
    new SimpleItem(ITEM_SLIMEBALL,      ic(14, 1));
    new SimpleItem(ITEM_COMPASS,        ic(6, 3));
    new SimpleItem(ITEM_CLOCK,          ic(6, 4));
    new SimpleItem(ITEM_GLOWSTONE_DUST, ic(9, 4));
    new SimpleItem(ITEM_BONE,           ic(12, 1), 64, 0, true);
    new SimpleItem(ITEM_SUGAR,          ic(13, 0));
    new SimpleItem(ITEM_NETHER_BRICK,   ic(5, 9));
    new SimpleItem(ITEM_NETHER_QUARTZ,  ic(5, 10));

    new SimpleItem(ITEM_FLINT_AND_STEEL, ic(5, 0),  1, 64);
    new SimpleItem(ITEM_SHEARS,          ic(13, 5), 1, 238);
    new SimpleItem(ITEM_EGG,             ic(12, 0), 16);
    new SimpleItem(ITEM_SNOWBALL,        ic(14, 0), 16);
    new SimpleItem(ITEM_DOOR_WOOD_ITEM,  ic(11, 2), 1);
    new SimpleItem(ITEM_DOOR_IRON_ITEM,  ic(12, 2), 1);
    new SimpleItem(ITEM_BED_ITEM,        ic(13, 2), 1);
    new SimpleItem(ITEM_CAMERA,          ic(0, 14), 1);

    new SeedItem(ITEM_SEEDS_WHEAT, BLOCK_WHEAT, ic(9, 0));
    new SeedItem(ITEM_SEEDS_MELON, BLOCK_MELON_STEM, ic(14, 3));
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
            { ITEM_STRING, 2 }, { ITEM_FEATHER, 2 }, { ITEM_GUNPOWDER, 2 }, { ITEM_FLINT, 2 },
            { ITEM_LEATHER, 2 }, { ITEM_BONE, 2 }, { ITEM_SHEARS, 2 },
            { BLOCK_TNT, 2 }, { BLOCK_TORCH, 2 },

            { ITEM_APPLE, 4 }, { ITEM_BOWL, 4 }, { ITEM_MUSHROOM_STEW, 4 },
            { ITEM_WHEAT, 4 }, { ITEM_BREAD, 4 }, { ITEM_SUGAR, 4 }, { ITEM_REEDS, 4 },
            { ITEM_SEEDS_WHEAT, 4 }, { ITEM_SEEDS_MELON, 4 }, { ITEM_MELON, 4 },
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
            { BLOCK_SAND, 1 }, { BLOCK_SANDSTONE, 1 }, { BLOCK_WOOL, 1 }, { BLOCK_SLAB, 1 },
            { BLOCK_BRICKS, 1 }, { BLOCK_GLASS_PANE, 1 }, { BLOCK_FENCE, 1 },
            { BLOCK_FENCE_GATE, 1 }, { BLOCK_SNOW_BLOCK, 1 }, { BLOCK_CLAY, 1 },
            { BLOCK_GLOWSTONE, 1 }, { BLOCK_LADDER, 1 }, { BLOCK_TRAPDOOR, 1 },
            { BLOCK_STONE_BRICKS, 1 }, { BLOCK_STAIRS_PLANKS, 1 }, { BLOCK_STAIRS_COBBLESTONE, 1 },
            { BLOCK_STAIRS_BRICK, 1 }, { BLOCK_STAIRS_STONE_BRICK, 1 }, { BLOCK_STAIRS_NETHER_BRICK, 1 },
            { BLOCK_CHEST, 1 }, { BLOCK_FURNACE, 1 }, { BLOCK_CRAFTING_TABLE, 1 },
            { BLOCK_STONECUTTER, 1 }, { BLOCK_NETHER_BRICK, 1 }, { BLOCK_QUARTZ_BLOCK, 1 },
            { BLOCK_NETHER_REACTOR, 1 },

            { ITEM_DIAMOND, 8 }, { ITEM_IRON_INGOT, 8 }, { ITEM_GOLD_INGOT, 8 },
            { ITEM_PAPER, 8 }, { ITEM_BOOK, 8 }, { ITEM_PAINTING, 8 }, { ITEM_SIGN, 8 },
            { ITEM_SNOWBALL, 8 }, { ITEM_BONEMEAL, 8 },
            { BLOCK_GOLD_BLOCK, 8 }, { BLOCK_IRON_BLOCK, 8 }, { BLOCK_DIAMOND_BLOCK, 8 },
            { BLOCK_LAPIS_BLOCK, 8 }, { BLOCK_BOOKSHELF, 8 },

            { ITEM_NETHER_QUARTZ, 16 },
        };
        for (unsigned int i = 0; i < sizeof(kCat) / sizeof(kCat[0]); i++)
            if (items[kCat[i].id]) items[kCat[i].id]->category = kCat[i].cat;
    }
}
