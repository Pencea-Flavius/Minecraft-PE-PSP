
#include "world/level/tile/tile.h"
#include "world/level/tile/tiles.h"
#include "world/level/tile/tile_behavior.h"
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/entity/item_entity.h"
#include "world/item/item.h"
#include "world/level/levelgen/Random.h"
#include "world/entity/player.h"
#include "world/entity/entity.h"
#include "world/inventory/inventory.h"
#include "world/level/tile/tile_gui_hooks.h"
#include "world/level/tile/entity/tile_entity.h"
#include "world/level/tile/entity/chest_tile_entity.h"
#include "world/level/tile/entity/furnace_tile_entity.h"
#include "world/level/tile/entity/sign_tile_entity.h"
#include "world/level/tile/entity/reactor_tile_entity.h"
#include "world/level/tile/nether_reactor_pattern.h"
#include "world/level/tile/redstone_ore.h"
#include "world/level/tile/fire.h"
#include "client/gamemode/gamemode.h"
#include <stdlib.h>
#include <math.h>

static void playDoorSound(int x, int y, int z, bool opened) {
    g_level.playSound(x + 0.5f, y + 0.5f, z + 0.5f,
                      opened ? "random.door_open" : "random.door_close",
                      1.0f, (rand() / (float)RAND_MAX) * 0.1f + 0.9f);
}

static int facingQuadrant(Player* p) {
    if (!p) return 0;
    float yaw = p->yRot;
    while (yaw < 0.0f)    yaw += 360.0f;
    while (yaw >= 360.0f) yaw -= 360.0f;
    int q = ((int)floorf(yaw * 4.0f / 360.0f + 0.5f)) & 3;
    return (4 - q) & 3;
}

static int facingFromYaw(float yawDeg) {
    static const int kQuadrantFace[4] = { F_BACK, F_LEFT, F_FORWARD, F_RIGHT };
    int q = ((int)floorf(yawDeg / 90.0f + 0.5f)) & 3;
    return kQuadrantFace[q];
}

Tile* Tile::tiles[256] = { 0 };

const SoundType g_tileSounds[SOUND_TYPE_COUNT] = {
     { 0.0f,  0.0f, 0,              0            },
     { 1.0f,  1.0f, "step.stone",   "step.stone" },
     { 1.0f,  1.0f, "step.wood",    "step.wood"  },
     { 1.0f,  1.0f, "step.gravel",  "step.gravel"},
     { 0.5f,  1.0f, "step.grass",   "step.grass" },
     { 1.0f,  1.5f, "step.stone",   "step.stone" },
     { 1.0f,  1.0f, "random.glass", "step.stone" },
     { 1.0f,  1.0f, "step.cloth",   "step.cloth" },

     { 1.0f,  1.0f, "step.sand",    "step.sand"  },
};

int Tile::getAABB(const World* , int x, int y, int z, BlockAABB out[3]) {
    if (!solidPhys) return 0;
    out[0].x0 = (float)x; out[0].x1 = x + 1.0f;
    out[0].y0 = (float)y; out[0].y1 = y + 1.0f;
    out[0].z0 = (float)z; out[0].z1 = z + 1.0f;
    return 1;
}

bool Tile::mayPlace(World* w, int x, int y, int z, int ) { return mayPlace(w, x, y, z); }
bool Tile::mayPlace(World* w, int x, int y, int z) {

    return isReplaceable(worldBlock(w, x, y, z));
}
bool Tile::canSurvive(World*, int, int, int) { return true; }
void Tile::entityInside(World*, int, int, int, Entity*) {}
void Tile::neighborChanged(World* w, int x, int y, int z) {
    if (canSurvive(w, x, y, z)) return;
    worldSpawnResources(w, x, y, z, id, worldData(w, x, y, z));
    worldSetBlockAndData(w, x, y, z, BLOCK_AIR, 0);
    worldNotifyNeighborsChanged(w, x, y, z);
}
void Tile::randomTick(World*, int, int, int) {}

Drop Tile::getResource(int data) {
    switch (id) {
        case BLOCK_STONE:               return { BLOCK_COBBLESTONE, 1, 0 };
        case BLOCK_GRASS:               return { BLOCK_DIRT, 1, 0 };
        case BLOCK_FARMLAND:            return { BLOCK_DIRT, 1, 0 };
        case BLOCK_LEAVES:              return { BLOCK_SAPLING, 1, (short)(data & 3) };
        case BLOCK_SAPLING:             return { BLOCK_SAPLING, 1, (short)(data & 3) };

        case BLOCK_DOUBLE_SLAB:         return { BLOCK_SLAB, 2, (short)(data & DSLAB_MAT_MASK) };
        case BLOCK_SLAB:                return { BLOCK_SLAB, 1, (short)(data & DSLAB_MAT_MASK) };
        case BLOCK_LOG:                 return { BLOCK_LOG, 1, (short)(data & LOG_TYPE_MASK) };
        case BLOCK_SANDSTONE:           return { BLOCK_SANDSTONE, 1, (short)data };
        case BLOCK_QUARTZ_BLOCK:        return { BLOCK_QUARTZ_BLOCK, 1, (short)data };
        case BLOCK_STONE_BRICKS:        return { BLOCK_STONE_BRICKS, 1, (short)data };

        case BLOCK_FURNACE_LIT:         return { BLOCK_FURNACE, 1, 0 };
        case BLOCK_GLOWING_OBSIDIAN:    return { BLOCK_OBSIDIAN, 1, 0 };

        case BLOCK_ORE_COAL:            return { ITEM_COAL, 1, 0 };
        case BLOCK_ORE_EMERALD:         return { ITEM_DIAMOND, 1, 0 };

        case BLOCK_GLOWSTONE:           return { ITEM_GLOWSTONE_DUST, 1, 0 };
        case BLOCK_CLAY:                return { ITEM_CLAY, 4, 0 };
        case BLOCK_BOOKSHELF:           return { ITEM_BOOK, 3, 0 };

        case BLOCK_TALLGRASS:           return { ITEM_SEEDS_WHEAT, 1, 0 };
        case BLOCK_SIGN: case BLOCK_WALL_SIGN: return { ITEM_SIGN, 1, 0 };
        case BLOCK_REEDS:               return { ITEM_REEDS, 1, 0 };
        case BLOCK_WOOL:                return { BLOCK_WOOL, 1, (short)data };

        case BLOCK_WHEAT:               return (data == 7) ? Drop{ ITEM_WHEAT, 1, 0 } : Drop{ 0, 0, 0 };

        case BLOCK_MELON:               return { ITEM_MELON, 1, 0 };

        case BLOCK_TOPSNOW:             return { ITEM_SNOWBALL, 1, 0 };
        case BLOCK_SNOW_BLOCK:          return { ITEM_SNOWBALL, 4, 0 };
        case BLOCK_GLASS: case BLOCK_GLASS_PANE:
        case BLOCK_ICE:
        case BLOCK_ORE_REDSTONE: case BLOCK_ORE_REDSTONE_LIT:
        case BLOCK_MELON_STEM:
        case BLOCK_BEDROCK:
        case BLOCK_WATER: case BLOCK_CALM_WATER: case BLOCK_LAVA: case BLOCK_CALM_LAVA:
            return { 0, 0, 0 };

        case BLOCK_DOOR_WOOD:
            return (data & 8) ? Drop{ 0, 0, 0 } : Drop{ ITEM_DOOR_WOOD_ITEM, 1, 0 };
        case BLOCK_DOOR_IRON:
            return (data & 8) ? Drop{ 0, 0, 0 } : Drop{ ITEM_DOOR_IRON_ITEM, 1, 0 };
        case BLOCK_BED:
            return (data & 8) ? Drop{ 0, 0, 0 } : Drop{ ITEM_BED_ITEM, 1, 0 };
        default:
            return { (short)id, 1, 0 };
    }
}

int Tile::getResourceCount(int data, Random& rng) {
    if (id == BLOCK_LEAVES) return (rng.nextInt(20) == 0) ? 1 : 0;
    if (id == BLOCK_TALLGRASS) return (rng.nextInt(8) == 0) ? 1 : 0;
    if (id == BLOCK_CLAY) return 4;
    if (id == BLOCK_BOOKSHELF) return 3;
    if (id == BLOCK_MELON) return 3 + rng.nextInt(5);
    if (id == BLOCK_GLOWSTONE) return 2 + rng.nextInt(3);
    if (id == BLOCK_TOPSNOW) return 0;
    return getResource(data).count;
}

