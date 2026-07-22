#include "client/gui/hud.h"
#include "world/item/item.h"
#include "world/item/spawn_egg_item.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "client/player/player.h"
#include "world/level/chunk/chunk.h"
#include "gpu/gu.h"
#include "gpu/texture.h"
#include "gpu/sprite.h"
#include "gpu/button_icons.h"
#include <pspgu.h>
#include <pspkernel.h>
#include "platform/time.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include "gpu/item_icons.h"
#include "gpu/spawn_egg_colors.h"

#define CHAT_LINES 4
#define CHAT_SHOW_S 10.0f
static char  s_chat[CHAT_LINES][64];
static float s_chatTime[CHAT_LINES];

void hudChatMessage(const char* msg) {
    for (int i = 0; i < CHAT_LINES - 1; i++) {
        strcpy(s_chat[i], s_chat[i + 1]);
        s_chatTime[i] = s_chatTime[i + 1];
    }
    strncpy(s_chat[CHAT_LINES - 1], msg, sizeof(s_chat[0]) - 1);
    s_chat[CHAT_LINES - 1][sizeof(s_chat[0]) - 1] = '\0';
    s_chatTime[CHAT_LINES - 1] = nowSeconds();
}

extern Texture g_terrain;
extern bool    g_haveTerrain;
extern Texture g_guiBlocks;
extern bool    g_haveGuiBlocks;

#define HUD_S   2.0f
#define HUD_N   (HOTBAR_SLOTS + 1)

#define HB_S       2.0f

#define HB_BOTTOM  18.0f
#define HUD_HOTBAR_TOP (272.0f - 22.0f * HB_S - HB_BOTTOM)

#define HUD_HINT_S  UI_HINT_S

#define HUD_ST_S    2.0f
#define HUD_HINTS_Y UI_HINTS_Y

int g_barOnTop = 0;
static const unsigned int HUD_WHITE = 0xFFFFFFFFu;

static short guiBlockIcon(short id) {
    switch (id) {
        case BLOCK_STONE: return 7;
        case BLOCK_GRASS: return 9;
        case BLOCK_DIRT: return 8;
        case BLOCK_COBBLESTONE: return 0;
        case BLOCK_PLANKS: return 5;
        case BLOCK_SAND: return 14;
        case BLOCK_GRAVEL: return 15;
        case BLOCK_CLAY: return 10;
        case BLOCK_ORE_GOLD: return 39;
        case BLOCK_ORE_IRON: return 38;
        case BLOCK_ORE_COAL: return 37;
        case BLOCK_ORE_LAPIS: return 41;
        case BLOCK_LAPIS_BLOCK: return 46;
        case BLOCK_ORE_REDSTONE: case BLOCK_ORE_REDSTONE_LIT: return 42;
        case BLOCK_ORE_EMERALD: return 40;
        case BLOCK_DIAMOND_BLOCK: return 45;
        case BLOCK_GOLD_BLOCK: return 43;
        case BLOCK_IRON_BLOCK: return 44;
        case BLOCK_BRICKS: return 6;
        case BLOCK_MOSSY_COBBLE: return 4;
        case BLOCK_OBSIDIAN: return 47;
        case BLOCK_SNOW_BLOCK: return 48;
        case BLOCK_NETHERRACK: return 20;
        case BLOCK_GLOWSTONE: return 50;
        case BLOCK_NETHER_BRICK: return 19;
        case BLOCK_TNT: return 76;
        case BLOCK_BOOKSHELF: return 71;
        case BLOCK_GLASS: return 49;
        case BLOCK_MELON: return 78;
        case BLOCK_STONECUTTER: return 73;
        case BLOCK_CRAFTING_TABLE: return 72;
        case BLOCK_FURNACE: case BLOCK_FURNACE_LIT: return 75;
        case BLOCK_CHEST: return 74;
        case BLOCK_NETHER_REACTOR: return 51;
        case BLOCK_CACTUS: return 77;
        case BLOCK_REEDS: return 138;
        case BLOCK_FLOWER: return 134;
        case BLOCK_ROSE: return 135;
        case BLOCK_MUSHROOM_BROWN: return 136;
        case BLOCK_MUSHROOM_RED: return 137;
        case BLOCK_FARMLAND: return 8;

        case BLOCK_STAIRS_COBBLESTONE: return 21;
        case BLOCK_STAIRS_PLANKS: return 22;
        case BLOCK_STAIRS_BRICK: return 23;
        case BLOCK_STAIRS_SANDSTONE: return 24;
        case BLOCK_STAIRS_STONE_BRICK: return 25;
        case BLOCK_STAIRS_NETHER_BRICK: return 26;
        case BLOCK_STAIRS_QUARTZ: return 27;

        case BLOCK_GLASS_PANE: return 130;
        case BLOCK_FENCE: return 69;
        case BLOCK_DOOR_WOOD: return 131;
        case BLOCK_TRAPDOOR: return 68;
        case BLOCK_FENCE_GATE: return 70;
        case BLOCK_BED: return 132;
        default: return -1;
    }
}

static inline int logGuiIcon(unsigned char data) {
    switch (data & LOG_TYPE_MASK) {
        case LOG_SPRUCE: return 17;
        case LOG_BIRCH:  return 18;
        default:         return 16;
    }
}
static inline int sandstoneGuiIcon(unsigned char data) {
    switch (data) {
        case SS_CHISELED: return 12;
        case SS_SMOOTH:   return 13;
        default:          return 11;
    }
}
static inline int quartzGuiIcon(unsigned char data) {
    switch (data) {
        case QZ_CHISELED: return 36;
        case QZ_PILLAR:   return 35;
        default:          return 34;
    }
}
static inline int stoneBrickGuiIcon(unsigned char data) {
    switch (data) {
        case SB_MOSSY:   return 2;
        case SB_CRACKED: return 3;
        default:         return 1;
    }
}
static inline int slabGuiIcon(unsigned char data) {
    switch (data & DSLAB_MAT_MASK) {
        case DSLAB_SAND:        return 32;
        case DSLAB_WOOD:        return 30;
        case DSLAB_COBBLE:      return 29;
        case DSLAB_BRICK:       return 31;
        case DSLAB_SMOOTHBRICK: return 33;
        default:                return 28;
    }
}

static inline int leafGuiIcon(unsigned char data) {
    switch (data & LEAF_TYPE_MASK) {
        case LEAF_SPRUCE: return 80;
        case LEAF_BIRCH:  return 81;
        default:          return 79;
    }
}

static inline int saplingGuiIcon(unsigned char data) {
    switch (data & LEAF_TYPE_MASK) {
        case LEAF_SPRUCE: return 140;
        case LEAF_BIRCH:  return 141;
        default:          return 139;
    }
}

static const short kWoolGuiIcon[16] = {
    52, 59, 58, 57, 56, 55, 54, 53,
    67, 66, 65, 64, 63, 62, 61, 60,
};

