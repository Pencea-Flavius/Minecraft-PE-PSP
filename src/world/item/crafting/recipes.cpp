
#include "world/item/crafting/recipes.h"
#include "world/item/item.h"
#include "world/item/bucket_item.h"
#include <cstring>

 Recipes* Recipes::instance = nullptr;

Recipes* Recipes::getInstance() {
    if (!instance) instance = new Recipes();
    return instance;
}

Recipes::~Recipes() {
    for (unsigned int i = 0; i < recipes.size(); ++i) delete recipes[i];
}

enum { SLAB_STONE = 0, SLAB_SAND = 1, SLAB_WOOD = 2, SLAB_COBBLE = 3,
       SLAB_BRICK = 4, SLAB_SMOOTHBRICK = 5, SLAB_QUARTZ = 6 };
enum { SANDSTONE_PLAIN = 0, SANDSTONE_HEIROGLYPHS = 1, SANDSTONE_SMOOTHSIDE = 2 };
enum { COAL_CHARCOAL = 1 };

void Recipes::addShapedRows(const ItemInstance& result, const char* const* rows, int height,
                            std::initializer_list<Type> types) {
    int width = (int)strlen(rows[0]);
    ItemInstance* ids = new ItemInstance[width * height];
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            char ch = rows[y][x];
            for (std::initializer_list<Type>::const_iterator t = types.begin(); t != types.end(); ++t)
                if (t->c == ch) { ids[x + y * width] = t->ing; break; }
        }
    recipes.push_back(new ShapedRecipe(width, height, ids, result));
}

void Recipes::addShapedRecipe(const ItemInstance& result, const char* r0,
                              std::initializer_list<Type> types) {
    const char* rows[1] = { r0 };
    addShapedRows(result, rows, 1, types);
}
void Recipes::addShapedRecipe(const ItemInstance& result, const char* r0, const char* r1,
                              std::initializer_list<Type> types) {
    const char* rows[2] = { r0, r1 };
    addShapedRows(result, rows, 2, types);
}
void Recipes::addShapedRecipe(const ItemInstance& result, const char* r0, const char* r1, const char* r2,
                              std::initializer_list<Type> types) {
    const char* rows[3] = { r0, r1, r2 };
    addShapedRows(result, rows, 3, types);
}

void Recipes::addShapelessRecipe(const ItemInstance& result,
                                 std::initializer_list<ItemInstance> ingredients) {
    recipes.push_back(new ShapelessRecipe(result,
        std::vector<ItemInstance>(ingredients.begin(), ingredients.end())));
}