static void dropItem(int x, int y, int z, short id, short aux, Random& rng) {
    const float s = 0.7f;
    float xo = rng.nextFloat() * s + (1 - s) * 0.5f;
    float yo = rng.nextFloat() * s + (1 - s) * 0.5f;
    float zo = rng.nextFloat() * s + (1 - s) * 0.5f;
    ItemEntity* e = new ItemEntity(&g_level, x + xo, y + yo, z + zo, ItemInstance(id, 1, aux));
    e->throwTime = 10;
    g_level.addEntity(e);
}

void Tile::spawnResources(World* , int x, int y, int z, int data, Random& rng) {

    if (id == BLOCK_GRAVEL) {
        dropItem(x, y, z, (rng.nextInt(10) == 0) ? ITEM_FLINT : BLOCK_GRAVEL, 0, rng);
        return;
    }
    if (id == BLOCK_ORE_LAPIS) {
        int n = 4 + rng.nextInt(5);
        for (int i = 0; i < n; i++) dropItem(x, y, z, ITEM_BONEMEAL, 4, rng);
        return;
    }

    Drop d = getResource(data);
    int count = (d.count <= 0) ? 0 : getResourceCount(data, rng);
    if (d.id > 0 && count > 0)
        for (int i = 0; i < count; i++)
            dropItem(x, y, z, d.id, d.aux, rng);

    if (id == BLOCK_LEAVES && (data & 3) == 0 && rng.nextInt(200) == 0)
        dropItem(x, y, z, ITEM_APPLE, 0, rng);
}

void Tile::getTexture(unsigned char data, int f, int* col, int* row, unsigned int* tint) {
    *tint = 0xFFFFFFFFu;
    switch (id) {
        case BLOCK_GRASS:
            if (f == F_TOP)       { *col = 0; *row = 0; *tint = 0xFF6BCB5Au; }
            else if (f == F_DOWN) { *col = 2; *row = 0; }
            else                  { *col = 3; *row = 0; }
            break;
        case BLOCK_DIRT:  *col = 2; *row = 0; break;
        case BLOCK_SLAB:
        case BLOCK_DOUBLE_SLAB:
            switch (data & DSLAB_MAT_MASK) {
                case DSLAB_SAND:        Tile::tiles[BLOCK_SANDSTONE]->getTexture(0, f, col, row, tint); return;
                case DSLAB_WOOD:        Tile::tiles[BLOCK_PLANKS]->getTexture(0, f, col, row, tint); return;
                case DSLAB_COBBLE:      Tile::tiles[BLOCK_COBBLESTONE]->getTexture(0, f, col, row, tint); return;
                case DSLAB_BRICK:       Tile::tiles[BLOCK_BRICKS]->getTexture(0, f, col, row, tint); return;
                case DSLAB_SMOOTHBRICK: Tile::tiles[BLOCK_STONE_BRICKS]->getTexture(0, f, col, row, tint); return;
                default:
                    if (f == F_TOP || f == F_DOWN) { *col = 6; *row = 0; }
                    else                           { *col = 5; *row = 0; }
                    return;
            }
            break;
        case BLOCK_STONE: *col = 1; *row = 0; break;
        case BLOCK_WATER:
        case BLOCK_CALM_WATER:
            if (f == F_TOP || f == F_DOWN) { *col = 13; *row = 12; }
            else                           { *col = 14; *row = 12; }
            break;
        case BLOCK_ICE:       *col = 3; *row = 4;  break;
        case BLOCK_SAND:      *col = 2; *row = 1;  break;
        case BLOCK_GRAVEL:    *col = 3; *row = 1;  break;

        case BLOCK_STAIRS_SANDSTONE:
            if (f == F_TOP)       { *col = 0; *row = 11; }
            else if (f == F_DOWN) { *col = 0; *row = 13; }
            else                  { *col = 0; *row = 12; }
            break;
        case BLOCK_SANDSTONE:
            switch (data) {
                case SS_CHISELED: tileChiseledSandstone(data, f, col, row); break;
                case SS_SMOOTH:   tileSmoothSandstone(data, f, col, row); break;
                default:
                    if (f == F_TOP)       { *col = 0; *row = 11; }
                    else if (f == F_DOWN) { *col = 0; *row = 13; }
                    else                  { *col = 0; *row = 12; }
                    break;
            }
            break;
        case BLOCK_BEDROCK:   *col = 1; *row = 1;  break;
        case BLOCK_BED: {
            bool isHead = (data & 0x8) != 0;
            int dir = data & 3;
            if (f == F_DOWN) {
                *col = 4; *row = 0;
            } else if (f == F_TOP) {
                if (isHead) { *col = 7; *row = 8; } else { *col = 6; *row = 8; }
            } else {
                bool isOuterFace = false, isInnerFace = false;
                if (dir == 0) {
                    if (f == F_FORWARD) { if (isHead) isOuterFace = true; else isInnerFace = true; }
                    else if (f == F_BACK) { if (isHead) isInnerFace = true; else isOuterFace = true; }
                } else if (dir == 1) {
                    if (f == F_LEFT) { if (isHead) isOuterFace = true; else isInnerFace = true; }
                    else if (f == F_RIGHT) { if (isHead) isInnerFace = true; else isOuterFace = true; }
                } else if (dir == 2) {
                    if (f == F_BACK) { if (isHead) isOuterFace = true; else isInnerFace = true; }
                    else if (f == F_FORWARD) { if (isHead) isInnerFace = true; else isOuterFace = true; }
                } else {
                    if (f == F_RIGHT) { if (isHead) isOuterFace = true; else isInnerFace = true; }
                    else if (f == F_LEFT) { if (isHead) isInnerFace = true; else isOuterFace = true; }
                }
                if (isOuterFace) {
                    if (isHead) { *col = 8; *row = 9; } else { *col = 5; *row = 9; }
                } else if (isInnerFace) {
                    *col = 5; *row = 9;
                } else {
                    if (isHead) { *col = 7; *row = 9; } else { *col = 6; *row = 9; }
                }
            }
            break;
        }
        case BLOCK_CLAY:      *col = 8; *row = 4;  break;
        case BLOCK_LOG:

            if (f == F_TOP || f == F_DOWN) { *col = 5; *row = 1; }
            else switch (data & LOG_TYPE_MASK) {
                case LOG_SPRUCE: *col = 4; *row = 7; break;
                case LOG_BIRCH:  *col = 5; *row = 7; break;
                default:         *col = 4; *row = 1; break;
            }
            break;
        case BLOCK_LEAVES:
            switch (data & LEAF_TYPE_MASK) {
                case LEAF_SPRUCE: *col = 4; *row = 8; *tint = 0xFF2BAE3Du; break;
                case LEAF_BIRCH:  *col = 4; *row = 3; *tint = 0xFF55A780u; break;
                default:          *col = 4; *row = 3; *tint = 0xFF18B548u; break;
            }
            break;
        case BLOCK_COBWEB:         *col = 11; *row = 0; break;

        case BLOCK_TALLGRASS:
            switch (data) {
                case TG_FERN:       *col = 8; *row = 3; *tint = 0xFF339933u; break;
                case TG_TALL_GRASS: *col = 7; *row = 2; *tint = 0xFF339933u; break;
                default:            *col = 7; *row = 3; break;
            }
            break;
        case BLOCK_FIRE:           *col = 15; *row = 1; break;
        case BLOCK_FLOWER:         *col = 13; *row = 0; break;
        case BLOCK_ROSE:           *col = 12; *row = 0; break;
        case BLOCK_MUSHROOM_BROWN: *col = 13; *row = 1; break;
        case BLOCK_MUSHROOM_RED:   *col = 12; *row = 1; break;
        case BLOCK_REEDS:          *col = 9;  *row = 4; break;
        case BLOCK_ORE_COAL:       *col = 2;  *row = 2; break;
        case BLOCK_ORE_IRON:       *col = 1;  *row = 2; break;
        case BLOCK_ORE_GOLD:       *col = 0;  *row = 2; break;
        case BLOCK_ORE_REDSTONE:
        case BLOCK_ORE_REDSTONE_LIT: *col = 3;  *row = 3; break;
        case BLOCK_ORE_EMERALD:    *col = 2;  *row = 3; break;
        case BLOCK_ORE_LAPIS:      *col = 0;  *row = 10; break;
        case BLOCK_LAVA:
        case BLOCK_CALM_LAVA:
            if (f == F_TOP || f == F_DOWN) { *col = 13; *row = 14; }
            else                           { *col = 14; *row = 14; }
            break;
        case BLOCK_CACTUS:
            if (f == F_TOP)       { *col = 5; *row = 4; }
            else if (f == F_DOWN) { *col = 7; *row = 4; }
            else                  { *col = 6; *row = 4; }
            break;
        case BLOCK_SAPLING:
            *col = 15;
            if (data == 1)      *row = 3;
            else if (data == 2) *row = 4;
            else                *row = 0;
            break;
        case BLOCK_TOPSNOW: *col = 2; *row = 4; break;
        case BLOCK_STAIRS_COBBLESTONE:
        case BLOCK_COBBLESTONE:  *col = 0; *row = 1;  break;
        case BLOCK_FENCE_GATE:
        case BLOCK_STAIRS_PLANKS:
        case BLOCK_FENCE:
        case BLOCK_SIGN: case BLOCK_WALL_SIGN:
        case BLOCK_PLANKS:       *col = 4; *row = 0;  break;
        case BLOCK_TRAPDOOR:     *col = 4; *row = 5;  break;
        case BLOCK_LADDER:       *col = 3; *row = 5;  break;
        case BLOCK_TORCH:        *col = 0; *row = 5;  break;
        case BLOCK_DOOR_WOOD:
        case BLOCK_DOOR_IRON:
            *row = ((data & 8) != 0 && f != F_TOP && f != F_DOWN) ? 5 : 6;
            *col = (id == BLOCK_DOOR_IRON) ? 2 : 1;
            break;
        case BLOCK_STAIRS_BRICK:
        case BLOCK_BRICKS:       *col = 7; *row = 0;  break;
        case BLOCK_STAIRS_STONE_BRICK: *col = 6; *row = 3;  break;
        case BLOCK_STONE_BRICKS:
            switch (data) {
                case SB_MOSSY:   *col = 4; *row = 6; break;
                case SB_CRACKED: *col = 5; *row = 6; break;
                default:         *col = 6; *row = 3; break;
            }
            break;
        case BLOCK_MOSSY_COBBLE: *col = 4; *row = 2;  break;
        case BLOCK_OBSIDIAN:     *col = 5; *row = 2;  break;
        case BLOCK_GLOWING_OBSIDIAN: *col = 10; *row = 13; break;
        case BLOCK_GOLD_BLOCK:   *col = 7; *row = 1;  break;
        case BLOCK_IRON_BLOCK:   *col = 6; *row = 1;  break;
        case BLOCK_DIAMOND_BLOCK:*col = 8; *row = 1;  break;
        case BLOCK_LAPIS_BLOCK:  *col = 0; *row = 9;  break;
        case BLOCK_SNOW_BLOCK:   *col = 2; *row = 4;  break;
        case BLOCK_NETHERRACK:   *col = 7; *row = 6;  break;
        case BLOCK_GLOWSTONE:    *col = 9; *row = 6;  break;
        case BLOCK_STAIRS_NETHER_BRICK:
        case BLOCK_NETHER_BRICK: *col = 0; *row = 14; break;
        case BLOCK_WOOL: tileWool(data, f, col, row); break;
        case BLOCK_TNT:
            if (f == F_TOP)       { *col = 9;  *row = 0; }
            else if (f == F_DOWN) { *col = 10; *row = 0; }
            else                  { *col = 8;  *row = 0; }
            break;
        case BLOCK_BOOKSHELF:
            if (f == F_TOP || f == F_DOWN) { *col = 4; *row = 0; }
            else                           { *col = 3; *row = 2; }
            break;
        case BLOCK_GLASS_PANE:
        case BLOCK_GLASS: tileGlass(data, f, col, row); break;
        case BLOCK_MELON:
            if (f == F_TOP || f == F_DOWN) { *col = 9; *row = 8; }
            else                           { *col = 8; *row = 8; }
            break;
        case BLOCK_STONECUTTER: tileStonecutter(data, f, col, row); break;
        case BLOCK_CRAFTING_TABLE: tileCraftingTable(data, f, col, row); break;
        case BLOCK_FURNACE: case BLOCK_FURNACE_LIT:
            tileFurnace(data, f, id == BLOCK_FURNACE_LIT, col, row); break;
        case BLOCK_CHEST: tileChest(data, f, col, row); break;
        case BLOCK_STAIRS_QUARTZ: tileQuartzBlock(data, f, col, row); break;
        case BLOCK_QUARTZ_BLOCK:
            switch (data) {
                case QZ_CHISELED: tileChiseledQuartz(data, f, col, row); break;
                case QZ_PILLAR:   tilePillarQuartz(data, f, col, row); break;
                default:          tileQuartzBlock(data, f, col, row); break;
            }
            break;
        case BLOCK_NETHER_REACTOR: tileNetherReactor(data, f, col, row); break;
        case BLOCK_FARMLAND:
            if (f == F_TOP) { *col = (data > 0) ? 6 : 7; *row = 5; }
            else { *col = 2; *row = 0; }
            break;
        case BLOCK_WHEAT:       *col = 8 + (data & 7); *row = 5; break;
        case BLOCK_MELON_STEM: {
            *col = 15; *row = 6;
            unsigned int r = data * 32u, g = 255u - data * 8u, b = data * 4u;
            *tint = 0xFF000000u | (b << 16) | (g << 8) | r;
            break;
        }
        case BLOCK_UPDATE1: *col = 12; *row = 15; break;

        case BLOCK_UPDATE2:
        default:            *col = 13; *row = 15; break;
    }
}