int getGuiBlockIcon(short id, unsigned char data) {
    if (id >= 256) return -2;
    return isLeaf(id) ? leafGuiIcon(data)
         : (id == BLOCK_SAPLING) ? saplingGuiIcon(data)
         : isWool(id) ? kWoolGuiIcon[data & 0xF]
         : isLog(id) ? logGuiIcon(data)
         : (id == BLOCK_SANDSTONE) ? sandstoneGuiIcon(data)
         : (id == BLOCK_QUARTZ_BLOCK) ? quartzGuiIcon(data)
         : (id == BLOCK_STONE_BRICKS) ? stoneBrickGuiIcon(data)
         : isSlab(id) ? slabGuiIcon(data)
         : guiBlockIcon(id);
}

int itemFlatIcon(short id, unsigned char data) {
    if (id < 256 || id >= 512) return -1;
    if (id == ITEM_COAL)      return kItemIconCoal[data & 0xF];
    if (id == ITEM_BONEMEAL)  return kItemIconDye[data & 0xF];
    if (id == ITEM_SPAWN_EGG) return II_SPAWN_EGG_BASE;
    return kItemIcon[id - 256];
}

void drawFlatIcon(int icon, float x, float y, float sizePx, unsigned int tint) {
    if (icon < 0 || !g_haveGuiBlocks) return;
    textureBind(&g_guiBlocks);
    spriteDraw(&g_guiBlocks, x, y, sizePx, sizePx,
               (icon & 31) * 16.0f, (27 + (icon >> 5)) * 16.0f, 16.0f, 16.0f, tint);
}

void drawBlockIcon(short id, unsigned char data, float x, float y, float sizePx, unsigned int colorTint) {
    if (id >= 256) {
        if (id == ITEM_SPAWN_EGG) {

            unsigned int base, spot;
            spawnEggColors(data, &base, &spot);
            drawFlatIcon(II_SPAWN_EGG_BASE,    x, y, sizePx, eggMul(base, colorTint));
            drawFlatIcon(II_SPAWN_EGG_OVERLAY, x, y, sizePx, eggMul(spot, colorTint));
            return;
        }
        drawFlatIcon(itemFlatIcon(id, data), x, y, sizePx, colorTint);
        return;
    }
    int i = getGuiBlockIcon(id, data);
    if (i >= 0 && g_haveGuiBlocks) {

        float sx, sy, ss;
        if (i < 128) { ss = 48.0f; sx = (i % 10) * 48.0f;         sy = (i / 10) * 48.0f; }
        else { i -= 128; ss = 16.0f; sx = (i & 31) * 16.0f; sy = (27 + (i >> 5)) * 16.0f; }
        textureBind(&g_guiBlocks);
        spriteDraw(&g_guiBlocks, x, y, sizePx, sizePx, sx, sy, ss, ss, colorTint);
    } else if (g_haveTerrain) {
        int col, row; unsigned int tint;
        tileForBlock(id, data, F_TOP, &col, &row, &tint);

        if (colorTint != HUD_WHITE) {
            int b1 = (tint >> 16) & 0xFF, g1 = (tint >> 8) & 0xFF, r1 = tint & 0xFF, a1 = (tint >> 24) & 0xFF;
            int b2 = (colorTint >> 16) & 0xFF, g2 = (colorTint >> 8) & 0xFF, r2 = colorTint & 0xFF, a2 = (colorTint >> 24) & 0xFF;
            tint = ((a1 * a2 / 255) << 24) | ((b1 * b2 / 255) << 16) | ((g1 * g2 / 255) << 8) | (r1 * r2 / 255);
        }

        textureBind(&g_terrain);
        spriteDraw(&g_terrain, x, y, sizePx, sizePx, col * 16.0f, row * 16.0f, 16.0f, 16.0f, tint);
    }
}

