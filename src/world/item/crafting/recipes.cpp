
#include "world/item/crafting/recipes.h"
#include "world/item/item.h"
#include <cstring>

 Recipes* Recipes::instance = nullptr;

Recipes* Recipes::getInstance() {
    if (!instance) instance = new Recipes();
    return instance;
}

Recipes::~Recipes() {
    for (unsigned int i = 0; i < recipes.size(); ++i) delete recipes[i];
}

enum {
    DYE_BLACK = 0, DYE_RED = 1, DYE_GREEN = 2, DYE_BROWN = 3, DYE_BLUE = 4,
    DYE_PURPLE = 5, DYE_CYAN = 6, DYE_SILVER = 7, DYE_GRAY = 8, DYE_PINK = 9,
    DYE_LIME = 10, DYE_YELLOW = 11, DYE_LIGHT_BLUE = 12, DYE_MAGENTA = 13,
    DYE_ORANGE = 14
};
static inline short clothData(int dyeAux) { return (short)(~dyeAux & 0xf); }

enum { SLAB_STONE = 0, SLAB_SAND = 1, SLAB_WOOD = 2, SLAB_COBBLE = 3,
       SLAB_BRICK = 4, SLAB_SMOOTHBRICK = 5 };
enum { SANDSTONE_HEIROGLYPHS = 1, SANDSTONE_SMOOTHSIDE = 2 };
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
    }

    {
        struct { short block; short item; short itemAux; } map[4] = {
            { BLOCK_GOLD_BLOCK,    ITEM_GOLD_INGOT, 0 },
            { BLOCK_IRON_BLOCK,    ITEM_IRON_INGOT, 0 },
            { BLOCK_DIAMOND_BLOCK, ITEM_DIAMOND,    0 },
            { BLOCK_LAPIS_BLOCK,   ITEM_BONEMEAL,   DYE_BLUE },
        };
        for (int i = 0; i < 4; ++i) {
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

    addShapedRecipe(ItemInstance(BLOCK_SANDSTONE, 1, 0),
                    "##",
                    "##", { TILE('#', BLOCK_SAND) });

    addShapedRecipe(ItemInstance(BLOCK_SANDSTONE, 4, SANDSTONE_SMOOTHSIDE),
                    "##",
                    "##", { TILE('#', BLOCK_SANDSTONE) });

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
        if (i == DYE_BLACK || i == DYE_BROWN || i == DYE_SILVER || i == DYE_GRAY || i == DYE_WHITE)
            continue;
        addShapelessRecipe(ItemInstance(BLOCK_WOOL, 1, clothData(i)),
            { ItemInstance(ITEM_BONEMEAL, 1, (short)i), ItemInstance(BLOCK_WOOL, 1, 0) });
    }
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_YELLOW),
        { ItemInstance(BLOCK_FLOWER, 1, 0) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 3, DYE_WHITE),
        { ItemInstance(ITEM_BONE, 1, 0) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_PINK),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_RED), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_ORANGE),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_RED), ItemInstance(ITEM_BONEMEAL, 1, DYE_YELLOW) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_LIME),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_GREEN), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_LIGHT_BLUE),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLUE), ItemInstance(ITEM_BONEMEAL, 1, DYE_WHITE) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_CYAN),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLUE), ItemInstance(ITEM_BONEMEAL, 1, DYE_GREEN) });
    addShapelessRecipe(ItemInstance(ITEM_BONEMEAL, 2, DYE_PURPLE),
        { ItemInstance(ITEM_BONEMEAL, 1, DYE_BLUE), ItemInstance(ITEM_BONEMEAL, 1, DYE_RED) });
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
                    "###", { TILE('#', BLOCK_SANDSTONE) });
    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_WOOD),
                    "###", { TILE('#', BLOCK_PLANKS) });
    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_BRICK),
                    "###", { TILE('#', BLOCK_BRICKS) });
    addShapedRecipe(ItemInstance(BLOCK_SLAB, 6, SLAB_SMOOTHBRICK),
                    "###", { TILE('#', BLOCK_STONE_BRICKS) });

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

    addShapedRecipe(ItemInstance(ITEM_SUGAR, 1, 0),
                    "#", { ITEM('#', ITEM_REEDS) });

    addShapedRecipe(ItemInstance(BLOCK_PLANKS, 4, 0),
                    "#", { TILE('#', BLOCK_LOG) });

    addShapedRecipe(ItemInstance(ITEM_STICK, 4, 0),
                    "#",
                    "#", { TILE('#', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(BLOCK_TORCH, 4, 0),
                    "X",
                    "#", { ITEM('X', ITEM_COAL), ITEM('#', ITEM_STICK) });

    addShapedRecipe(ItemInstance(BLOCK_TORCH, 4, 0),
                    "X",
                    "#", { INST('X', ITEM_COAL, COAL_CHARCOAL), ITEM('#', ITEM_STICK) });

    addShapedRecipe(ItemInstance(ITEM_BOWL, 4, 0),
                    "# #",
                    " # ", { TILE('#', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(ITEM_FLINT_AND_STEEL, 1, 0),
                    "A ",
                    " B", { ITEM('A', ITEM_IRON_INGOT), ITEM('B', ITEM_FLINT) });

    addShapedRecipe(ItemInstance(ITEM_BREAD, 1, 0),
                    "###", { ITEM('#', ITEM_WHEAT) });

    addShapedRecipe(ItemInstance(BLOCK_STAIRS_PLANKS, 4, 0),
                    "#  ",
                    "## ",
                    "###", { TILE('#', BLOCK_PLANKS) });

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
                    "###", { TILE('#', BLOCK_SANDSTONE) });

    addShapedRecipe(ItemInstance(ITEM_PAINTING, 1, 0),
                    "###",
                    "#X#",
                    "###", { ITEM('#', ITEM_STICK), TILE('X', BLOCK_WOOL) });

    addShapedRecipe(ItemInstance(ITEM_BED_ITEM, 1, 0),
                    "###",
                    "XXX", { TILE('#', BLOCK_WOOL), TILE('X', BLOCK_PLANKS) });

    addShapedRecipe(ItemInstance(BLOCK_NETHER_REACTOR, 1, 0),
                    "X#X",
                    "X#X",
                    "X#X", { ITEM('#', ITEM_DIAMOND), ITEM('X', ITEM_IRON_INGOT) });
}