struct BedTile : Tile { BedTile(unsigned char i) : Tile(i) {}

    bool use(World* w, int x, int y, int z, Player* p) {
        if (!p) return true;
        int bx = x, by = y, bz = z;
        unsigned char bdata = worldData(w, bx, by, bz);
        if (!(bdata & 8)) {
            int dir = bdata & 3;
            bx += BED_HEAD_OFF[dir][0];
            bz += BED_HEAD_OFF[dir][1];
            if (worldBlock(w, bx, by, bz) != BLOCK_BED) return true;
            bdata = worldData(w, bx, by, bz);
        }

        if ((bdata & 4) && !p->isSleeping())
            worldSetData(w, bx, by, bz, (unsigned char)(bdata & ~4));
        int r = p->startSleepInBed(bx, by, bz);
        if (r == Player::BED_OK)
            worldSetData(w, bx, by, bz, (unsigned char)(worldData(w, bx, by, bz) | 4));
        else if (r == Player::BED_NOT_POSSIBLE_NOW)
            guiChatMessage("You can only sleep at night");
        else if (r == Player::BED_NOT_SAFE)
            guiChatMessage("You may not rest now, there are monsters nearby");
        return true;
    }
    int getAABB(const World*, int x, int y, int z, BlockAABB out[3]) {
        out[0] = { (float)x, (float)y, (float)z, x + 1.0f, y + 9.0f/16.0f, z + 1.0f }; return 1; }
    bool canSurvive(World* w, int x, int y, int z) { return supportCanSurvive(w, id, x, y, z, -1); }

};

struct FarmTile : Tile { FarmTile(unsigned char i) : Tile(i) { randomTicks = true; }

    int getAABB(const World*, int x, int y, int z, BlockAABB out[3]) {
        out[0] = { (float)x, (float)y, (float)z, x + 1.0f, y + 1.0f, z + 1.0f }; return 1; }
    void randomTick(World* w, int x, int y, int z) { tickFarmland(w, x, y, z); } };

struct SlabTile : Tile { SlabTile(unsigned char i) : Tile(i) {}
    int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
        unsigned char data = worldData(w, x, y, z);
        out[0].x0 = (float)x; out[0].x1 = x + 1.0f;
        out[0].z0 = (float)z; out[0].z1 = z + 1.0f;
        out[0].y0 = y + ((data & SLAB_TOP_SLOT_BIT) ? 0.5f : 0.0f);
        out[0].y1 = y + ((data & SLAB_TOP_SLOT_BIT) ? 1.0f : 0.5f);
        return 1; }

    int getPlacedOnFaceDataValue(World*, int, int, int, int face,
                                 float, float clickY, float, int itemValue) {
        bool upperHalf = (face == F_DOWN) || (face != F_TOP && clickY > 0.5f);
        return (itemValue & DSLAB_MAT_MASK) | (upperHalf ? SLAB_TOP_SLOT_BIT : 0);
    } };