const char* getBlockName(short id, unsigned char data) {

    if (id >= 256) {
        switch (id) {
            case ITEM_HOE_IRON: return "Iron Hoe";
            case ITEM_SEEDS_WHEAT: return "Wheat Seeds";
            case ITEM_SEEDS_MELON: return "Melon Seeds";
            case ITEM_PAINTING: return "Painting";
            case ITEM_SIGN: return "Sign";
            case ITEM_BOW: return "Bow";

            case ITEM_SWORD_WOOD: return "Wooden Sword";
            case ITEM_SWORD_STONE: return "Stone Sword";
            case ITEM_SWORD_IRON: return "Iron Sword";
            case ITEM_SWORD_DIAMOND: return "Diamond Sword";
            case ITEM_SWORD_GOLD: return "Golden Sword";
            case ITEM_SHOVEL_WOOD: return "Wooden Shovel";
            case ITEM_SHOVEL_STONE: return "Stone Shovel";
            case ITEM_SHOVEL_IRON: return "Iron Shovel";
            case ITEM_SHOVEL_DIAMOND: return "Diamond Shovel";
            case ITEM_SHOVEL_GOLD: return "Golden Shovel";
            case ITEM_PICKAXE_WOOD: return "Wooden Pickaxe";
            case ITEM_PICKAXE_STONE: return "Stone Pickaxe";
            case ITEM_PICKAXE_IRON: return "Iron Pickaxe";
            case ITEM_PICKAXE_DIAMOND: return "Diamond Pickaxe";
            case ITEM_PICKAXE_GOLD: return "Golden Pickaxe";
            case ITEM_HATCHET_WOOD: return "Wooden Axe";
            case ITEM_HATCHET_STONE: return "Stone Axe";
            case ITEM_HATCHET_IRON: return "Iron Axe";
            case ITEM_HATCHET_DIAMOND: return "Diamond Axe";
            case ITEM_HATCHET_GOLD: return "Golden Axe";
            case ITEM_HOE_WOOD: return "Wooden Hoe";
            case ITEM_HOE_STONE: return "Stone Hoe";

            case ITEM_HOE_DIAMOND: return "Diamond Hoe";
            case ITEM_HOE_GOLD: return "Golden Hoe";
            case ITEM_FLINT_AND_STEEL: return "Flint and Steel";
            case ITEM_SHEARS: return "Shears";
            case ITEM_COMPASS: return "Compass";
            case ITEM_CLOCK: return "Clock";
            case ITEM_CAMERA: return "Camera";

            case ITEM_HELMET_CLOTH: return "Leather Cap";
            case ITEM_CHESTPLATE_CLOTH: return "Leather Tunic";
            case ITEM_LEGGINGS_CLOTH: return "Leather Pants";
            case ITEM_BOOTS_CLOTH: return "Leather Boots";
            case ITEM_HELMET_CHAIN: return "Chain Helmet";
            case ITEM_CHESTPLATE_CHAIN: return "Chain Chestplate";
            case ITEM_LEGGINGS_CHAIN: return "Chain Leggings";
            case ITEM_BOOTS_CHAIN: return "Chain Boots";
            case ITEM_HELMET_IRON: return "Iron Helmet";
            case ITEM_CHESTPLATE_IRON: return "Iron Chestplate";
            case ITEM_LEGGINGS_IRON: return "Iron Leggings";
            case ITEM_BOOTS_IRON: return "Iron Boots";
            case ITEM_HELMET_DIAMOND: return "Diamond Helmet";
            case ITEM_CHESTPLATE_DIAMOND: return "Diamond Chestplate";
            case ITEM_LEGGINGS_DIAMOND: return "Diamond Leggings";
            case ITEM_BOOTS_DIAMOND: return "Diamond Boots";
            case ITEM_HELMET_GOLD: return "Golden Helmet";
            case ITEM_CHESTPLATE_GOLD: return "Golden Chestplate";
            case ITEM_LEGGINGS_GOLD: return "Golden Leggings";
            case ITEM_BOOTS_GOLD: return "Golden Boots";

            case ITEM_APPLE: return "Apple";
            case ITEM_BREAD: return "Bread";
            case ITEM_MUSHROOM_STEW: return "Mushroom Stew";
            case ITEM_PORKCHOP_RAW: return "Raw Porkchop";
            case ITEM_PORKCHOP_COOKED: return "Cooked Porkchop";
            case ITEM_MELON: return "Melon";
            case ITEM_BEEF_RAW: return "Raw Beef";
            case ITEM_BEEF_COOKED: return "Steak";
            case ITEM_CHICKEN_RAW: return "Raw Chicken";
            case ITEM_CHICKEN_COOKED: return "Cooked Chicken";

            case ITEM_COAL: return data == 1 ? "Charcoal" : "Coal";
            case ITEM_DIAMOND: return "Diamond";
            case ITEM_IRON_INGOT: return "Iron Ingot";
            case ITEM_GOLD_INGOT: return "Gold Ingot";
            case ITEM_STICK: return "Stick";
            case ITEM_BOWL: return "Bowl";
            case ITEM_STRING: return "String";
            case ITEM_FEATHER: return "Feather";
            case ITEM_GUNPOWDER: return "Gunpowder";
            case ITEM_WHEAT: return "Wheat";
            case ITEM_FLINT: return "Flint";
            case ITEM_LEATHER: return "Leather";
            case ITEM_BRICK: return "Brick";
            case ITEM_CLAY: return "Clay";
            case ITEM_PAPER: return "Paper";
            case ITEM_BOOK: return "Book";
            case ITEM_SLIMEBALL: return "Slimeball";
            case ITEM_GLOWSTONE_DUST: return "Glowstone Dust";
            case ITEM_BONE: return "Bone";
            case ITEM_SUGAR: return "Sugar";
            case ITEM_NETHER_BRICK: return "Nether Brick";
            case ITEM_NETHER_QUARTZ: return "Nether Quartz";
            case ITEM_ARROW: return "Arrow";
            case ITEM_EGG: return "Egg";
            case ITEM_SNOWBALL: return "Snowball";

            case ITEM_BONEMEAL: {
                static const char* dye[16] = {
                    "Ink Sac", "Rose Red", "Cactus Green", "Cocoa Beans", "Lapis Lazuli",
                    "Purple Dye", "Cyan Dye", "Light Gray Dye", "Gray Dye", "Pink Dye",
                    "Lime Dye", "Dandelion Yellow", "Light Blue Dye", "Magenta Dye",
                    "Orange Dye", "Bone Meal"
                };
                return dye[data & 0xF];
            }

            case ITEM_SPAWN_EGG:
                switch (data) {
                    case 12: return "Spawn Pig";
                    case 11: return "Spawn Cow";
                    case 10: return "Spawn Chicken";
                    case 13: return "Spawn Sheep";
                    case 32: return "Spawn Zombie";
                    case 33: return "Spawn Creeper";
                    case 34: return "Spawn Skeleton";
                    case 35: return "Spawn Spider";
                    case 36: return "Spawn Zombie Pigman";
                    default: return "Spawn Egg";
                }
            case ITEM_BED_ITEM: return "Bed";
            case ITEM_REEDS: return "Sugar Cane";
            case ITEM_DOOR_WOOD_ITEM: return "Wooden Door";
            case ITEM_DOOR_IRON_ITEM: return "Iron Door";
            default: return "Item";
        }
    }
    switch ((unsigned char)id) {
        case BLOCK_COBBLESTONE: return "Cobblestone";
        case BLOCK_STONE: return "Stone";
        case BLOCK_MOSSY_COBBLE: return "Moss Stone";
        case BLOCK_PLANKS: return "Wooden Planks";
        case BLOCK_BRICKS: return "Bricks";
        case BLOCK_STONE_BRICKS:
            switch (data) {
                case SB_MOSSY:   return "Mossy Stone Bricks";
                case SB_CRACKED: return "Cracked Stone Bricks";
                default:         return "Stone Bricks";
            }
        case BLOCK_DIRT: return "Dirt";
        case BLOCK_GRASS: return "Grass Block";
        case BLOCK_FARMLAND: return "Farmland";

        case BLOCK_LADDER: return "Ladder";
        case BLOCK_COBWEB: return "Cobweb";
        case BLOCK_TORCH: return "Torch";
        case BLOCK_BED: return "Bed";
        case BLOCK_CLAY: return "Clay";
        case BLOCK_SANDSTONE:
            switch (data) {
                case SS_CHISELED: return "Chiseled Sandstone";
                case SS_SMOOTH:   return "Smooth Sandstone";
                default:          return "Sandstone";
            }
        case BLOCK_SAND: return "Sand";
        case BLOCK_GRAVEL: return "Gravel";
        case BLOCK_LOG:
            switch (data & LOG_TYPE_MASK) {
                case LOG_SPRUCE: return "Spruce Wood";
                case LOG_BIRCH:  return "Birch Wood";
                default:         return "Wood";
            }
        case BLOCK_NETHER_BRICK: return "Nether Bricks";

        case BLOCK_NETHERRACK: return "Netherrack";
        case BLOCK_STAIRS_COBBLESTONE: return "Cobblestone Stairs";
        case BLOCK_STAIRS_PLANKS: return "Wooden Stairs";
        case BLOCK_STAIRS_BRICK: return "Brick Stairs";
        case BLOCK_STAIRS_SANDSTONE: return "Sandstone Stairs";
        case BLOCK_STAIRS_STONE_BRICK: return "Stone Brick Stairs";
        case BLOCK_STAIRS_NETHER_BRICK: return "Nether Brick Stairs";
        case BLOCK_STAIRS_QUARTZ: return "Quartz Stairs";
        case BLOCK_SLAB:
            switch (data & DSLAB_MAT_MASK) {
                case DSLAB_SAND:        return "Sandstone Slab";
                case DSLAB_WOOD:        return "Wooden Slab";
                case DSLAB_COBBLE:      return "Cobblestone Slab";
                case DSLAB_BRICK:       return "Brick Slab";
                case DSLAB_SMOOTHBRICK: return "Stone Brick Slab";
                default:                return "Stone Slab";
            }
        case BLOCK_QUARTZ_BLOCK:
            switch (data) {
                case QZ_PILLAR:   return "Pillar Quartz Block";
                case QZ_CHISELED: return "Chiseled Quartz Block";
                default:          return "Block of Quartz";
            }
        case BLOCK_ORE_COAL: return "Coal Ore";
        case BLOCK_ORE_IRON: return "Iron Ore";
        case BLOCK_ORE_GOLD: return "Gold Ore";

        case BLOCK_ORE_EMERALD: return "Diamond Ore";
        case BLOCK_ORE_LAPIS: return "Lapis Lazuli Ore";
        case BLOCK_ORE_REDSTONE: return "Redstone Ore";
        case BLOCK_GOLD_BLOCK: return "Block of Gold";
        case BLOCK_IRON_BLOCK: return "Block of Iron";
        case BLOCK_DIAMOND_BLOCK: return "Block of Diamond";
        case BLOCK_LAPIS_BLOCK: return "Lapis Lazuli Block";
        case BLOCK_OBSIDIAN: return "Obsidian";
        case BLOCK_GLOWING_OBSIDIAN: return "Glowing Obsidian";
        case BLOCK_TALLGRASS:
            switch (data) {
                case TG_TALL_GRASS: return "Tall Grass";
                case TG_FERN:       return "Fern";
                default:            return "Dead Bush";
            }
        case BLOCK_SNOW_BLOCK: return "Snow Block";
        case BLOCK_GLASS: return "Glass";
        case BLOCK_GLASS_PANE: return "Glass Pane";
        case BLOCK_FENCE: return "Oak Fence";
        case BLOCK_DOOR_WOOD: return "Wooden Door";
        case BLOCK_DOOR_IRON: return "Iron Door";
        case BLOCK_TRAPDOOR: return "Trapdoor";
        case BLOCK_FENCE_GATE: return "Fence Gate";

        case BLOCK_GLOWSTONE: return "Glowstone";
        case BLOCK_NETHER_REACTOR: return "Nether Reactor Core";
        case BLOCK_WOOL: {
            switch (data & 0xF) {
                case 0: return "White Wool";
                case 1: return "Orange Wool";
                case 2: return "Magenta Wool";
                case 3: return "Light Blue Wool";
                case 4: return "Yellow Wool";
                case 5: return "Lime Wool";
                case 6: return "Pink Wool";
                case 7: return "Gray Wool";
                case 8: return "Light Gray Wool";
                case 9: return "Cyan Wool";
                case 10: return "Purple Wool";
                case 11: return "Blue Wool";
                case 12: return "Brown Wool";
                case 13: return "Green Wool";
                case 14: return "Red Wool";
                case 15: return "Black Wool";
                default: return "Wool";
            }
        }

        case BLOCK_BOOKSHELF: return "Bookshelf";
        case BLOCK_CRAFTING_TABLE: return "Crafting Table";
        case BLOCK_STONECUTTER: return "Stonecutter";
        case BLOCK_CHEST: return "Chest";
        case BLOCK_FURNACE: return "Furnace";
        case BLOCK_TNT: return "TNT";
        case BLOCK_CACTUS: return "Cactus";
        case BLOCK_MELON: return "Melon";
        case BLOCK_LEAVES: {
            int t = data & LEAF_TYPE_MASK;
            if (t == LEAF_SPRUCE) return "Spruce Leaves";
            if (t == LEAF_BIRCH) return "Birch Leaves";
            return "Leaves";
        }

        case BLOCK_FLOWER: return "Dandelion";
        case BLOCK_ROSE: return "Rose";
        case BLOCK_MUSHROOM_BROWN: return "Brown Mushroom";
        case BLOCK_MUSHROOM_RED: return "Red Mushroom";
        case BLOCK_SAPLING: {
            int t = data & LEAF_TYPE_MASK;
            if (t == LEAF_SPRUCE) return "Spruce Sapling";
            if (t == LEAF_BIRCH) return "Birch Sapling";
            return "Sapling";
        }
        case BLOCK_REEDS: return "Sugar Cane";

        default: return "Block";
    }
}

