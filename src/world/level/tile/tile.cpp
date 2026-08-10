
#include "world/level/tile/tile.h"
#include "world/level/tile/material.h"
#include "world/level/tile/tiles.h"
#include "world/level/tile/tile_behavior.h"
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"
#include "world/level/level.h"
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
    static const int kQuadrantFace[4] = { 2 , 4 , 3 , 5  };
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

int Tile::getTileAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
    float b[3][6];
    int n = tileShapeBoxes(w, x, y, z, id, worldData(w, x, y, z), b);
    for (int i = 0; i < n; i++) {
        out[i].x0 = b[i][0]; out[i].y0 = b[i][1]; out[i].z0 = b[i][2];
        out[i].x1 = b[i][3]; out[i].y1 = b[i][4]; out[i].z1 = b[i][5];
    }
    return n;
}

int Tile::getAABB(const World* w, int x, int y, int z, BlockAABB out[3]) {
    if (!solidPhys) return 0;
    return getTileAABB(w, x, y, z, out);
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

        case BLOCK_WOOD_SLAB_DOUBLE:    return { BLOCK_WOOD_SLAB, 2, (short)(data & DSLAB_MAT_MASK) };
        case BLOCK_WOOD_SLAB:           return { BLOCK_WOOD_SLAB, 1, (short)(data & DSLAB_MAT_MASK) };
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
        case BLOCK_BAMBOO:               return { BLOCK_BAMBOO, 1, 0 };
        case BLOCK_VINE:                 return { BLOCK_VINE, 1, 0 };
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

static void dropItem(int x, int y, int z, short id, short aux, Random&) {
    Tile::popResource(x, y, z, ItemInstance(id, 1, aux));
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

        case BLOCK_WOOD_SLAB:
        case BLOCK_WOOD_SLAB_DOUBLE:
            Tile::tiles[BLOCK_PLANKS]->getTexture(0, f, col, row, tint); return;
        case BLOCK_SLAB:
        case BLOCK_DOUBLE_SLAB:
            switch (data & DSLAB_MAT_MASK) {
                case DSLAB_SAND:        Tile::tiles[BLOCK_SANDSTONE]->getTexture(0, f, col, row, tint); return;
                case DSLAB_WOOD:        Tile::tiles[BLOCK_PLANKS]->getTexture(0, f, col, row, tint); return;
                case DSLAB_COBBLE:      Tile::tiles[BLOCK_COBBLESTONE]->getTexture(0, f, col, row, tint); return;
                case DSLAB_BRICK:       Tile::tiles[BLOCK_BRICKS]->getTexture(0, f, col, row, tint); return;
                case DSLAB_SMOOTHBRICK: Tile::tiles[BLOCK_STONE_BRICKS]->getTexture(0, f, col, row, tint); return;
                case DSLAB_QUARTZ:      Tile::tiles[BLOCK_QUARTZ_BLOCK]->getTexture(0, f, col, row, tint); return;
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
                case LOG_JUNGLE: *col = 9; *row = 9; break;
                default:         *col = 4; *row = 1; break;
            }
            break;
        case BLOCK_LEAVES:
            switch (data & LEAF_TYPE_MASK) {
                case LEAF_SPRUCE: *col = 4; *row = 8; *tint = 0xFF2BAE3Du; break;
                case LEAF_BIRCH:  *col = 4; *row = 3; *tint = 0xFF55A780u; break;
                case LEAF_JUNGLE: *col = 4; *row = 3; *tint = 0xFF2AB01Du; break;
                default:          *col = 4; *row = 3; *tint = 0xFF18B548u; break;
            }
            break;
        case BLOCK_COBWEB:         *col = 11; *row = 0; break;

        case BLOCK_TALLGRASS:
            switch (data & 3) {
                case TG_FERN:       *col = 8; *row = 3; *tint = 0xFF339933u; break;
                case TG_TALL_GRASS: *col = 7; *row = 2; *tint = 0xFF339933u; break;
                default:            *col = 7; *row = 3; break;
            }
            break;
        case BLOCK_BAMBOO:         *col = 10; *row = 10; break;
        case BLOCK_COCOA: {
            int age = (data >> COCOA_AGE_SHIFT) & COCOA_AGE_MASK;
            *col = 11; *row = 10;
            *tint = (age >= 2) ? 0xFFFFFFFFu : 0xFFB0B0B0u;
            break;
        }
        case BLOCK_VINE:           *col = 12; *row = 10; *tint = 0xFF3E8A28u; break;
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
    bool canSurvive(World* w, int x, int y, int z) { return supportCanSurvive(w, id, x, y, z, -1); }

};

struct CakeTile : Tile { CakeTile(unsigned char i) : Tile(i) {}

    void getTexture(unsigned char data, int f, int* col, int* row, unsigned int* tint) {
        *tint = 0xFFFFFFFFu;
        *row = 7;
        if (f == F_TOP)                       *col = 9;
        else if (f == F_DOWN)                 *col = 12;
        else if (f == F_LEFT && (data & 7))   *col = 11;
        else                                  *col = 10;
    }

    bool mayPlace(World* w, int x, int y, int z, int face) {
        return Tile::mayPlace(w, x, y, z) && face == F_TOP; }
    bool canSurvive(World* w, int x, int y, int z) { return supportCanSurvive(w, id, x, y, z, -1); }

    Drop getResource(int) { return { 0, 0, 0 }; }

    bool use(World* w, int x, int y, int z, Player* p) {
        if (!p) return true;
        bool creative = g_gameMode && g_gameMode->isCreative();
        if (!creative && p->health >= p->getMaxHealth()) return true;
        if (!creative) p->heal(3);

        g_level.playSound(p, "random.burp", 0.5f,
                          (rand() / (float)RAND_MAX) * 0.1f + 0.9f);
        int d = (worldData(w, x, y, z) & 7) + 1;
        if (d <= 5) worldSetData(w, x, y, z, (unsigned char)d);
        else        worldSetTileUpdate(w, x, y, z, BLOCK_AIR, 0);
        worldRebuildAroundNow(w, x, y, z);
        return true;
    } };

struct FarmTile : Tile { FarmTile(unsigned char i) : Tile(i) { randomTicks = true; }

    int getAABB(const World*, int x, int y, int z, BlockAABB out[3]) {
        out[0] = { (float)x, (float)y, (float)z, x + 1.0f, y + 1.0f, z + 1.0f }; return 1; }
    void randomTick(World* w, int x, int y, int z) { tickFarmland(w, x, y, z); }

    void neighborChanged(World* w, int x, int y, int z) {
        Tile::neighborChanged(w, x, y, z);
        if (worldBlock(w, x, y, z) != BLOCK_FARMLAND) return;

        if (!materialOf(worldBlock(w, x, y + 1, z)).isSolid()) return;
        worldSetBlockAndData(w, x, y, z, BLOCK_DIRT, 0);
    }

    void fallOn(World* w, int x, int y, int z, Entity*, float dist) {

        if ((rand() / (float)RAND_MAX) < (dist - 0.5f)) {
            worldSetBlockAndData(w, x, y, z, BLOCK_DIRT, 0);
            worldNotifyNeighborsChanged(w, x, y, z);
            worldRebuildAroundNow(w, x, y, z);
        }
    } };

struct SlabTile : Tile { SlabTile(unsigned char i) : Tile(i) {}

    int getPlacedOnFaceDataValue(World*, int, int, int, int face,
                                 float, float clickY, float, int itemValue) {
        bool upperHalf = (face == F_DOWN) || (face != F_TOP && clickY > 0.5f);
        return (itemValue & DSLAB_MAT_MASK) | (upperHalf ? SLAB_TOP_SLOT_BIT : 0);
    } };

struct StairTile : Tile { StairTile(unsigned char i) : Tile(i) {}

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

void chestShapeBox(const World* w, int gx, int y, int gz, float out[6]) {
    const float A = 1.0f / 16.0f, B = 15.0f / 16.0f, H = 14.0f / 16.0f;
    out[0] = A; out[1] = 0.0f; out[2] = A; out[3] = B; out[4] = H; out[5] = B;
    (void)w;
    TileEntity* te = g_level.getTileEntity(gx, y, gz);
    if (!te || te->type != TE_CHEST) return;
    ChestTileEntity* ce = (ChestTileEntity*)te;
    if (!ce->pair) return;
    int dx = ce->pair->x - gx, dz = ce->pair->z - gz;
    if      (dz < 0) out[2] = 0.0f;
    else if (dz > 0) out[5] = 1.0f;
    else if (dx < 0) out[0] = 0.0f;
    else if (dx > 0) out[3] = 1.0f;
}

struct ChestTile : Tile { ChestTile(unsigned char i) : Tile(i) {}

    void setPlacedBy(World* w, int x, int y, int z, Player* p) {

        worldSetData(w, x, y, z, (unsigned char)(p ? facingFromYaw(p->yRot) : 4 ));

        if (g_level.getTileEntity(x, y, z)) return;
        ChestTileEntity* ce = new ChestTileEntity();
        g_level.setTileEntity(x, y, z, ce);

        static const int D[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
        for (int k = 0; k < 4; k++) {
            int nx = x + D[k][0], nz = z + D[k][1];
            if (worldBlock(w, nx, y, nz) != BLOCK_CHEST) continue;
            TileEntity* t = g_level.getTileEntity(nx, y, nz);
            if (!t || t->type != TE_CHEST) continue;
            ChestTileEntity* other = (ChestTileEntity*)t;
            if (other->pair || !other->canPairWith(ce)) continue;
            other->pairWith(ce);
            ce->pairWith(other);
            break;
        }
    }

    bool use(World*, int x, int y, int z, Player*) {
        TileEntity* te = g_level.getTileEntity(x, y, z);
        if (!te) {
            g_level.setTileEntity(x, y, z, new ChestTileEntity());
            te = g_level.getTileEntity(x, y, z);
        }
        if (te && te->type == TE_CHEST) {
            ChestTileEntity* ce = (ChestTileEntity*)te;
            if (ce->canOpen()) ce->openBy();
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
    bool canSurvive(World* w, int x, int y, int z) {
        if (id == BLOCK_TALLGRASS && (worldData(w, x, y, z) & 8))
            return fernTopCanSurvive(w, x, y, z);
        return bushFamilyCanSurvive(w, id, x, y, z);
    }

    bool mayPlace(World* w, int x, int y, int z) {
        if (!Tile::mayPlace(w, x, y, z)) return false;
        if (id == BLOCK_MUSHROOM_BROWN || id == BLOCK_MUSHROOM_RED) return canSurvive(w, x, y, z);
        return bushMayPlaceOn(w, id, x, y, z);
    }
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

struct BambooTile : GrowerTile { BambooTile(unsigned char i) : GrowerTile(i) {}
    bool canSurvive(World* w, int x, int y, int z) { return bambooCanSurvive(w, x, y, z); }
    bool mayPlace(World* w, int x, int y, int z) { return Tile::mayPlace(w, x, y, z) && canSurvive(w, x, y, z); }
    void grow(World* w, int x, int y, int z) { bambooGrow(w, x, y, z, 3, 12); } };

struct VineTile : GrowerTile { VineTile(unsigned char i) : GrowerTile(i) {}
    bool canSurvive(World* w, int x, int y, int z) { return vineCanSurvive(w, x, y, z); }
    bool mayPlace(World* w, int x, int y, int z) { return Tile::mayPlace(w, x, y, z) && canSurvive(w, x, y, z); }

    void grow(World* w, int x, int y, int z) {
        if (worldBlock(w, x, y - 1, z) != BLOCK_AIR) return;
        int length = 1;
        while (worldBlock(w, x, y - length, z) == BLOCK_VINE) length++;
        if (length >= 6) return;
        if (rand() % 4 == 0)
            worldSetTileUpdate(w, x, y - 1, z, BLOCK_VINE, 0);
    } };

struct CocoaTile : GrowerTile { CocoaTile(unsigned char i) : GrowerTile(i) {}

    bool canSurvive(World* w, int x, int y, int z) { return cocoaCanSurvive(w, x, y, z, worldData(w, x, y, z)); }
    bool mayPlace(World* w, int x, int y, int z) { return canSurvive(w, x, y, z); }
    bool mayPlace(World* w, int x, int y, int z, int) { return canSurvive(w, x, y, z); }

    void grow(World* w, int x, int y, int z) {
        unsigned char data = worldData(w, x, y, z);
        int dir = data & COCOA_DIR_MASK;
        int age = (data >> COCOA_AGE_SHIFT) & COCOA_AGE_MASK;
        if (age < 2 && rand() % 5 == 0)
            worldSetDataNoUpdate(w, x, y, z, (unsigned char)(dir | ((age + 1) << COCOA_AGE_SHIFT)));
    }

    void spawnResources(World* w, int x, int y, int z, int data, Random& rng) {
        int age = (data >> COCOA_AGE_SHIFT) & COCOA_AGE_MASK;
        int count = (age >= 2) ? (2 + rng.nextInt(2)) : 1;
        for (int i = 0; i < count; i++)
            dropItem(x, y, z, ITEM_BONEMEAL, (short)3, rng); // 3 = DYE_BROWN
    } };

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

static void meltTick(World* w, int x, int y, int z, unsigned char id, int lightBlock,
                     unsigned char becomes) {

    if (lightBlockGet(w, x, y, z) <= 11 - lightBlock) return;
    worldSpawnResources(w, x, y, z, id, worldData(w, x, y, z));
    worldSetBlockAndData(w, x, y, z, becomes, 0);

    if (isLiquidId(becomes)) worldScheduleTick(w, x, y, z, becomes, 5);
    worldNotifyNeighborsChanged(w, x, y, z);
}

struct IceTile : Tile { IceTile(unsigned char i) : Tile(i) { randomTicks = true; }
    void randomTick(World* w, int x, int y, int z) {
        meltTick(w, x, y, z, id, lightBlock, BLOCK_WATER); } };

struct SnowLayerTile : SupportTile { SnowLayerTile(unsigned char i) : SupportTile(i) { randomTicks = true; }
    void randomTick(World* w, int x, int y, int z) {
        meltTick(w, x, y, z, id, 0, BLOCK_AIR); } };

struct SnowBlockTile : Tile { SnowBlockTile(unsigned char i) : Tile(i) { randomTicks = true; }
    void randomTick(World* w, int x, int y, int z) {
        meltTick(w, x, y, z, id, 0, BLOCK_AIR); } };

struct LavaTile : Tile { LavaTile(unsigned char i) : Tile(i) { randomTicks = true; }
    void randomTick(World* w, int x, int y, int z) {
        int h = rand() % 3;
        for (int i = 0; i < h; i++) {
            x += (rand() % 3) - 1;
            y++;
            z += (rand() % 3) - 1;
            unsigned char t = worldBlock(w, x, y, z);
            if (t == BLOCK_AIR) {
                if (fireCanBurn(w, x - 1, y, z) || fireCanBurn(w, x + 1, y, z) ||
                    fireCanBurn(w, x, y, z - 1) || fireCanBurn(w, x, y, z + 1) ||
                    fireCanBurn(w, x, y - 1, z) || fireCanBurn(w, x, y + 1, z)) {
                    firePlace(w, x, y, z);
                    return;
                }
            } else if (Tile::tiles[t]->solidPhys) {
                return;
            }
        }
    } };

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
    if (id == BLOCK_CAKE) return false;
    return true;
}
static bool rawOpaque(unsigned char id) {
    return id != BLOCK_AIR && !isLiquidId(id) && id != BLOCK_ICE &&
           id != BLOCK_LEAVES && id != BLOCK_GLASS && id != BLOCK_SAPLING && id != BLOCK_GLASS_PANE &&
           !isCrossShaped(id) && id != BLOCK_CACTUS && id != BLOCK_TOPSNOW && id != BLOCK_REEDS &&
           !isSlab(id) && !isStairs(id) && id != BLOCK_FENCE && id != BLOCK_LADDER && id != BLOCK_TORCH &&
           !isDoor(id) && !isTrapdoor(id) && !isFenceGate(id) && !isBed(id) && id != BLOCK_FARMLAND &&
           id != BLOCK_CHEST && !isSign(id) && id != BLOCK_FIRE && id != BLOCK_CAKE;
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
        id == BLOCK_CHEST || id == BLOCK_CAKE) return 0;
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
        case BLOCK_BAMBOO: case BLOCK_COCOA: case BLOCK_VINE:
            return SOUND_GRASS;

        case BLOCK_DIRT: case BLOCK_GRAVEL: case BLOCK_CLAY: case BLOCK_FARMLAND:
            return SOUND_GRAVEL;

        case BLOCK_SAND:
            return SOUND_SAND;

        case BLOCK_WOOL: case BLOCK_TOPSNOW: case BLOCK_SNOW_BLOCK: case BLOCK_CACTUS:
        case BLOCK_CAKE:
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

        case BLOCK_WOOD_SLAB: case BLOCK_WOOD_SLAB_DOUBLE:
            return SOUND_WOOD;

        default:
            return SOUND_STONE;
    }
}

static float rawDestroySpeed(int id) {
    switch (id) {
        case BLOCK_STONE: case BLOCK_STONE_BRICKS: case BLOCK_BOOKSHELF:
        case BLOCK_STAIRS_STONE_BRICK:
            return 1.5f;
        case BLOCK_DIRT: case BLOCK_SAND:
            return 0.5f;
        case BLOCK_GRASS: case BLOCK_GRAVEL: case BLOCK_CLAY: case BLOCK_FARMLAND:
            return 0.6f;
        case BLOCK_PLANKS: case BLOCK_LOG: case BLOCK_FENCE: case BLOCK_FENCE_GATE:
        case BLOCK_DOUBLE_SLAB: case BLOCK_SLAB: case BLOCK_COBBLESTONE:
        case BLOCK_WOOD_SLAB: case BLOCK_WOOD_SLAB_DOUBLE:
        case BLOCK_BRICKS: case BLOCK_MOSSY_COBBLE: case BLOCK_NETHER_BRICK:
        case BLOCK_STAIRS_PLANKS: case BLOCK_STAIRS_COBBLESTONE:
        case BLOCK_STAIRS_BRICK: case BLOCK_STAIRS_NETHER_BRICK:
            return 2.0f;
        case BLOCK_LEAVES:
            return 0.2f;
        case BLOCK_GLASS: case BLOCK_GLASS_PANE: case BLOCK_GLOWSTONE:
            return 0.3f;
        case BLOCK_ORE_GOLD: case BLOCK_ORE_IRON: case BLOCK_ORE_COAL:
        case BLOCK_ORE_LAPIS: case BLOCK_ORE_EMERALD:
        case BLOCK_ORE_REDSTONE: case BLOCK_ORE_REDSTONE_LIT:
        case BLOCK_LAPIS_BLOCK: case BLOCK_GOLD_BLOCK:
        case BLOCK_DOOR_WOOD: case BLOCK_TRAPDOOR: case BLOCK_NETHER_REACTOR:
            return 3.0f;
        case BLOCK_IRON_BLOCK: case BLOCK_DIAMOND_BLOCK: case BLOCK_DOOR_IRON:
            return 5.0f;
        case BLOCK_OBSIDIAN: case BLOCK_GLOWING_OBSIDIAN:
            return 10.0f;
        case BLOCK_COBWEB:
            return 4.0f;
        case BLOCK_WOOL: case BLOCK_SANDSTONE: case BLOCK_QUARTZ_BLOCK:
        case BLOCK_STAIRS_SANDSTONE: case BLOCK_STAIRS_QUARTZ:
            return 0.8f;
        case BLOCK_ICE:
            return 0.5f;
        case BLOCK_TOPSNOW:
            return 0.1f;
        case BLOCK_SNOW_BLOCK: case BLOCK_BED:
            return 0.2f;
        case BLOCK_CAKE:
            return 0.5f;
        case BLOCK_CACTUS: case BLOCK_LADDER: case BLOCK_NETHERRACK:
            return 0.4f;
        case BLOCK_CHEST: case BLOCK_CRAFTING_TABLE: case BLOCK_STONECUTTER:
            return 2.5f;
        case BLOCK_FURNACE: case BLOCK_FURNACE_LIT:
            return 3.5f;
        case BLOCK_SIGN: case BLOCK_WALL_SIGN: case BLOCK_MELON:
            return 1.0f;
        case BLOCK_BEDROCK: case BLOCK_INVISIBLE_BEDROCK:
            return -1.0f;

        case BLOCK_SAPLING: case BLOCK_TALLGRASS: case BLOCK_FLOWER: case BLOCK_ROSE:
        case BLOCK_MUSHROOM_BROWN: case BLOCK_MUSHROOM_RED: case BLOCK_TNT:
        case BLOCK_TORCH: case BLOCK_WHEAT: case BLOCK_REEDS: case BLOCK_MELON_STEM:
            return 0.0f;
        default:
            return 1.0f;
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
    if (id == BLOCK_CAKE)       return SHAPE_CAKE;
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
        case BLOCK_BAMBOO:   return new BambooTile(id);
        case BLOCK_VINE:     return new VineTile(id);
        case BLOCK_COCOA:    return new CocoaTile(id);
        case BLOCK_FLOWER: case BLOCK_ROSE: case BLOCK_SAPLING:
        case BLOCK_WHEAT: case BLOCK_MELON_STEM: case BLOCK_TALLGRASS:
        case BLOCK_MUSHROOM_BROWN: case BLOCK_MUSHROOM_RED:
            return new BushTile(id);
        case BLOCK_TOPSNOW:
            return new SnowLayerTile(id);
        case BLOCK_SNOW_BLOCK:
            return new SnowBlockTile(id);
        case BLOCK_ICE:
            return new IceTile(id);
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
        case BLOCK_LAVA: case BLOCK_CALM_LAVA:
            return new LavaTile(id);
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
        case SHAPE_CAKE:      return new CakeTile(id);
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
        t->destroySpeed  = rawDestroySpeed((unsigned char)id);
        t->material      = &materialOf((unsigned char)id);
        t->blocksLight   = t->material->blocksLight();
        tiles[id] = t;
    }
}