static bool stairLockAttached(const World* w, int x, int y, int z, unsigned char myData) {
    unsigned char nb = worldBlock(w, x, y, z);
    if (isStairs(nb)) {
        unsigned char nbData = worldData(w, x, y, z);
        return (nbData & STAIR_UPSIDEDOWN_BIT) == (myData & STAIR_UPSIDEDOWN_BIT);
    }
    return false;
}

int stairShapeBoxes(const World* w, int gx, int y, int gz, unsigned char data, float out[3][6]) {
    int dir = data & STAIR_DIR_MASK;
    bool upsideDown = (data & STAIR_UPSIDEDOWN_BIT) != 0;
    int n = 0;

    float by0 = upsideDown ? 0.5f : 0.0f;
    float by1 = upsideDown ? 1.0f : 0.5f;
    out[n][0] = 0.0f; out[n][1] = by0; out[n][2] = 0.0f;
    out[n][3] = 1.0f; out[n][4] = by1; out[n][5] = 1.0f;
    n++;

    float sx0 = 0.0f, sx1 = 1.0f;
    float sy0 = upsideDown ? 0.0f : 0.5f;
    float sy1 = upsideDown ? 0.5f : 1.0f;
    float sz0 = 0.0f, sz1 = 1.0f;
    bool checkInnerPiece = true;

    if (dir == STAIR_DIR_EAST) {
        sx0 = 0.5f; sz1 = 1.0f;
        unsigned char backTile = worldBlock(w, gx + 1, y, gz);
        if (isStairs(backTile) && stairLockAttached(w, gx + 1, y, gz, data)) {
            int backDir = worldData(w, gx + 1, y, gz) & STAIR_DIR_MASK;
            if (backDir == STAIR_DIR_NORTH && !stairLockAttached(w, gx, y, gz + 1, data)) {
                sz1 = 0.5f; checkInnerPiece = false;
            } else if (backDir == STAIR_DIR_SOUTH && !stairLockAttached(w, gx, y, gz - 1, data)) {
                sz0 = 0.5f; checkInnerPiece = false;
            }
        }
    } else if (dir == STAIR_DIR_WEST) {
        sx1 = 0.5f; sz1 = 1.0f;
        unsigned char backTile = worldBlock(w, gx - 1, y, gz);
        if (isStairs(backTile) && stairLockAttached(w, gx - 1, y, gz, data)) {
            int backDir = worldData(w, gx - 1, y, gz) & STAIR_DIR_MASK;
            if (backDir == STAIR_DIR_NORTH && !stairLockAttached(w, gx, y, gz + 1, data)) {
                sz1 = 0.5f; checkInnerPiece = false;
            } else if (backDir == STAIR_DIR_SOUTH && !stairLockAttached(w, gx, y, gz - 1, data)) {
                sz0 = 0.5f; checkInnerPiece = false;
            }
        }
    } else if (dir == STAIR_DIR_SOUTH) {
        sz0 = 0.5f; sz1 = 1.0f;
        unsigned char backTile = worldBlock(w, gx, y, gz + 1);
        if (isStairs(backTile) && stairLockAttached(w, gx, y, gz + 1, data)) {
            int backDir = worldData(w, gx, y, gz + 1) & STAIR_DIR_MASK;
            if (backDir == STAIR_DIR_WEST && !stairLockAttached(w, gx + 1, y, gz, data)) {
                sx1 = 0.5f; checkInnerPiece = false;
            } else if (backDir == STAIR_DIR_EAST && !stairLockAttached(w, gx - 1, y, gz, data)) {
                sx0 = 0.5f; checkInnerPiece = false;
            }
        }
    } else if (dir == STAIR_DIR_NORTH) {
        sz1 = 0.5f;
        unsigned char backTile = worldBlock(w, gx, y, gz - 1);
        if (isStairs(backTile) && stairLockAttached(w, gx, y, gz - 1, data)) {
            int backDir = worldData(w, gx, y, gz - 1) & STAIR_DIR_MASK;
            if (backDir == STAIR_DIR_WEST && !stairLockAttached(w, gx + 1, y, gz, data)) {
                sx1 = 0.5f; checkInnerPiece = false;
            } else if (backDir == STAIR_DIR_EAST && !stairLockAttached(w, gx - 1, y, gz, data)) {
                sx0 = 0.5f; checkInnerPiece = false;
            }
        }
    }
    out[n][0] = sx0; out[n][1] = sy0; out[n][2] = sz0;
    out[n][3] = sx1; out[n][4] = sy1; out[n][5] = sz1;
    n++;

    if (checkInnerPiece) {
        float ix0 = 0.0f, ix1 = 0.5f;
        float iy0 = upsideDown ? 0.0f : 0.5f;
        float iy1 = upsideDown ? 0.5f : 1.0f;
        float iz0 = 0.5f, iz1 = 1.0f;
        bool hasInner = false;

        if (dir == STAIR_DIR_EAST) {
            unsigned char frontTile = worldBlock(w, gx - 1, y, gz);
            if (isStairs(frontTile) && stairLockAttached(w, gx - 1, y, gz, data)) {
                int frontDir = worldData(w, gx - 1, y, gz) & STAIR_DIR_MASK;
                if (frontDir == STAIR_DIR_NORTH && !stairLockAttached(w, gx, y, gz - 1, data)) {
                    iz0 = 0.0f; iz1 = 0.5f; hasInner = true;
                } else if (frontDir == STAIR_DIR_SOUTH && !stairLockAttached(w, gx, y, gz + 1, data)) {
                    iz0 = 0.5f; iz1 = 1.0f; hasInner = true;
                }
            }
        } else if (dir == STAIR_DIR_WEST) {
            unsigned char frontTile = worldBlock(w, gx + 1, y, gz);
            if (isStairs(frontTile) && stairLockAttached(w, gx + 1, y, gz, data)) {
                ix0 = 0.5f; ix1 = 1.0f;
                int frontDir = worldData(w, gx + 1, y, gz) & STAIR_DIR_MASK;
                if (frontDir == STAIR_DIR_NORTH && !stairLockAttached(w, gx, y, gz - 1, data)) {
                    iz0 = 0.0f; iz1 = 0.5f; hasInner = true;
                } else if (frontDir == STAIR_DIR_SOUTH && !stairLockAttached(w, gx, y, gz + 1, data)) {
                    iz0 = 0.5f; iz1 = 1.0f; hasInner = true;
                }
            }
        } else if (dir == STAIR_DIR_SOUTH) {
            unsigned char frontTile = worldBlock(w, gx, y, gz - 1);
            if (isStairs(frontTile) && stairLockAttached(w, gx, y, gz - 1, data)) {
                iz0 = 0.0f; iz1 = 0.5f;
                int frontDir = worldData(w, gx, y, gz - 1) & STAIR_DIR_MASK;
                if (frontDir == STAIR_DIR_WEST && !stairLockAttached(w, gx - 1, y, gz, data)) {
                    hasInner = true;
                } else if (frontDir == STAIR_DIR_EAST && !stairLockAttached(w, gx + 1, y, gz, data)) {
                    ix0 = 0.5f; ix1 = 1.0f; hasInner = true;
                }
            }
        } else if (dir == STAIR_DIR_NORTH) {
            unsigned char frontTile = worldBlock(w, gx, y, gz + 1);
            if (isStairs(frontTile) && stairLockAttached(w, gx, y, gz + 1, data)) {
                int frontDir = worldData(w, gx, y, gz + 1) & STAIR_DIR_MASK;
                if (frontDir == STAIR_DIR_WEST && !stairLockAttached(w, gx - 1, y, gz, data)) {
                    hasInner = true;
                } else if (frontDir == STAIR_DIR_EAST && !stairLockAttached(w, gx + 1, y, gz, data)) {
                    ix0 = 0.5f; ix1 = 1.0f; hasInner = true;
                }
            }
        }

        if (hasInner) {
            out[n][0] = ix0; out[n][1] = iy0; out[n][2] = iz0;
            out[n][3] = ix1; out[n][4] = iy1; out[n][5] = iz1;
            n++;
        }
    }
    return n;
}