const char* getBlockDescription(short id, unsigned char data) {
    if (id >= 256) {
        switch (id) {

            case ITEM_SWORD_WOOD: case ITEM_SWORD_STONE: case ITEM_SWORD_IRON:
            case ITEM_SWORD_DIAMOND: case ITEM_SWORD_GOLD:
                return "Deals more damage than by hand.";
            case ITEM_SHOVEL_WOOD: case ITEM_SHOVEL_STONE: case ITEM_SHOVEL_IRON:
            case ITEM_SHOVEL_DIAMOND: case ITEM_SHOVEL_GOLD:
                return "Used to dig dirt, grass, sand, gravel and snow faster than by hand.";
            case ITEM_PICKAXE_WOOD: case ITEM_PICKAXE_STONE: case ITEM_PICKAXE_IRON:
            case ITEM_PICKAXE_DIAMOND: case ITEM_PICKAXE_GOLD:
                return "Required to mine stone-related blocks and ore.";
            case ITEM_HATCHET_WOOD: case ITEM_HATCHET_STONE: case ITEM_HATCHET_IRON:
            case ITEM_HATCHET_DIAMOND: case ITEM_HATCHET_GOLD:
                return "Used to chop wood-related blocks faster than by hand.";
            case ITEM_HOE_WOOD: case ITEM_HOE_STONE: case ITEM_HOE_IRON:
            case ITEM_HOE_DIAMOND: case ITEM_HOE_GOLD:
                return "Used to till dirt and grass blocks to prepare for crops.";
            case ITEM_FLINT_AND_STEEL: return "Used to detonate TNT.";
            case ITEM_SHEARS: return "Used to obtain wool from sheep and to harvest placeable Leaf blocks.";
            case ITEM_COMPASS: return "Points to your start point.";
            case ITEM_CLOCK: return "Displays positions of the Sun and Moon.";
            case ITEM_BOW: return "Allows for ranged attacks by using arrows.";
            case ITEM_SIGN: return "Shows text entered by you or other players.";
            case ITEM_PAINTING: return "Used as decoration.";

            case ITEM_HELMET_CLOTH: case ITEM_HELMET_CHAIN: case ITEM_HELMET_IRON:
            case ITEM_HELMET_DIAMOND: case ITEM_HELMET_GOLD:
                return "Gives the user 1.5 Armor when worn.";
            case ITEM_CHESTPLATE_CLOTH: case ITEM_CHESTPLATE_CHAIN: case ITEM_CHESTPLATE_IRON:
            case ITEM_CHESTPLATE_DIAMOND: case ITEM_CHESTPLATE_GOLD:
                return "Gives the user 4 Armor when worn.";
            case ITEM_LEGGINGS_CLOTH: case ITEM_LEGGINGS_CHAIN: case ITEM_LEGGINGS_IRON:
            case ITEM_LEGGINGS_DIAMOND: case ITEM_LEGGINGS_GOLD:
                return "Gives the user 3 Armor when worn.";
            case ITEM_BOOTS_CLOTH: case ITEM_BOOTS_CHAIN: case ITEM_BOOTS_IRON:
            case ITEM_BOOTS_DIAMOND: case ITEM_BOOTS_GOLD:
                return "Gives the user 1.5 Armor when worn.";

            case ITEM_APPLE: return "Restores health, and can be crafted into a golden apple.";
            case ITEM_BREAD: return "Restores 2.5 Hearts.";
            case ITEM_MUSHROOM_STEW: return "Restores 5 hearts.";
            case ITEM_PORKCHOP_RAW: return "Collected by killing a pig, and can be cooked in a furnace. Restores health.";
            case ITEM_PORKCHOP_COOKED: return "Created by cooking a porkchop in a furnace. Restores health.";
            case ITEM_MELON: return "Can be cut up and eaten.";
            case ITEM_BEEF_RAW: return "Collected by killing a cow, and can be cooked in a furnace. Restores health.";
            case ITEM_BEEF_COOKED: return "Created by cooking a beef in a furnace. Restores health.";
            case ITEM_CHICKEN_RAW: return "Collected by killing a chicken, and can be cooked in a furnace. Restores health.";
            case ITEM_CHICKEN_COOKED: return "Created by cooking a chicken in a furnace. Restores health.";

            case ITEM_COAL: return data == 1 ? "Used as a renewable fuel in a furnace, or crafted to make a torch."
                                             : "Used as a fuel in a furnace, or crafted to make a torch.";
            case ITEM_DIAMOND: return "Use these to create very strong tools, weapons or armor";
            case ITEM_IRON_INGOT: case ITEM_GOLD_INGOT:
                return "A shiny ingot which can be used to craft tools made from this material.";
            case ITEM_STICK: return "Used to craft torches, arrows, signs, ladders, fences and as handles for tools and weapons.";
            case ITEM_BOWL: return "Used to hold mushroom stew. You keep the bowl when the stew has been eaten.";
            case ITEM_STRING: return "Collected by killing a spider, and can be crafted into a bow.";
            case ITEM_FEATHER: return "Collected by killing a chicken, and can be crafted into an arrow.";
            case ITEM_GUNPOWDER: return "Collected by killing a creeper, and can be crafted into TNT.";
            case ITEM_WHEAT: return "Harvested from crops, and can be used to craft food items.";
            case ITEM_FLINT: return "Collected by digging gravel, and can be used to craft a flint and steel.";
            case ITEM_LEATHER: return "Collected by killing a cow, and can be crafted into armor.";
            case ITEM_BRICK: return "Baked from clay in a furnace.";
            case ITEM_CLAY: return "Can be baked into bricks in a furnace.";
            case ITEM_PAPER: return "Used to create books and maps.";
            case ITEM_BOOK: return "Used to create a bookshelf.";
            case ITEM_SLIMEBALL: return "Collected by killing a slime.";
            case ITEM_GLOWSTONE_DUST: return "Collected by mining Glowstone, and can be crafted to make Glowstone blocks again.";
            case ITEM_BONE: return "Collected by killing a skeleton. Can be crafted into bone meal.";
            case ITEM_SUGAR: return "Used in the cake recipe.";
            case ITEM_NETHER_BRICK: return "Used to form blocks of Nether bricks.";
            case ITEM_NETHER_QUARTZ: return "Quartz from the Nether, used to create Blocks of Quartz.";
            case ITEM_ARROW: return "Used as ammunition for bows.";
            case ITEM_EGG: return "Dropped randomly by chickens, and can be crafted into food items.";
            case ITEM_SNOWBALL: return "Collected by digging snow, and can be thrown.";
            case ITEM_BONEMEAL: {
                static const char* dye[16] = {
                    "Used as a dye to create black wool.", "Used as a dye to create red wool.",
                    "Used as a dye to create green wool.", "Used as a dye to create brown wool.",
                    "Used as dye to create Blue Wool.", "Used as a dye to create purple wool.",
                    "Used as a dye to create cyan wool.", "Used as a dye to create light gray wool.",
                    "Used as a dye to create gray wool.", "Used as a dye to create pink wool.",
                    "Used as a dye to create lime wool.", "Used as a dye to create yellow wool.",
                    "Used as a dye to create light blue wool.", "Used as a dye to create magenta wool.",
                    "Used as a dye to create orange wool.",
                    "Used to instantly grow crops and flowers. Can be used in dye recipes."
                };
                return dye[data & 0xF];
            }
            case ITEM_DOOR_WOOD_ITEM: return "Wooden doors are activated by using, hitting them or with Redstone.";
            case ITEM_DOOR_IRON_ITEM: return "Iron doors can only be opened by Redstone, buttons or switches.";
            default: return getBlockName(id, data);
        }
    }
    switch ((unsigned char)id) {
        case BLOCK_STONE: return "Can be mined with a pickaxe to collect cobblestone.";
        case BLOCK_MOSSY_COBBLE: return "Can be used for construction and decoration.";
        case BLOCK_PLANKS: return "Used as a building material and can be crafted into many things. Can be crafted from any wood.";
        case BLOCK_STONE_BRICKS: return "Used as building material.";
        case BLOCK_DIRT: return "Collected using a shovel. Can be used for construction.";
        case BLOCK_FARMLAND: return "Ground that has been prepared ready to plant seeds.";
        case BLOCK_LADDER: return "Used to climb vertically.";
        case BLOCK_TORCH: return "Used to create light. Torches also melt snow and ice.";
        case BLOCK_BED: return "Used to sleep until dawn. Changes your spawn point to the bed's position.";
        case BLOCK_CLAY: return "Can be baked into bricks in a furnace.";
        case BLOCK_SANDSTONE: return "Used as a building material.";
        case BLOCK_SAND: return "Collected using a shovel. Can be smelted into glass using the furnace. Is affected by gravity if there is no other tile underneath it.";
        case BLOCK_GRAVEL: return "Collected using a shovel. Sometimes produces flint when dug up. Is affected by gravity if there is no other tile underneath it.";
        case BLOCK_LOG: return "Chopped using an axe, and can be crafted into planks or used as a fuel.";
        case BLOCK_NETHER_BRICK: return "Construction block made from Nether bricks.";
        case BLOCK_NETHERRACK: return "Is a stone material from the Nether.";
        case BLOCK_STAIRS_COBBLESTONE: case BLOCK_STAIRS_PLANKS: case BLOCK_STAIRS_BRICK:
        case BLOCK_STAIRS_SANDSTONE: case BLOCK_STAIRS_STONE_BRICK: case BLOCK_STAIRS_NETHER_BRICK:
        case BLOCK_STAIRS_QUARTZ:
            return "Used for compact staircases.";
        case BLOCK_SLAB: return "Used for making long staircases.";
        case BLOCK_QUARTZ_BLOCK: return "Decorative block, used to create other kinds of Quartz blocks.";
        case BLOCK_ORE_COAL: return "Can be mined with a pickaxe to collect coal.";
        case BLOCK_ORE_IRON: return "Can be mined with a stone pickaxe or better, then smelted in a furnace to produce iron ingots.";
        case BLOCK_ORE_GOLD: return "Can be mined with an iron pickaxe or better, then smelted in a furnace to produce gold ingots.";
        case BLOCK_ORE_EMERALD: return "Can be mined with an iron pickaxe or better to collect diamonds.";
        case BLOCK_ORE_LAPIS: return "Can be mined with a stone pickaxe or better to collect lapis lazuli.";
        case BLOCK_ORE_REDSTONE: return "Can be mined with an iron pickaxe or better to collect redstone dust.";
        case BLOCK_GOLD_BLOCK: case BLOCK_IRON_BLOCK: case BLOCK_DIAMOND_BLOCK: case BLOCK_LAPIS_BLOCK:
            return "Used as an expensive building block or compact storage of the ore.";
        case BLOCK_OBSIDIAN: return "Can only be mined with a diamond pickaxe. Is produced by the meeting of water and still lava, and is used to build a portal.";
        case BLOCK_GLOWING_OBSIDIAN: return "Obsidion activated by external source.";
        case BLOCK_TALLGRASS: return "Sometimes produces seeds when broken.";
        case BLOCK_SNOW_BLOCK: return "A compact way to store snowballs.";
        case BLOCK_GLASS: return "Created in a furnace by smelting sand. Will break if you try to mine it.";
        case BLOCK_GLASS_PANE: return "Will break if you try to mine it.";
        case BLOCK_FENCE: case BLOCK_FENCE_GATE: return "Used as a barrier that cannot be jumped over.";
        case BLOCK_DOOR_WOOD: return "Wooden doors are activated by using, hitting them or with Redstone.";
        case BLOCK_DOOR_IRON: return "Iron doors can only be opened by Redstone, buttons or switches.";
        case BLOCK_TRAPDOOR: return "Work like normal doors, but are a one by one block and lay flat on the ground.";
        case BLOCK_GLOWSTONE: return "Used to create brighter light than torches. Melts snow/ice and can be used underwater.";
        case BLOCK_NETHER_REACTOR: return "Core of the Nether Reactor";
        case BLOCK_WOOL: return "Collected from sheep, and can be colored with dyes.";
        case BLOCK_BOOKSHELF: return "Used as decoration.";
        case BLOCK_CRAFTING_TABLE: return "Allows you to craft a more varied selection of items than the normal crafting.";
        case BLOCK_STONECUTTER: return "For crafting stoneblocks.";
        case BLOCK_CHEST: return "Stores blocks and items inside.";
        case BLOCK_FURNACE: return "Allows you to smelt ore, create charcoal and glass, and cook fish and porkchops.";
        case BLOCK_TNT: return "Used to cause explosions. Activated after placing by hitting, or with an electrical charge.";
        case BLOCK_CACTUS: return "Can be crafted to create a dye.";
        case BLOCK_MELON: return "Can be broken into melon slices.";
        case BLOCK_LEAVES: return "When broken sometimes drops a sapling which can then be replanted to grow into a tree.";
        case BLOCK_FLOWER: case BLOCK_ROSE: return "Can be crafted into a dye.";
        case BLOCK_MUSHROOM_BROWN: case BLOCK_MUSHROOM_RED: return "Can be crafted with a bowl to make stew.";
        case BLOCK_SAPLING: return "Can be planted and it will eventually grow into a tree.";
        case BLOCK_REEDS: return "Can be crafted to create sugar.";
        default: return getBlockName(id, data);
    }
}