Recipes::Recipes() {

    {
        static const char* const shapes[4][3] = {
            { "XXX", " # ", " # " },
            { "X",   "#",   "#"   },
            { "XX",  "X#",  " #"  },
            { "XX",  " #",  " #"  },
        };
        const short materialIds[5] = { BLOCK_PLANKS, BLOCK_COBBLESTONE,
                                       ITEM_IRON_INGOT, ITEM_DIAMOND, ITEM_GOLD_INGOT };
        const short map[4][5] = {
            { ITEM_PICKAXE_WOOD, ITEM_PICKAXE_STONE, ITEM_PICKAXE_IRON, ITEM_PICKAXE_DIAMOND, ITEM_PICKAXE_GOLD },
            { ITEM_SHOVEL_WOOD,  ITEM_SHOVEL_STONE,  ITEM_SHOVEL_IRON,  ITEM_SHOVEL_DIAMOND,  ITEM_SHOVEL_GOLD  },
            { ITEM_HATCHET_WOOD, ITEM_HATCHET_STONE, ITEM_HATCHET_IRON, ITEM_HATCHET_DIAMOND, ITEM_HATCHET_GOLD },
            { ITEM_HOE_WOOD,     ITEM_HOE_STONE,     ITEM_HOE_IRON,     ITEM_HOE_DIAMOND,     ITEM_HOE_GOLD     },
        };
        for (int m = 0; m < 5; ++m)
            for (int t = 0; t < 4; ++t) {
                Type mat = (materialIds[m] < 256) ? TILE('X', materialIds[m]) : ITEM('X', materialIds[m]);
                addShapedRecipe(ItemInstance(map[t][m], 1, 0),
                                shapes[t][0], shapes[t][1], shapes[t][2],
                                { ITEM('#', ITEM_STICK), mat });
            }

        addShapedRecipe(ItemInstance(ITEM_SHEARS, 1, 0),
                        " #",
                        "# ", { ITEM('#', ITEM_IRON_INGOT) });
    }

    {
        const short materialIds[5] = { BLOCK_PLANKS, BLOCK_COBBLESTONE,
                                       ITEM_IRON_INGOT, ITEM_DIAMOND, ITEM_GOLD_INGOT };
        const short swords[5] = { ITEM_SWORD_WOOD, ITEM_SWORD_STONE, ITEM_SWORD_IRON,
                                  ITEM_SWORD_DIAMOND, ITEM_SWORD_GOLD };
        for (int m = 0; m < 5; ++m) {
            Type mat = (materialIds[m] < 256) ? TILE('X', materialIds[m]) : ITEM('X', materialIds[m]);
            addShapedRecipe(ItemInstance(swords[m], 1, 0),
                            "X",
                            "X",
                            "#", { ITEM('#', ITEM_STICK), mat });
        }

        addShapedRecipe(ItemInstance(ITEM_BOW, 1, 0),
                        " #X",
                        "# X",
                        " #X", { ITEM('X', ITEM_STRING), ITEM('#', ITEM_STICK) });

        addShapedRecipe(ItemInstance(ITEM_ARROW, 4, 0),
                        "X",
                        "#",
                        "Y", { ITEM('Y', ITEM_FEATHER), ITEM('X', ITEM_FLINT), ITEM('#', ITEM_STICK) });

        addShapedRecipe(ItemInstance(ITEM_CAMERA, 1, 0),
                        "GPG",
                        "GRG",
                        "S S", { ITEM('G', ITEM_GOLD_INGOT), TILE('P', BLOCK_GLASS_PANE),
                                 ITEM('R', ITEM_REDSTONE),   ITEM('S', ITEM_STICK) });
    }

    {
        struct { short block; short item; short itemAux; } map[6] = {
            { BLOCK_GOLD_BLOCK,    ITEM_GOLD_INGOT, 0 },
            { BLOCK_IRON_BLOCK,    ITEM_IRON_INGOT, 0 },
            { BLOCK_DIAMOND_BLOCK, ITEM_DIAMOND,    0 },
            { BLOCK_LAPIS_BLOCK,   ITEM_BONEMEAL,   DYE_BLUE },

            { BLOCK_HAY_BLOCK,     ITEM_WHEAT,      0 },

            { BLOCK_COAL_BLOCK,    ITEM_COAL,       0 },
        };
        for (int i = 0; i < 6; ++i) {
            addShapedRecipe(ItemInstance(map[i].block, 1, 0),
                            "###",
                            "###",
                            "###", { INST('#', map[i].item, map[i].itemAux) });
            addShapedRecipe(ItemInstance(map[i].item, 9, map[i].itemAux),
                            "#", { TILE('#', map[i].block) });
        }
    }

    addShapelessRecipe(ItemInstance(ITEM_MUSHROOM_STEW, 1, 0),
        { ItemInstance(BLOCK_MUSHROOM_BROWN, 1, 0), ItemInstance(BLOCK_MUSHROOM_RED, 1, 0),
          ItemInstance(ITEM_BOWL, 1, 0) });

    addShapedRecipe(ItemInstance(BLOCK_MELON, 1, 0),
                    "MMM",
                    "MMM",
                    "MMM", { ITEM('M', ITEM_MELON) });

    addShapedRecipe(ItemInstance(ITEM_SEEDS_MELON, 1, 0),
                    "M", { ITEM('M', ITEM_MELON) });

    addShapedRecipe(ItemInstance(ITEM_SEEDS_PUMPKIN, 4, 0),
                    "M", { TILE('M', BLOCK_PUMPKIN) });

    addShapelessRecipe(ItemInstance(ITEM_PUMPKIN_PIE, 1, 0),
        { ItemInstance(BLOCK_PUMPKIN, 1, 0), ItemInstance(ITEM_SUGAR, 1, 0),
          ItemInstance(ITEM_EGG, 1, 0) });

    addShapedRecipe(ItemInstance(ITEM_BEETROOT_SOUP, 1, 0),
                    "BB",
                    "BB",
                    "#", { ITEM('B', ITEM_BEETROOT), ITEM('#', ITEM_BOWL) });

    addShapedRecipe(ItemInstance(BLOCK_CHEST, 1, 0),
                    "###",
                    "# #",
                    "###", { TILE('#', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(BLOCK_FURNACE, 1, 0),
                    "###",
                    "# #",
                    "###", { TILE('#', BLOCK_COBBLESTONE) });

    addShapedRecipe(ItemInstance(BLOCK_CRAFTING_TABLE, 1, 0),
                    "##",
                    "##", { TILE('#', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(BLOCK_STONECUTTER, 1, 0),
                    "##",
                    "##", { TILE('#', BLOCK_COBBLESTONE) });

    addShapedRecipe(ItemInstance(BLOCK_COBBLE_WALL, 6, WALL_COBBLE),
                    "###",
                    "###", { TILE('#', BLOCK_COBBLESTONE) });

    addShapedRecipe(ItemInstance(BLOCK_SANDSTONE, 1, 0),
                    "##",
                    "##", { TILE('#', BLOCK_SAND) });

    addShapedRecipe(ItemInstance(BLOCK_SANDSTONE, 4, SANDSTONE_SMOOTHSIDE),
                    "##",

                    "##", { INST('#', BLOCK_SANDSTONE, SANDSTONE_PLAIN) });

    addShapedRecipe(ItemInstance(BLOCK_SANDSTONE, 1, SANDSTONE_HEIROGLYPHS),
                    "#",
                    "#", { INST('#', BLOCK_SLAB, SLAB_SAND) });

    addShapedRecipe(ItemInstance(BLOCK_STONE_BRICKS, 4, 0),
                    "##",
                    "##", { TILE('#', BLOCK_STONE) });

    addShapedRecipe(ItemInstance(BLOCK_GLASS_PANE, 16, 0),
                    "###",
                    "###", { TILE('#', BLOCK_GLASS) });

    addShapedRecipe(ItemInstance(BLOCK_NETHER_BRICK, 1, 0),
                    "NN",
                    "NN", { ITEM('N', ITEM_NETHER_BRICK) });

    addShapedRecipe(ItemInstance(BLOCK_QUARTZ_BLOCK, 1, 0),
                    "NN",
                    "NN", { ITEM('N', ITEM_NETHER_QUARTZ) });

    addShapedRecipe(ItemInstance(BLOCK_QUARTZ_BLOCK, 2, QZ_PILLAR),
                    "#",
                    "#", { INST('#', BLOCK_QUARTZ_BLOCK, QZ_DEFAULT) });

    addShapedRecipe(ItemInstance(BLOCK_QUARTZ_BLOCK, 1, QZ_CHISELED),
                    "#",
                    "#", { INST('#', BLOCK_SLAB, SLAB_QUARTZ) });

    {
        static const char* const shapes[4][3] = {
            { "XXX", "X X", 0     },
            { "X X", "XXX", "XXX" },
            { "XXX", "X X", "X X" },
            { "X X", "X X", 0     },
        };
        const short materialIds[4] = { ITEM_LEATHER, ITEM_IRON_INGOT, ITEM_DIAMOND, ITEM_GOLD_INGOT };
        const short map[4][4] = {
            { ITEM_HELMET_CLOTH,     ITEM_HELMET_IRON,     ITEM_HELMET_DIAMOND,     ITEM_HELMET_GOLD     },
            { ITEM_CHESTPLATE_CLOTH, ITEM_CHESTPLATE_IRON, ITEM_CHESTPLATE_DIAMOND, ITEM_CHESTPLATE_GOLD },
            { ITEM_LEGGINGS_CLOTH,   ITEM_LEGGINGS_IRON,   ITEM_LEGGINGS_DIAMOND,   ITEM_LEGGINGS_GOLD   },
            { ITEM_BOOTS_CLOTH,      ITEM_BOOTS_IRON,      ITEM_BOOTS_DIAMOND,      ITEM_BOOTS_GOLD      },
        };
        for (int m = 0; m < 4; ++m)
            for (int t = 0; t < 4; ++t) {
                if (shapes[t][2])
                    addShapedRecipe(ItemInstance(map[t][m], 1, 0),
                                    shapes[t][0], shapes[t][1], shapes[t][2],
                                    { ITEM('X', materialIds[m]) });
                else
                    addShapedRecipe(ItemInstance(map[t][m], 1, 0),
                                    shapes[t][0], shapes[t][1],
                                    { ITEM('X', materialIds[m]) });
            }
    }

    for (int i = 0; i < 16; ++i) {
        if (i == DYE_WHITE) continue;
        addShapelessRecipe(ItemInstance(BLOCK_WOOL, 1, clothData(i)),
            { ItemInstance(ITEM_BONEMEAL, 1, (short)i), ItemInstance(BLOCK_WOOL, 1, 0) });
    }

    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_YELLOW),
        { ItemInstance(BLOCK_FLOWER, 1, 0) });

    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_RED),
        { ItemInstance(BLOCK_ROSE, 1, 0) });

    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_RED),
        { ItemInstance(ITEM_BEETROOT, 1, 0) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 3, DYE_WHITE),
        { ItemInstance(ITEM_BONE, 1, 0) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_PINK),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_RED), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_ORANGE),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_RED), ItemInstance(ITEM_BONEMEAL, 1, DYE_YELLOW) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_LIME),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_GREEN), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_GRAY),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLACK), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_SILVER),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_GRAY), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });

    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 3, DYE_SILVER),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLACK), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE),
          ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_BROWN),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLACK), ItemInstance(ITEM_BONEMEAL, 1, DYE_ORANGE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 3, DYE_BROWN),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLACK), ItemInstance(ITEM_BONEMEAL, 1, DYE_RED),
          ItemInstance(ITEM_BONEMEAL, 1, DYE_YELLOW) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_LIGHT_BLUE),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLUE), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_CYAN),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLUE), ItemInstance(ITEM_BONEMEAL, 1, DYE_GREEN) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_PURPLE),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLUE), ItemInstance(ITEM_BONEMEAL, 1, DYE_RED) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_MAGENTA),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_PURPLE), ItemInstance(ITEM_BONEMEAL, 1, DYE_PINK) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 3, DYE_MAGENTA),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLUE), ItemInstance(ITEM_BONEMEAL, 1, DYE_RED),
          ItemInstance(ITEM_BONEMEAL, 1, DYE_PINK) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 4, DYE_MAGENTA),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLUE), ItemInstance(ITEM_BONEMEAL, 1, DYE_RED),
          ItemInstance(ITEM_BONEMEAL, 1, DYE_RED),  ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });

    addShapedRecipe(ItemInstance(ITEM_PAPER, 3, 0),
                    "###", { ITEM('#', ITEM_REEDS) });

    addShapedRecipe(ItemInstance(ITEM_BOOK, 1, 0),
                    "#",
                    "#",
                    "#", { ITEM('#', ITEM_PAPER) });

    addShapedRecipe(ItemInstance(BLOCK_FENCE, 2, 0),
                    "###",
                    "###", { ITEM('#', ITEM_STICK) });

    addShapedRecipe(ItemInstance(BLOCK_FENCE_GATE, 1, 0),
                    "#W#",
                    "#W#", { ITEM('#', ITEM_STICK), TILE('W', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(BLOCK_BOOKSHELF, 1, 0),
                    "###",
                    "XXX",
                    "###", { TILE('#', BLOCK_PLANKS), ITEM('X', ITEM_BOOK) });

    addShapedRecipe(ItemInstance(BLOCK_SNOW_BLOCK, 1, 0),
                    "##",
                    "##", { ITEM('#', ITEM_SNOWBALL) });

    addShapedRecipe(ItemInstance(BLOCK_CLAY, 1, 0),
                    "##",
                    "##", { ITEM('#', ITEM_CLAY) });

    addShapedRecipe(ItemInstance(BLOCK_BRICKS, 1, 0),
                    "##",
                    "##", { ITEM('#', ITEM_BRICK) });

    addShapedRecipe(ItemInstance(BLOCK_GLOWSTONE, 1, 0),
                    "##",
                    "##", { ITEM('#', ITEM_GLOWSTONE_DUST) });

    addShapedRecipe(ItemInstance(BLOCK_WOOL, 1, 0),
                    "##",
                    "##", { ITEM('#', ITEM_STRING) });

    addShapedRecipe(ItemInstance(BLOCK_TNT, 1, 0),
                    "X#X",
                    "#X#",
                    "X#X", { ITEM('X', ITEM_GUNPOWDER), TILE('#', BLOCK_SAND) });

    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_COBBLE),
                    "###", { TILE('#', BLOCK_COBBLESTONE) });
    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_STONE),
                    "###", { TILE('#', BLOCK_STONE) });
    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_SAND),
                    "###", { INST('#', BLOCK_SANDSTONE, SANDSTONE_PLAIN) });

    for (short sp = 0; sp < 4; ++sp)
        addShapedRecipe(ItemInstance(BLOCK_WOOD_SLAB, 6, sp),
                        "###", { INST('#', BLOCK_PLANKS, sp) });
    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_BRICK),
                    "###", { TILE('#', BLOCK_BRICKS) });

    for (short c = 0; c < 16; ++c)
        addShapedRecipe(ItemInstance(BLOCK_CARPET, 3, c),
                        "##", { INST('#', BLOCK_WOOL, c) });
    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_SMOOTHBRICK),
                    "###", { TILE('#', BLOCK_STONE_BRICKS) });

    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_QUARTZ),
                    "###", { TILE('#', BLOCK_QUARTZ_BLOCK) });

    addShapedRecipe(ItemInstance(BLOCK_LADDER, 2, 0),
                    "# #",
                    "###",
                    "# #", { ITEM('#', ITEM_STICK) });

    addShapedRecipe(ItemInstance(ITEM_DOOR_WOOD_ITEM, 1, 0),
                    "##",
                    "##",
                    "##", { TILE('#', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(BLOCK_TRAPDOOR, 2, 0),
                    "###",
                    "###", { TILE('#', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(ITEM_SIGN, 1, 0),
                    "###",
                    "###",
                    " X ", { TILE('#', BLOCK_PLANKS), ITEM('X', ITEM_STICK) });

    addShapedRecipe(ItemInstance(ITEM_CAKE, 1, 0),
                    "AAA",
                    "BEB",
                    "CCC", { INST('A', ITEM_BUCKET, BUCKET_MILK), ITEM('B', ITEM_SUGAR),
                             ITEM('C', ITEM_WHEAT), ITEM('E', ITEM_EGG) });

    addShapedRecipe(ItemInstance(ITEM_SUGAR, 1, 0),
                    "#", { ITEM('#', ITEM_REEDS) });

    for (short sp = 0; sp < 4; ++sp)
        addShapedRecipe(ItemInstance(BLOCK_PLANKS, 4, sp),
                        "#", { INST('#', BLOCK_LOG, sp) });

    addShapedRecipe(ItemInstance(ITEM_STICK, 4, 0),
                    "#",
                    "#", { TILE('#', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(BLOCK_TORCH, 4, 0),
                    "X",
                    "#", { ITEM('X', ITEM_COAL), ITEM('#', ITEM_STICK) });

    addShapedRecipe(ItemInstance(BLOCK_PUMPKIN_LIT, 1, 0),
                    "A",
                    "B", { TILE('A', BLOCK_PUMPKIN), TILE('B', BLOCK_TORCH) });

    addShapedRecipe(ItemInstance(BLOCK_TORCH, 4, 0),
                    "X",
                    "#", { INST('X', ITEM_COAL, COAL_CHARCOAL), ITEM('#', ITEM_STICK) });

    addShapedRecipe(ItemInstance(ITEM_BOWL, 4, 0),
                    "# #",
                    " # ", { TILE('#', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(ITEM_BUCKET, 1, BUCKET_EMPTY),
                    "# #",
                    " # ", { ITEM('#', ITEM_IRON_INGOT) });

    addShapedRecipe(ItemInstance(ITEM_FLINT_AND_STEEL, 1, 0),
                    "A ",
                    " B", { ITEM('A', ITEM_IRON_INGOT), ITEM('B', ITEM_FLINT) });

    addShapedRecipe(ItemInstance(BLOCK_RAIL, 16, 0),
                    "X X",
                    "X#X",
                    "X X", { ITEM('#', ITEM_STICK), ITEM('X', ITEM_IRON_INGOT) });

    addShapedRecipe(ItemInstance(BLOCK_GOLDEN_RAIL, 6, 0),
                    "X X",
                    "X#X",
                    "XRX", { ITEM('#', ITEM_STICK), ITEM('X', ITEM_GOLD_INGOT),
                             ITEM('R', ITEM_REDSTONE) });

    addShapedRecipe(ItemInstance(ITEM_MINECART, 1, 0),
                    "# #",
                    "###", { ITEM('#', ITEM_IRON_INGOT) });

    addShapedRecipe(ItemInstance(ITEM_CLOCK, 1, 0),
                    " # ",
                    "#X#",
                    " # ", { ITEM('#', ITEM_GOLD_INGOT), ITEM('X', ITEM_REDSTONE) });

    addShapedRecipe(ItemInstance(ITEM_COMPASS, 1, 0),
                    " # ",
                    "#X#",
                    " # ", { ITEM('#', ITEM_IRON_INGOT), ITEM('X', ITEM_REDSTONE) });

    addShapedRecipe(ItemInstance(ITEM_BREAD, 1, 0),
                    "###", { ITEM('#', ITEM_WHEAT) });

    {
        static const short kStairOf[4] = { BLOCK_STAIRS_PLANKS, BLOCK_STAIRS_SPRUCE,
                                           BLOCK_STAIRS_BIRCH,  BLOCK_STAIRS_JUNGLE };
        for (short sp = 0; sp < 4; ++sp)
            addShapedRecipe(ItemInstance(kStairOf[sp], 4, 0),
                            "#  ",
                            "## ",
                            "###", { INST('#', BLOCK_PLANKS, sp) });
    }

    addShapedRecipe(ItemInstance(BLOCK_STAIRS_COBBLESTONE, 4, 0),
                    "#  ",
                    "## ",
                    "###", { TILE('#', BLOCK_COBBLESTONE) });

    addShapedRecipe(ItemInstance(BLOCK_STAIRS_BRICK, 4, 0),
                    "#  ",
                    "## ",
                    "###", { TILE('#', BLOCK_BRICKS) });

    addShapedRecipe(ItemInstance(BLOCK_STAIRS_STONE_BRICK, 4, 0),
                    "#  ",
                    "## ",
                    "###", { TILE('#', BLOCK_STONE_BRICKS) });

    addShapedRecipe(ItemInstance(BLOCK_STAIRS_NETHER_BRICK, 4, 0),
                    "#  ",
                    "## ",
                    "###", { TILE('#', BLOCK_NETHER_BRICK) });

    addShapedRecipe(ItemInstance(BLOCK_STAIRS_QUARTZ, 4, 0),
                    "#  ",
                    "## ",
                    "###", { TILE('#', BLOCK_QUARTZ_BLOCK) });

    addShapedRecipe(ItemInstance(BLOCK_STAIRS_SANDSTONE, 4, 0),
                    "#  ",
                    "## ",
                    "###", { INST('#', BLOCK_SANDSTONE, SANDSTONE_PLAIN) });

    addShapedRecipe(ItemInstance(ITEM_PAINTING, 1, 0),
                    "###",
                    "#X#",
                    "###", { ITEM('#', ITEM_STICK), TILE('X', BLOCK_WOOL) });

    addShapedRecipe(ItemInstance(ITEM_BED_ITEM, 1, 0),
                    "###",
                    "XXX", { TILE('#', BLOCK_WOOL), TILE('X', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(BLOCK_IRON_BARS, 16, 0),
                    "###",
                    "###", { ITEM('#', ITEM_IRON_INGOT) });

    addShapedRecipe(ItemInstance(BLOCK_NETHER_REACTOR, 1, 0),
                    "X#X",
                    "X#X",
                    "X#X", { ITEM('#', ITEM_DIAMOND), ITEM('X', ITEM_IRON_INGOT) });
}