struct StairTile : Tile { StairTile(unsigned char i) : Tile(i) {}
    int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
        float b[3][6];
        int n = stairShapeBoxes(w, x, y, z, worldData(w, x, y, z), b);
        for (int i = 0; i < n; i++) {
            out[i].x0 = x + b[i][0]; out[i].y0 = y + b[i][1]; out[i].z0 = z + b[i][2];
            out[i].x1 = x + b[i][3]; out[i].y1 = y + b[i][4]; out[i].z1 = z + b[i][5];
        }
        return n; }

    int getPlacedOnFaceDataValue(World*, int, int, int, int face, float, float clickY, float, int) {
        bool upperHalf = (face == F_DOWN) || (face != F_TOP && clickY > 0.5f);
        return upperHalf ? STAIR_UPSIDEDOWN_BIT : 0;
    }
    void setPlacedBy(World* w, int x, int y, int z, Player* p) {
        if (!p) return;
        static const int kQuadrantDir[4] = { 2, 0, 3, 1 };
        int q = ((int)floorf(p->yRot / 90.0f + 0.5f)) & 3;
        unsigned char d = worldData(w, x, y, z);
        worldSetData(w, x, y, z, (unsigned char)((d & ~STAIR_DIR_MASK) | kQuadrantDir[q]));
    } };

struct PaneTile : Tile { PaneTile(unsigned char i) : Tile(i) {}
    int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
        bool north = isSolidPhys(worldBlock(w, x, y, z - 1)) || isPane(worldBlock(w, x, y, z - 1));
        bool south = isSolidPhys(worldBlock(w, x, y, z + 1)) || isPane(worldBlock(w, x, y, z + 1));
        bool west  = isSolidPhys(worldBlock(w, x - 1, y, z)) || isPane(worldBlock(w, x - 1, y, z));
        bool east  = isSolidPhys(worldBlock(w, x + 1, y, z)) || isPane(worldBlock(w, x + 1, y, z));
        bool isolated = !north && !south && !west && !east;
        int num = 0;
        bool hasX = isolated || west || east || (!north && !south);
        if (hasX && num < 2) {
            float ax0 = (west && !east) ? (float)x : ((!west && east) ? x + 0.5f : (float)x);
            float ax1 = (west && !east) ? x + 0.5f : x + 1.0f;
            out[num].x0 = ax0; out[num].x1 = ax1;
            out[num].y0 = (float)y; out[num].y1 = y + 1.0f;
            out[num].z0 = z + 7.0f/16.0f; out[num].z1 = z + 9.0f/16.0f;
            num++;
        }
        bool hasZ = isolated || north || south || (!west && !east);
        if (hasZ && num < 2) {
            float az0 = (north && !south) ? (float)z : ((!north && south) ? z + 0.5f : (float)z);
            float az1 = (north && !south) ? z + 0.5f : z + 1.0f;
            out[num].x0 = x + 7.0f/16.0f; out[num].x1 = x + 9.0f/16.0f;
            out[num].y0 = (float)y; out[num].y1 = y + 1.0f;
            out[num].z0 = az0; out[num].z1 = az1;
            num++;
        }
        return num; } };

struct FenceTile : Tile { FenceTile(unsigned char i) : Tile(i) {}
    int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
        bool fn = connectsFence(worldBlock(w, x, y, z - 1));
        bool fs = connectsFence(worldBlock(w, x, y, z + 1));
        bool fw = connectsFence(worldBlock(w, x - 1, y, z));
        bool fe = connectsFence(worldBlock(w, x + 1, y, z));
        out[0].x0 = fw ? (float)x : x + 6.0f/16.0f;
        out[0].x1 = fe ? x + 1.0f : x + 10.0f/16.0f;
        out[0].z0 = fn ? (float)z : z + 6.0f/16.0f;
        out[0].z1 = fs ? z + 1.0f : z + 10.0f/16.0f;
        out[0].y0 = (float)y; out[0].y1 = y + 1.5f;
        return 1; } };

struct DoorTile : Tile { DoorTile(unsigned char i) : Tile(i) {}
    int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
        unsigned char data = worldData(w, x, y, z);
        bool isUpper = (data & 8) != 0;
        int lowerData = isUpper ? worldData(w, x, y - 1, z) : data;
        int upperData = isUpper ? data : worldData(w, x, y + 1, z);
        float r = 3.0f / 16.0f;
        int dir = lowerData & 3;
        bool open = (lowerData & 4) != 0;
        bool rightHinge = (upperData & 1) != 0;
        float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 1.0f, y1 = 1.0f, z1 = 1.0f;
        if (dir == 0) {
            if (open) { if (!rightHinge) { x0 = 0; x1 = 1; z0 = 0; z1 = r; } else { x0 = 0; x1 = 1; z0 = 1 - r; z1 = 1; } }
            else { x0 = 0; x1 = r; z0 = 0; z1 = 1; }
        } else if (dir == 1) {
            if (open) { if (!rightHinge) { x0 = 1 - r; x1 = 1; z0 = 0; z1 = 1; } else { x0 = 0; x1 = r; z0 = 0; z1 = 1; } }
            else { x0 = 0; x1 = 1; z0 = 0; z1 = r; }
        } else if (dir == 2) {
            if (open) { if (!rightHinge) { x0 = 0; x1 = 1; z0 = 1 - r; z1 = 1; } else { x0 = 0; x1 = 1; z0 = 0; z1 = r; } }
            else { x0 = 1 - r; x1 = 1; z0 = 0; z1 = 1; }
        } else if (dir == 3) {
            if (open) { if (!rightHinge) { x0 = 0; x1 = r; z0 = 0; z1 = 1; } else { x0 = 1 - r; x1 = 1; z0 = 0; z1 = 1; } }
            else { x0 = 0; x1 = 1; z0 = 1 - r; z1 = 1; }
        }
        out[0].x0 = x + x0; out[0].x1 = x + x1;
        out[0].y0 = y + y0; out[0].y1 = y + y1;
        out[0].z0 = z + z0; out[0].z1 = z + z1;
        return 1; }
    bool canSurvive(World* w, int x, int y, int z) { return supportCanSurvive(w, id, x, y, z, -1); }

    bool mayPlace(World* w, int x, int y, int z, int ) {
        if (y >= WORLD_H - 1) return false;
        return isSolidPhys(worldBlock(w, x, y - 1, z))
            && Tile::mayPlace(w, x, y, z)
            && Tile::mayPlace(w, x, y + 1, z);
    }

    bool use(World* w, int x, int y, int z, Player*) {
        unsigned char data = worldData(w, x, y, z);
        int lowerY = (data & 8) ? y - 1 : y;
        int upperY = (data & 8) ? y : y + 1;
        unsigned char lowerData = worldData(w, x, lowerY, z);
        worldSetBlockAndData(w, x, lowerY, z, worldBlock(w, x, lowerY, z), lowerData ^ 4);
        worldRebuildAroundNow(w, x, lowerY, z);
        worldRebuildAroundNow(w, x, upperY, z);
        playDoorSound(x, y, z, ((lowerData ^ 4) & 4) != 0);
        return true;
    } };

struct TrapdoorTile : Tile { TrapdoorTile(unsigned char i) : Tile(i) {}
    int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
        unsigned char data = worldData(w, x, y, z);
        bool open = (data & 4) != 0;
        float r = 3.0f / 16.0f;
        float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 1.0f, y1 = r, z1 = 1.0f;
        if (open) {
            if ((data & 3) == 0) { x0 = 0; x1 = 1; y0 = 0; y1 = 1; z0 = 1 - r; z1 = 1; }
            else if ((data & 3) == 1) { x0 = 0; x1 = 1; y0 = 0; y1 = 1; z0 = 0; z1 = r; }
            else if ((data & 3) == 2) { x0 = 1 - r; x1 = 1; y0 = 0; y1 = 1; z0 = 0; z1 = 1; }
            else if ((data & 3) == 3) { x0 = 0; x1 = r; y0 = 0; y1 = 1; z0 = 0; z1 = 1; }
        }
        out[0].x0 = x + x0; out[0].x1 = x + x1;
        out[0].y0 = y + y0; out[0].y1 = y + y1;
        out[0].z0 = z + z0; out[0].z1 = z + z1;
        return 1; }
    bool canSurvive(World* w, int x, int y, int z) { return supportCanSurvive(w, id, x, y, z, -1); }
    bool mayPlace(World* w, int x, int y, int z, int face) {
        if (!Tile::mayPlace(w, x, y, z)) return false;

        return supportCanSurvive(w, id, x, y, z,
                                 getPlacedOnFaceDataValue(w, x, y, z, face, 0.0f, 0.0f, 0.0f, 0));
    }

    bool use(World* w, int x, int y, int z, Player*) {
        unsigned char data = worldData(w, x, y, z);
        worldSetBlockAndData(w, x, y, z, id, data ^ 4);
        worldRebuildAroundNow(w, x, y, z);
        playDoorSound(x, y, z, ((data ^ 4) & 4) != 0);
        return true;
    }

    int getPlacedOnFaceDataValue(World*, int, int, int, int face, float, float, float, int) {
        switch (face) { case F_BACK: return 0; case F_FORWARD: return 1;
                        case F_LEFT: return 2; case F_RIGHT: return 3; default: return 0; }
    } };