void drawStackCount(Font& font, int count, float slotX, float slotY, float size) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", count);
    float scale = HUD_S;
    int tw = fontTextWidth(&font, buf);
    fontDrawTextShadow(&font, slotX + size - tw * scale, slotY + size - 8.0f * scale,
                       buf, 0xFFFFFFFFu, scale);
}

void hotbarDraw(MenuState& s) {
    if (!s.haveGui) return;
    const float barW = 20.0f * HUD_N * HB_S;
    const float barX = (480.0f - barW) * 0.5f;
    const float barY = HUD_HOTBAR_TOP;

    textureBind(&s.guiAtlas);
    spriteDraw(&s.guiAtlas, barX, barY, 20.0f * HUD_N * HB_S, 22.0f * HB_S,
               GA_HOTBAR_X, GA_HOTBAR_Y, 20.0f * HUD_N, 22.0f, HUD_WHITE);
    spriteDraw(&s.guiAtlas, barX + 20.0f * HUD_N * HB_S, barY, 2.0f * HB_S, 22.0f * HB_S,
               GA_HOTBAR_CAP, HUD_WHITE);

    extern float g_dropCharge;
    unsigned int selTint = (g_dropCharge >= 0.0f) ? 0xFF40FF40u : HUD_WHITE;
    spriteDraw(&s.guiAtlas, barX + (20.0f * g_level.player->inventory->selected - 1.0f) * HB_S, barY - 1.0f * HB_S,
               24.0f * HB_S, 24.0f * HB_S, GA_SEL_FRAME, selTint);

    if (g_haveGuiBlocks) {
        const float dotsW = 14.0f * HB_S, dotsH = 4.0f * HB_S;
        const float slotX = barX + (3.0f + 20.0f * HOTBAR_SLOTS) * HB_S;
        textureBind(&g_guiBlocks);
        spriteDraw(&g_guiBlocks, slotX + (16.0f * HB_S - dotsW) * 0.5f,
                   barY + 3.0f * HB_S + (16.0f * HB_S - dotsH) * 0.5f, dotsW, dotsH,
                   484, 504, 28, 8, HUD_WHITE);
    }

    extern float g_dropCharge;
    if (g_dropCharge >= 0.0f) {
        float frac = g_dropCharge; if (frac > 1.0f) frac = 1.0f;
        float slotX = barX + (3.0f + 20.0f * g_level.player->inventory->selected) * HB_S;
        float slotY = barY + 3.0f * HB_S;
        float full = 16.0f * HB_S;
        float h = full * frac;
        sceGuEnable(GU_BLEND);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        guiFill(slotX, slotY + full - h, 16.0f * HB_S, h, 0x8000FF00u);
    }

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        float slotX = barX + (3.0f + 20.0f * i) * HB_S;
        float slotY = barY + 3.0f * HB_S;

        if (i == 0 && g_flashSlotStartTime >= 0.0f) {
            float now = nowSeconds();
            float since = now - g_flashSlotStartTime;
            if (since > 0.2f) {
                g_flashSlotStartTime = -1.0f;
            } else {

                int alpha = (int)(0xB0 * (1.0f - since / 0.2f));
                if (alpha < 0) alpha = 0;
                if (alpha > 255) alpha = 255;
                unsigned int color = 0x00FFFFFF | (alpha << 24);
                guiFill(slotX, slotY, 16.0f * HB_S, 16.0f * HB_S, color);
            }
        }

        ItemInstance* it = g_level.player->inventory->getLinked(i);
        if (it) {
            drawBlockIcon(it->id, it->data, slotX, slotY, 16.0f * HB_S);
            if (!g_level.player->inventory->isCreative() && it->count > 1)
                drawStackCount(s.font, it->count, slotX, slotY, 16.0f * HB_S);

            Item* itm = (it->id > 0 && it->id < 4096) ? Item::items[it->id] : nullptr;
            if (itm && itm->maxDamage > 0 && it->data > 0) {
                float frac = 1.0f - (float)it->data / (float)itm->maxDamage;
                if (frac < 0.0f) frac = 0.0f;
                float bx = slotX + 1.0f * HB_S, by = slotY + 14.0f * HB_S;
                float bw = 14.0f * HB_S, bh = 1.0f * HB_S;
                unsigned int r = (unsigned int)((1.0f - frac) * 255.0f);
                unsigned int g = (unsigned int)(frac * 255.0f);
                guiFill(bx, by, bw, bh, 0xFF000000u);
                guiFill(bx, by, bw * frac, bh, 0xFF000000u | (g << 8) | r);
            }
        }
    }

    bool overlayUp = g_invOpen || g_chestOpen || g_furnaceOpen ||
                     g_craftOpen || g_armorOpen || g_paused;
    extern int g_cloudTicks;
    if (!overlayUp && !g_level.player->inventory->isCreative() && s.haveGui && g_level.player) {
        int hp = g_level.player->health; if (hp < 0) hp = 0;
        int hearts = g_level.player->getMaxHealth() / 2;

        int invT = g_level.player->invulnerableTime;
        bool flash = (invT >= 10) && ((invT / 3) % 2 == 1);
        float containerU = flash ? 25.0f : 16.0f;

        int oldHp = g_level.player->lastHealth;
        const float hs = 9.0f * HUD_ST_S, step = 8.0f * HUD_ST_S;

        const float armorW = 9.0f * step + hs;
        int armorVal = g_level.player->getArmorValue();
        float hx0, hy, armorX, armorY, airX, airY;
        if (!g_barOnTop) {
            hx0    = 2.0f * HUD_ST_S;                          hy     = 2.0f * HUD_ST_S;
            armorX = 480.0f - 2.0f * HUD_ST_S - armorW;        armorY = hy;
            airX   = armorX;                                   airY   = hy + 10.0f * HUD_ST_S;

            if (armorVal <= 0) airY = armorY;
        } else {
            const float barW = 20.0f * HUD_N * HB_S;
            const float barX = (480.0f - barW) * 0.5f;
            const float gap  = 2.0f;
            float row1 = HUD_HOTBAR_TOP - hs - gap;
            float row2 = row1 - hs - gap;

            const float POKE = 8.0f;
            hx0    = barX - POKE;                              hy     = row1;
            armorX = barX - POKE;                              armorY = row2;

            airX   = barX + barW - armorW + POKE;
            airY   = (armorVal > 0) ? row2 : row1;
        }
        textureBind(&s.guiAtlas);
        for (int i = 0; i < hearts; i++) {
            float hx = hx0 + i * step;

            float jit = 0.0f;
            if (hp <= 4) jit = (float)((((i * 3 + g_cloudTicks) * 1103515245) >> 16 & 1) - 1) * HUD_ST_S;
            float hyj = hy + jit;
            int ip2 = i + i + 1;
            spriteDraw(&s.guiAtlas, hx, hyj, hs, hs, GA_ICONS_X + containerU, 0 + GA_ICONS_Y, 9, 9, HUD_WHITE);
            if (flash) {
                if (ip2 < oldHp)       spriteDraw(&s.guiAtlas, hx, hyj, hs, hs, GA_ICONS_X + 70, 0 + GA_ICONS_Y, 9, 9, HUD_WHITE);
                else if (ip2 == oldHp) spriteDraw(&s.guiAtlas, hx, hyj, hs, hs, GA_ICONS_X + 79, 0 + GA_ICONS_Y, 9, 9, HUD_WHITE);
            }
            if (hp >= (i + 1) * 2)   spriteDraw(&s.guiAtlas, hx, hyj, hs, hs, GA_ICONS_X + 52, 0 + GA_ICONS_Y, 9, 9, HUD_WHITE);
            else if (hp == i * 2 + 1) spriteDraw(&s.guiAtlas, hx, hyj, hs, hs, GA_ICONS_X + 61, 0 + GA_ICONS_Y, 9, 9, HUD_WHITE);
        }

        {
            if (armorVal > 0) {
                for (int i = 0; i < 10; i++) {
                    int ip2 = i * 2 + 1;
                    float ax = armorX + i * step;
                    float u = (ip2 < armorVal) ? 34.0f : (ip2 == armorVal) ? 52.0f : 16.0f;
                    spriteDraw(&s.guiAtlas, ax, armorY, hs, hs, GA_ICONS_X + u, 9 + GA_ICONS_Y, 9, 9, HUD_WHITE);
                }
            }
        }

        if (g_level.player->airSupply < 300) {
            int air = g_level.player->airSupply; if (air < 0) air = 0;
            int full    = (int)ceilf((air - 2) * 10.0f / 300.0f); if (full < 0) full = 0;
            int partial = (int)ceilf(air * 10.0f / 300.0f) - full;
            for (int i = 0; i < 10; i++) {
                float ax = airX + i * step;
                if (i < full)                        spriteDraw(&s.guiAtlas, ax, airY, hs, hs, GA_ICONS_X + 16, 18 + GA_ICONS_Y, 9, 9, HUD_WHITE);
                else if (i == full && partial > 0)   spriteDraw(&s.guiAtlas, ax, airY, hs, hs, GA_ICONS_X + 25, 18 + GA_ICONS_Y, 9, 9, HUD_WHITE);
            }
        }
    }

    if (!overlayUp && s.haveFont) {
        float now = nowSeconds();
        float ly = HUD_HOTBAR_TOP - 14.0f;
        for (int i = CHAT_LINES - 1; i >= 0; i--) {
            if (s_chatTime[i] <= 0.0f) continue;
            float age = now - s_chatTime[i];
            if (age > CHAT_SHOW_S) { s_chatTime[i] = 0.0f; continue; }
            int alpha = 255;
            if (age > CHAT_SHOW_S - 1.0f) alpha = (int)(255.0f * (CHAT_SHOW_S - age));
            if (alpha < 0) alpha = 0;

            float tw = fontTextWidth(&s.font, s_chat[i]) * HUD_S;

            guiFill(2.0f, ly - 1.0f * HUD_S, tw + 4.0f * HUD_S, 10.0f * HUD_S,
                    (unsigned int)((alpha / 2) << 24));
            fontDrawTextShadow(&s.font, 4.0f, ly, s_chat[i],
                               0x00FFFFFFu | ((unsigned int)alpha << 24), HUD_S);
            ly -= 10.0f * HUD_S;
        }
    }

    if (!g_invOpen && s.haveGui) {
        sceGuBlendFunc(GU_ADD, GU_ONE_MINUS_OTHER_COLOR, GU_ONE_MINUS_OTHER_COLOR, 0, 0);
        textureBind(&s.guiAtlas);
        float cx = 240.0f - 8.0f * HUD_S;
        float cy = 136.0f - 8.0f * HUD_S;
        spriteDraw(&s.guiAtlas, cx, cy, 16.0f * HUD_S, 16.0f * HUD_S, GA_ICONS_X + 0, 0 + GA_ICONS_Y, 16, 16, HUD_WHITE);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    }

    static int lastSelSlot = -1;
    static short lastId = 255;
    static unsigned char lastData = 255;
    static float nameDisplayStartTime = -1.0f;

    ItemInstance* selIt = g_level.player->inventory->getSelected();
    short curId = selIt ? selIt->id : 0;
    unsigned char curData = selIt ? (unsigned char)selIt->data : 0;

    if (curId > 0 && curId < 4096 && Item::items[curId] && Item::items[curId]->maxDamage > 0)
        curData = 0;

    if (g_level.player->inventory->selected != lastSelSlot || curId != lastId || curData != lastData) {
        lastSelSlot = g_level.player->inventory->selected;
        lastId = curId;
        lastData = curData;

        nameDisplayStartTime = (curId > 0) ? nowSeconds() : -1.0f;
    }

    if (nameDisplayStartTime >= 0.0f) {
        float now = nowSeconds();
        float since = now - nameDisplayStartTime;
        if (since > 2.0f) {
            nameDisplayStartTime = -1.0f;
        } else {
            const char* name = getBlockName(curId, curData);
            float scale = HUD_S;
            int tw = fontTextWidth(&s.font, name);
            float textX = 240.0f - (tw * scale) * 0.5f;
            float textY = barY - 18.0f * scale;
            int alpha = 255;
            if (since > 1.5f) {
                alpha = (int)(255.0f * (2.0f - since) / 0.5f);
                if (alpha < 0) alpha = 0;
                if (alpha > 255) alpha = 255;
            }
            unsigned int color = 0x00FFFFFF | (alpha << 24);
            fontDrawTextShadow(&s.font, textX, textY, name, color, scale);
        }
    }
}

void guiFill(float x, float y, float w, float h, unsigned int color) {
    struct CV { unsigned int color; float x, y, z; };
    CV* v = (CV*)sceGuGetMemory(2 * sizeof(CV));
    v[0].color = color; v[0].x = x;     v[0].y = y;     v[0].z = 0.0f;
    v[1].color = color; v[1].x = x + w; v[1].y = y + h; v[1].z = 0.0f;
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, v);
    sceGuEnable(GU_TEXTURE_2D);
}

void guiFillGradient(float x, float y, float w, float h,
                            unsigned int topColor, unsigned int botColor) {
    struct CV { unsigned int color; float x, y, z; };
    CV* v = (CV*)sceGuGetMemory(4 * sizeof(CV));
    v[0].color = topColor; v[0].x = x;     v[0].y = y;     v[0].z = 0.0f;
    v[1].color = topColor; v[1].x = x + w; v[1].y = y;     v[1].z = 0.0f;
    v[2].color = botColor; v[2].x = x;     v[2].y = y + h; v[2].z = 0.0f;
    v[3].color = botColor; v[3].x = x + w; v[3].y = y + h; v[3].z = 0.0f;
    sceGuDisable(GU_TEXTURE_2D);

    sceGuShadeModel(GU_SMOOTH);
    sceGuDrawArray(GU_TRIANGLE_STRIP, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, v);
    sceGuShadeModel(GU_FLAT);
    sceGuEnable(GU_TEXTURE_2D);
}