struct LadderTile : Tile { LadderTile(unsigned char i) : Tile(i) {}

    int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
        unsigned char data = worldData(w, x, y, z);
        const float r = 2.0f / 16.0f;
        if (data == 2)      out[0] = { x + 0.0f,     y + 0.0f, z + 1.0f - r, x + 1.0f, y + 1.0f, z + 1.0f };
        else if (data == 3) out[0] = { x + 0.0f,     y + 0.0f, z + 0.0f,     x + 1.0f, y + 1.0f, z + r    };
        else if (data == 4) out[0] = { x + 1.0f - r, y + 0.0f, z + 0.0f,     x + 1.0f, y + 1.0f, z + 1.0f };
        else if (data == 5) out[0] = { x + 0.0f,     y + 0.0f, z + 0.0f,     x + r,    y + 1.0f, z + 1.0f };
        else return 0;
        return 1; }
    bool canSurvive(World* w, int x, int y, int z) { return supportCanSurvive(w, id, x, y, z, -1); }
    bool mayPlace(World* w, int x, int y, int z, int face) {
        if (!Tile::mayPlace(w, x, y, z)) return false;

        return supportCanSurvive(w, id, x, y, z,
                                 getPlacedOnFaceDataValue(w, x, y, z, face, 0.0f, 0.0f, 0.0f, 0));
    }

    int getPlacedOnFaceDataValue(World*, int, int, int, int face, float, float, float, int) {
        switch (face) { case F_BACK: return 2; case F_FORWARD: return 3;
                        case F_LEFT: return 4; case F_RIGHT: return 5; default: return 2; }
    } };

struct FenceGateTile : Tile { FenceGateTile(unsigned char i) : Tile(i) {}
    int getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
        unsigned char data = worldData(w, x, y, z);
        bool open = (data & 4) != 0;
        if (open) return 0;
        int dir = data & 3;
        if (dir == 0 || dir == 2) {
            out[0].x0 = (float)x; out[0].x1 = x + 1.0f;
            out[0].y0 = (float)y; out[0].y1 = y + 1.5f;
            out[0].z0 = z + 6.0f/16.0f; out[0].z1 = z + 10.0f/16.0f;
        } else {
            out[0].x0 = x + 6.0f/16.0f; out[0].x1 = x + 10.0f/16.0f;
            out[0].y0 = (float)y; out[0].y1 = y + 1.5f;
            out[0].z0 = (float)z; out[0].z1 = z + 1.0f;
        }
        return 1; }

    bool use(World* w, int x, int y, int z, Player* placer) {
        unsigned char data = worldData(w, x, y, z);
        bool isOpen = (data & 4) != 0;
        if (isOpen) data = data & ~4;
        else {
            int pDir = facingQuadrant(placer);
            if ((data & 3) == ((pDir + 2) % 4)) data = (data & ~3) | pDir;
            data |= 4;
        }
        worldSetBlockAndData(w, x, y, z, id, data);
        worldRebuildAroundNow(w, x, y, z);
        playDoorSound(x, y, z, !isOpen);
        return true;
    }

    void setPlacedBy(World* w, int x, int y, int z, Player* p) {
        worldSetData(w, x, y, z, (unsigned char)facingQuadrant(p));
    } };

struct ChestTile : Tile { ChestTile(unsigned char i) : Tile(i) {}
    int getAABB(const World*, int x, int y, int z, BlockAABB out[3]) {
        const float m = 0.025f;
        out[0] = { x + m, y + 0.0f, z + m, x + 1.0f - m, y + 1.0f - 2*m, z + 1.0f - m };
        return 1; }

    bool mayPlace(World* w, int x, int y, int z) {
        if (!Tile::mayPlace(w, x, y, z)) return false;
        return worldBlock(w, x - 1, y, z) != BLOCK_CHEST && worldBlock(w, x + 1, y, z) != BLOCK_CHEST
            && worldBlock(w, x, y, z - 1) != BLOCK_CHEST && worldBlock(w, x, y, z + 1) != BLOCK_CHEST; }

    void setPlacedBy(World* w, int x, int y, int z, Player* p) {
        if (p) worldSetData(w, x, y, z, (unsigned char)facingFromYaw(p->yRot));
        if (!g_level.getTileEntity(x, y, z)) g_level.setTileEntity(x, y, z, new ChestTileEntity());
    }

    bool use(World* w, int x, int y, int z, Player*) {
        if (g_gameMode && !g_gameMode->isCreative() && !isOpaque(worldBlock(w, x, y + 1, z))) {
            TileEntity* te = g_level.getTileEntity(x, y, z);
            if (!te) {
                g_level.setTileEntity(x, y, z, new ChestTileEntity());
                te = g_level.getTileEntity(x, y, z);
            }
            if (te && te->type == TE_CHEST) guiOpenChest((ChestTileEntity*)te);
        }
        return true;
    } };

struct FurnaceTile : Tile { FurnaceTile(unsigned char i) : Tile(i) {}
    void setPlacedBy(World* w, int x, int y, int z, Player* p) {
        if (p) worldSetData(w, x, y, z, (unsigned char)facingFromYaw(p->yRot));
        if (!g_level.getTileEntity(x, y, z)) g_level.setTileEntity(x, y, z, new FurnaceTileEntity());
    }

    bool use(World*, int x, int y, int z, Player*) {
        if (g_gameMode && !g_gameMode->isCreative()) {
            TileEntity* te = g_level.getTileEntity(x, y, z);
            if (!te) { g_level.setTileEntity(x, y, z, new FurnaceTileEntity());
                       te = g_level.getTileEntity(x, y, z); }
            if (te && te->type == TE_FURNACE) guiOpenFurnace((FurnaceTileEntity*)te);
        }
        return true;
    } };

struct WorkbenchTile : Tile { WorkbenchTile(unsigned char i) : Tile(i) {}
    bool use(World*, int, int, int, Player*) {
        if (g_gameMode && !g_gameMode->isCreative()) guiOpenCrafting(id == BLOCK_STONECUTTER);
        return true;
    } };

struct ReactorTile : Tile { ReactorTile(unsigned char i) : Tile(i) {}
    void setPlacedBy(World*, int x, int y, int z, Player*) {
        if (!g_level.getTileEntity(x, y, z)) g_level.setTileEntity(x, y, z, new ReactorTileEntity());
    }
    bool use(World*, int x, int y, int z, Player* p) {
        if (g_gameMode && !g_gameMode->isCreative()) {
            if (!g_level.getTileEntity(x, y, z))
                g_level.setTileEntity(x, y, z, new ReactorTileEntity());
            NetherReactor::use(&g_level, x, y, z, g_level.player);
        }
        return true;
    } };

struct HeavyTile : Tile { HeavyTile(unsigned char i) : Tile(i) {}
    void neighborChanged(World* w, int x, int y, int z) { worldScheduleTick(w, x, y, z, id, 2); } };

struct RedStoneOreTile : Tile { RedStoneOreTile(unsigned char i) : Tile(i) {}
    bool use(World* w, int x, int y, int z, Player*) { redstoneOreInteract(w, x, y, z); return false; }
    void attack(World* w, int x, int y, int z, Player*) { redstoneOreInteract(w, x, y, z); } };

struct FireTile : Tile { FireTile(unsigned char i) : Tile(i) {}
    bool mayPlace(World* w, int x, int y, int z, int) { return fireMayPlace(w, x, y, z); }
    bool mayPlace(World* w, int x, int y, int z)      { return fireMayPlace(w, x, y, z); }
    void neighborChanged(World* w, int x, int y, int z) { fireNeighborChanged(w, x, y, z); }
    Drop getResource(int) { Drop d = { 0, 0, 0 }; return d; } };

struct WebTile : Tile { WebTile(unsigned char i) : Tile(i) {}
    void entityInside(World*, int, int, int, Entity* e) { if (e) e->makeStuckInWeb(); } };

struct SupportTile : Tile { SupportTile(unsigned char i) : Tile(i) {}
    bool canSurvive(World* w, int x, int y, int z) { return supportCanSurvive(w, id, x, y, z, -1); }
    bool mayPlace(World* w, int x, int y, int z, int face) {
        if (!Tile::mayPlace(w, x, y, z)) return false;

        return supportCanSurvive(w, id, x, y, z,
                                 getPlacedOnFaceDataValue(w, x, y, z, face, 0.0f, 0.0f, 0.0f, 0));
    } };

struct SignTile : SupportTile { SignTile(unsigned char i) : SupportTile(i) {}
    bool use(World* w, int x, int y, int z, Player* p) {
        if (p && p->inventory->getSelected() && !p->inventory->getSelected()->isNull())
            return false;
        TileEntity* te = g_level.getTileEntity(x, y, z);
        if (te && te->type == TE_SIGN) { guiOpenSignEditor((SignTileEntity*)te); return true; }
        return false;
    } };

struct TorchTile : SupportTile { TorchTile(unsigned char i) : SupportTile(i) {}
    int getPlacedOnFaceDataValue(World*, int, int, int, int face, float, float, float, int) {
        switch (face) { case F_TOP: return 5; case F_LEFT: return 2; case F_RIGHT: return 1;
                        case F_BACK: return 4; case F_FORWARD: return 3; default: return 5; }
    } };

struct GrassTile : Tile { GrassTile(unsigned char i) : Tile(i) { randomTicks = true; }
    void randomTick(World* w, int x, int y, int z) {

        int lightAbove = lightRawAt(w, x, y + 1, z);
        unsigned char blockAbove = worldBlock(w, x, y + 1, z);
        if (lightAbove < 4 && lightOpacity(blockAbove) > 0) {
            worldSetBlockAndData(w, x, y, z, BLOCK_DIRT, 0);
            worldNotifyNeighborsChanged(w, x, y, z);
        } else if (lightAbove >= 9) {
            int xt = x + (rand() % 3) - 1, yt = y + (rand() % 5) - 3, zt = z + (rand() % 3) - 1;
            if (worldBlock(w, xt, yt, zt) == BLOCK_DIRT &&
                lightRawAt(w, xt, yt + 1, zt) >= 4 && lightOpacity(worldBlock(w, xt, yt + 1, zt)) == 0) {
                worldSetBlockAndData(w, xt, yt, zt, BLOCK_GRASS, 0);
                worldNotifyNeighborsChanged(w, xt, yt, zt);
            }
        } } };

struct LeafTile : Tile { LeafTile(unsigned char i) : Tile(i) { randomTicks = true; }

    void setPlacedBy(World* w, int x, int y, int z, Player* p) {
        if (p) worldSetData(w, x, y, z, (unsigned char)(worldData(w, x, y, z) | LEAF_PERSISTENT_BIT));
    }
    void randomTick(World* w, int x, int y, int z) { leafDecayTick(w, x, y, z); } };

struct GrowerTile : Tile { GrowerTile(unsigned char i) : Tile(i) { randomTicks = true; }
    virtual void grow(World* w, int x, int y, int z) = 0;
    void randomTick(World* w, int x, int y, int z) {

        if (!canSurvive(w, x, y, z)) {
            worldSpawnResources(w, x, y, z, id, worldData(w, x, y, z));
            worldSetBlockAndData(w, x, y, z, BLOCK_AIR, 0);
            worldNotifyNeighborsChanged(w, x, y, z);
            return;
        }
        grow(w, x, y, z); } };

struct BushTile : GrowerTile { BushTile(unsigned char i) : GrowerTile(i) {}
    bool canSurvive(World* w, int x, int y, int z) { return bushFamilyCanSurvive(w, id, x, y, z); }
    bool mayPlace(World* w, int x, int y, int z) { return Tile::mayPlace(w, x, y, z) && canSurvive(w, x, y, z); }
    void grow(World* w, int x, int y, int z) {
        if (id == BLOCK_WHEAT)              cropTick(w, x, y, z);
        else if (id == BLOCK_MELON_STEM)    stemTick(w, x, y, z);
        else if (id == BLOCK_SAPLING)       saplingTick(w, x, y, z);
        else if (id == BLOCK_MUSHROOM_BROWN || id == BLOCK_MUSHROOM_RED) mushroomTick(w, x, y, z);
         }

    void spawnResources(World* w, int x, int y, int z, int data, Random& rng) {
        Tile::spawnResources(w, x, y, z, data, rng);
        short seed = (id == BLOCK_WHEAT)      ? ITEM_SEEDS_WHEAT
                   : (id == BLOCK_MELON_STEM) ? ITEM_SEEDS_MELON : 0;
        if (seed)
            for (int i = 0; i < 3; i++)
                if (rng.nextInt(5 * 3) <= data)
                    dropItem(x, y, z, seed, 0, rng);
    } };

struct ReedTile : GrowerTile { ReedTile(unsigned char i) : GrowerTile(i) {}
    bool canSurvive(World* w, int x, int y, int z) { return reedCanSurvive(w, x, y, z); }
    bool mayPlace(World* w, int x, int y, int z) { return Tile::mayPlace(w, x, y, z) && canSurvive(w, x, y, z); }
    void grow(World* w, int x, int y, int z) { reedCactusGrow(w, x, y, z, id, 15); } };

struct CactusTile : GrowerTile { CactusTile(unsigned char i) : GrowerTile(i) {}

    int getAABB(const World*, int x, int y, int z, BlockAABB out[3]) {
        const float r = 1.0f / 16.0f;
        out[0].x0 = x + r;       out[0].x1 = x + 1.0f - r;
        out[0].y0 = (float)y;    out[0].y1 = y + 1.0f - r;
        out[0].z0 = z + r;       out[0].z1 = z + 1.0f - r;
        return 1;
    }
    bool canSurvive(World* w, int x, int y, int z) { return cactusCanSurvive(w, x, y, z); }
    bool mayPlace(World* w, int x, int y, int z) { return Tile::mayPlace(w, x, y, z) && canSurvive(w, x, y, z); }
    void grow(World* w, int x, int y, int z) { reedCactusGrow(w, x, y, z, id, 10); }

    void entityInside(World*, int, int, int, Entity* e) { if (e) e->hurt(nullptr, 1); } };

static bool rawSolidPhys(unsigned char id) {
    if (id == BLOCK_AIR || isLiquidId(id)) return false;

    if (isCrossShaped(id)) return false;

    if (id == BLOCK_CACTUS) return false;
    if (id == BLOCK_TOPSNOW || id == BLOCK_TORCH) return false;
    if (id == BLOCK_FIRE) return false;
    if (isSign(id)) return false;
    if (id == BLOCK_LADDER) return false;
    return true;
}
static bool rawCube(unsigned char id) {
    if (id == BLOCK_AIR || id == BLOCK_INVISIBLE_BEDROCK || isLiquidId(id)) return false;
    if (isCrossShaped(id)) return false;
    if (id == BLOCK_CACTUS || id == BLOCK_TOPSNOW || id == BLOCK_TORCH) return false;
    if (isFence(id) || isFenceGate(id) || isPane(id)) return false;
    if (isStairs(id) || isSlab(id)) return false;
    if (id == BLOCK_TRAPDOOR || isDoor(id) || id == BLOCK_LADDER || id == BLOCK_TORCH || isBed(id)) return false;
    if (id == BLOCK_FIRE) return false;
    if (isSign(id)) return false;
    if (id == BLOCK_CHEST) return false;
    return true;
}
static bool rawOpaque(unsigned char id) {
    return id != BLOCK_AIR && !isLiquidId(id) && id != BLOCK_ICE &&
           id != BLOCK_LEAVES && id != BLOCK_GLASS && id != BLOCK_SAPLING && id != BLOCK_GLASS_PANE &&
           !isCrossShaped(id) && id != BLOCK_CACTUS && id != BLOCK_TOPSNOW && id != BLOCK_REEDS &&
           !isSlab(id) && !isStairs(id) && id != BLOCK_FENCE && id != BLOCK_LADDER && id != BLOCK_TORCH &&
           !isDoor(id) && !isTrapdoor(id) && !isFenceGate(id) && !isBed(id) && id != BLOCK_FARMLAND &&
           id != BLOCK_CHEST && !isSign(id) && id != BLOCK_FIRE;
}
static bool rawReplaceable(unsigned char id) {

    return id == BLOCK_AIR || isLiquidId(id) || id == BLOCK_TOPSNOW ||
           id == BLOCK_TALLGRASS || id == BLOCK_FIRE;
}
static int rawLightOpacity(unsigned char id) {
    if (id == BLOCK_AIR || id == BLOCK_INVISIBLE_BEDROCK) return 0;
    if (isWaterId(id) || id == BLOCK_ICE) return 3;
    if (isLavaId(id)) return 15;
    if (id == BLOCK_LEAVES) return 1;
    if (isCrossShaped(id) ||
        id == BLOCK_CACTUS || id == BLOCK_TOPSNOW ||
        id == BLOCK_GLASS || id == BLOCK_GLASS_PANE ||
        isFence(id) || isStairs(id) || isSlab(id) || isDoor(id) ||
        isTrapdoor(id) || isFenceGate(id) || id == BLOCK_LADDER || id == BLOCK_TORCH || isBed(id) ||
        id == BLOCK_FARMLAND || isSign(id) || id == BLOCK_FIRE ||
        id == BLOCK_CHEST) return 0;
    return 15;
}
static int rawLightEmit(unsigned char id) {
    if (isLavaId(id) || id == BLOCK_GLOWSTONE || id == BLOCK_FIRE) return 15;
    if (id == BLOCK_TORCH) return 14;
    if (id == BLOCK_GLOWING_OBSIDIAN) return 13;

    if (id == BLOCK_ORE_REDSTONE_LIT) return 9;
    if (id == BLOCK_FURNACE_LIT) return 13;
    if (id == BLOCK_MUSHROOM_BROWN) return 1;
    return 0;
}