#include "client/gamemode/gamemode.h"
#include "world/level/tile/entity/furnace_tile_entity.h"
#include "gpu/gui_atlas.h"

void gameHintsDraw(MenuState& s) {

    extern bool g_photoPending;
    if (g_photoPending) return;
    extern bool g_paused, g_deadScreen;
    if (g_paused || g_deadScreen || g_signEditing) return;

    ButtonHint h[4];
    int n = 0;

    float hintsY = HUD_HINTS_Y;

    if (g_level.player && g_level.player->isSleeping()) {
        h[n++] = (ButtonHint){ BTN_ICON_L, PSP_CTRL_LTRIGGER, "Wake Up" };
        buttonHintsDraw(s, h, n, hintsY, HUD_HINT_S);
        return;
    }

    if (g_furnaceOpen) {
        if (furnaceFocusIsSlots()) {
            h[n++] = (ButtonHint){ BTN_ICON_CROSS,    PSP_CTRL_CROSS,    "Take" };
            h[n++] = (ButtonHint){ BTN_ICON_TRIANGLE, PSP_CTRL_TRIANGLE, "Quick Move" };
            h[n++] = (ButtonHint){ BTN_ICON_SQUARE,   PSP_CTRL_SQUARE,   "Take Half" };
        } else {

            int slot = furnaceTargetSlotForCursor();
            bool fuel = (slot == FurnaceTileEntity::SLOT_FUEL);
            if (slot >= 0) {
                h[n++] = (ButtonHint){ BTN_ICON_CROSS, PSP_CTRL_CROSS,
                                       fuel ? "Add Fuel" : "Add Ingredient" };
                h[n++] = (ButtonHint){ BTN_ICON_TRIANGLE, PSP_CTRL_TRIANGLE,
                                       fuel ? "Move Fuel" : "Move Ingredient" };
                h[n++] = (ButtonHint){ BTN_ICON_SQUARE, PSP_CTRL_SQUARE, "Move Half" };
            }
        }
        h[n++] = (ButtonHint){ BTN_ICON_CIRCLE, PSP_CTRL_CIRCLE, "Exit" };
    } else if (g_chestOpen) {

        bool take = chestCursorOnChest();
        if (!g_level.player->inventory->isCreative())
            h[n++] = (ButtonHint){ BTN_ICON_CROSS, PSP_CTRL_CROSS, take ? "Take" : "Move" };
        h[n++] = (ButtonHint){ BTN_ICON_TRIANGLE, PSP_CTRL_TRIANGLE, "Quick Move" };
        h[n++] = (ButtonHint){ BTN_ICON_SQUARE,   PSP_CTRL_SQUARE,
                               take ? "Take Half" : "Move Half" };
        h[n++] = (ButtonHint){ BTN_ICON_CIRCLE,   PSP_CTRL_CIRCLE, "Exit" };
    } else if (g_armorOpen) {
        h[n++] = (ButtonHint){ BTN_ICON_CROSS, PSP_CTRL_CROSS,
                               armorFocusIsWornSlot() ? "Take Off" : "Put On" };
        h[n++] = (ButtonHint){ BTN_ICON_CIRCLE, PSP_CTRL_CIRCLE, "Exit" };
    } else if (g_craftOpen) {
        h[n++] = (ButtonHint){ BTN_ICON_CROSS,  PSP_CTRL_CROSS,  "Create" };
        h[n++] = (ButtonHint){ BTN_ICON_CIRCLE, PSP_CTRL_CIRCLE, "Exit" };
        if (craftHasCategories()) {
            h[n++] = (ButtonHint){ BTN_ICON_L, PSP_CTRL_LTRIGGER, "" };
            h[n++] = (ButtonHint){ BTN_ICON_R, PSP_CTRL_RTRIGGER, "Change Group" };
        }
    } else if (g_invOpen) {
        h[n++] = (ButtonHint){ BTN_ICON_CROSS,  PSP_CTRL_CROSS,  "Take" };
        h[n++] = (ButtonHint){ BTN_ICON_CIRCLE, PSP_CTRL_CIRCLE, "Exit" };

        if (!g_level.player->inventory->isCreative()) {
            h[n++] = (ButtonHint){ BTN_ICON_SQUARE,   PSP_CTRL_SQUARE,   "Crafting" };
            h[n++] = (ButtonHint){ BTN_ICON_TRIANGLE, PSP_CTRL_TRIANGLE, "Armour" };
        }
    } else {

        CrosshairTarget t = gameModeCrosshairTarget();
        if (g_level.player->inventory->selected == HOTBAR_SLOTS)
            h[n++] = (ButtonHint){ BTN_ICON_L, PSP_CTRL_LTRIGGER, "Inventory" };
        else if (t.useLabel)
            h[n++] = (ButtonHint){ BTN_ICON_L, PSP_CTRL_LTRIGGER, t.useLabel };
        if (t.breakLabel) h[n++] = (ButtonHint){ BTN_ICON_R, PSP_CTRL_RTRIGGER, t.breakLabel };
    }

    if (n) buttonHintsDraw(s, h, n, hintsY, HUD_HINT_S);
}