static int rawSoundType(unsigned char id) {
    if (id == BLOCK_AIR || id == BLOCK_INVISIBLE_BEDROCK || isUpdateBlock(id) ||
        isLiquidId(id))
        return SOUND_SILENT;

    switch (id) {
        case BLOCK_GRASS: case BLOCK_LEAVES: case BLOCK_FLOWER: case BLOCK_ROSE:
        case BLOCK_MUSHROOM_BROWN: case BLOCK_MUSHROOM_RED: case BLOCK_SAPLING:
        case BLOCK_REEDS: case BLOCK_WHEAT: case BLOCK_TNT: case BLOCK_TALLGRASS:
            return SOUND_GRASS;

        case BLOCK_DIRT: case BLOCK_GRAVEL: case BLOCK_CLAY: case BLOCK_FARMLAND:
            return SOUND_GRAVEL;

        case BLOCK_SAND:
            return SOUND_SAND;

        case BLOCK_WOOL: case BLOCK_TOPSNOW: case BLOCK_SNOW_BLOCK: case BLOCK_CACTUS:
            return SOUND_CLOTH;

        case BLOCK_GLASS: case BLOCK_GLASS_PANE: case BLOCK_ICE: case BLOCK_GLOWSTONE:
            return SOUND_GLASS;

        case BLOCK_GOLD_BLOCK: case BLOCK_IRON_BLOCK: case BLOCK_DIAMOND_BLOCK:
        case BLOCK_LAPIS_BLOCK: case BLOCK_DOOR_IRON:
            return SOUND_METAL;

        case BLOCK_LOG:
        case BLOCK_PLANKS: case BLOCK_BOOKSHELF: case BLOCK_CRAFTING_TABLE:
        case BLOCK_CHEST: case BLOCK_FENCE: case BLOCK_FENCE_GATE:
        case BLOCK_DOOR_WOOD: case BLOCK_TRAPDOOR: case BLOCK_LADDER:
        case BLOCK_TORCH: case BLOCK_SIGN: case BLOCK_WALL_SIGN:
        case BLOCK_MELON: case BLOCK_MELON_STEM:
        case BLOCK_FIRE:

        case BLOCK_STAIRS_PLANKS:
            return SOUND_WOOD;

        default:
            return SOUND_STONE;
    }
}

static int shapeOf(unsigned char id) {
    if (id == BLOCK_AIR || id == BLOCK_INVISIBLE_BEDROCK) return SHAPE_AIR;
    if (isLiquidId(id))         return SHAPE_LIQUID;
    if (isSlab(id))             return SHAPE_SLAB;
    if (isStairs(id))           return SHAPE_STAIRS;
    if (isPane(id))             return SHAPE_PANE;
    if (isFence(id))            return SHAPE_FENCE;
    if (isFenceGate(id))        return SHAPE_FENCEGATE;
    if (isDoor(id))             return SHAPE_DOOR;
    if (isTrapdoor(id))         return SHAPE_TRAPDOOR;
    if (isLadder(id))           return SHAPE_LADDER;
    if (isTorch(id))            return SHAPE_TORCH;
    if (isBed(id))              return SHAPE_BED;
    if (isSign(id))             return SHAPE_SIGN;
    if (id == BLOCK_CHEST)      return SHAPE_CHEST;
    if (id == BLOCK_FIRE)       return SHAPE_FIRE;
    if (id == BLOCK_CACTUS)     return SHAPE_CACTUS;
    if (id == BLOCK_TOPSNOW)    return SHAPE_TOPSNOW;
    if (id == BLOCK_REEDS)      return SHAPE_REEDS;
    if (id == BLOCK_WHEAT)      return SHAPE_WHEAT;
    if (id == BLOCK_MELON_STEM) return SHAPE_MELON_STEM;
    if (isCrossShaped(id))            return SHAPE_CROSS;
    return SHAPE_CUBE;
}

static Tile* makeTile(unsigned char id) {

    switch (id) {
        case BLOCK_FARMLAND: return new FarmTile(id);
        case BLOCK_SAND: case BLOCK_GRAVEL: return new HeavyTile(id);
        case BLOCK_COBWEB:                  return new WebTile(id);
        case BLOCK_ORE_REDSTONE: case BLOCK_ORE_REDSTONE_LIT:
            return new RedStoneOreTile(id);
        case BLOCK_FIRE:     return new FireTile(id);
        case BLOCK_GRASS:    return new GrassTile(id);
        case BLOCK_LEAVES:   return new LeafTile(id);
        case BLOCK_CACTUS:   return new CactusTile(id);
        case BLOCK_REEDS:    return new ReedTile(id);
        case BLOCK_FLOWER: case BLOCK_ROSE: case BLOCK_SAPLING:
        case BLOCK_WHEAT: case BLOCK_MELON_STEM: case BLOCK_TALLGRASS:
        case BLOCK_MUSHROOM_BROWN: case BLOCK_MUSHROOM_RED:
            return new BushTile(id);
        case BLOCK_TOPSNOW:
            return new SupportTile(id);
        case BLOCK_SIGN: case BLOCK_WALL_SIGN:
            return new SignTile(id);
        case BLOCK_TORCH:
            return new TorchTile(id);
        case BLOCK_FURNACE: case BLOCK_FURNACE_LIT:
            return new FurnaceTile(id);
        case BLOCK_CRAFTING_TABLE: case BLOCK_STONECUTTER:
            return new WorkbenchTile(id);
        case BLOCK_NETHER_REACTOR:
            return new ReactorTile(id);
        default: break;
    }
    switch (shapeOf(id)) {
        case SHAPE_SLAB:      return new SlabTile(id);
        case SHAPE_STAIRS:    return new StairTile(id);
        case SHAPE_PANE:      return new PaneTile(id);
        case SHAPE_FENCE:     return new FenceTile(id);
        case SHAPE_FENCEGATE: return new FenceGateTile(id);
        case SHAPE_DOOR:      return new DoorTile(id);
        case SHAPE_TRAPDOOR:  return new TrapdoorTile(id);
        case SHAPE_LADDER:    return new LadderTile(id);
        case SHAPE_BED:       return new BedTile(id);
        case SHAPE_CHEST:     return new ChestTile(id);
        default:              return new Tile(id);
    }
}

void Tile::initTiles() {
    for (int id = 0; id < 256; id++) {
        Tile* t = makeTile((unsigned char)id);
        t->shape         = shapeOf((unsigned char)id);
        t->solidPhys     = rawSolidPhys((unsigned char)id);
        t->cube          = rawCube((unsigned char)id);
        t->opaque        = rawOpaque((unsigned char)id);
        t->replaceable   = rawReplaceable((unsigned char)id);
        t->lightBlock    = (unsigned char)rawLightOpacity((unsigned char)id);
        t->lightEmission = (unsigned char)rawLightEmit((unsigned char)id);
        t->soundType     = (unsigned char)rawSoundType((unsigned char)id);
        tiles[id] = t;
    }
}
