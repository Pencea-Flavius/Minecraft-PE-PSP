
#ifndef MCPSP_WORLD_LEVEL_TILE_MATERIAL_H
#define MCPSP_WORLD_LEVEL_TILE_MATERIAL_H

#include "world/level/chunk/chunk.h"

enum TileMaterial {
    MAT_AIR,
    MAT_DIRT,
    MAT_WOOD,
    MAT_STONE,
    MAT_METAL,
    MAT_WATER,
    MAT_LAVA,
    MAT_LEAVES,
    MAT_PLANT,
    MAT_REPLACEABLE_PLANT,
    MAT_CLOTH,
    MAT_SAND,
    MAT_DECORATION,
    MAT_GLASS,
    MAT_EXPLOSIVE,
    MAT_ICE,
    MAT_TOPSNOW,
    MAT_SNOW,
    MAT_CACTUS,
    MAT_CLAY,
    MAT_VEGETABLE,
    MAT_WEB,
};

inline TileMaterial tileMaterial(unsigned char id) {
    switch (id) {
        case BLOCK_DIRT: case BLOCK_FARMLAND: case BLOCK_UPDATE1: case BLOCK_UPDATE2:
            return MAT_DIRT;
        case BLOCK_PLANKS: case BLOCK_LOG: case BLOCK_BOOKSHELF: case BLOCK_STAIRS_PLANKS:
        case BLOCK_CHEST: case BLOCK_CRAFTING_TABLE: case BLOCK_SIGN: case BLOCK_DOOR_WOOD:
        case BLOCK_WALL_SIGN: case BLOCK_FENCE: case BLOCK_TRAPDOOR: case BLOCK_FENCE_GATE:
            return MAT_WOOD;
        case BLOCK_GOLD_BLOCK: case BLOCK_IRON_BLOCK: case BLOCK_DIAMOND_BLOCK: case BLOCK_DOOR_IRON:
        case BLOCK_NETHER_REACTOR:
            return MAT_METAL;
        case BLOCK_WATER: case BLOCK_CALM_WATER:
            return MAT_WATER;
        case BLOCK_LAVA: case BLOCK_CALM_LAVA:
            return MAT_LAVA;
        case BLOCK_SAPLING: case BLOCK_FLOWER: case BLOCK_ROSE: case BLOCK_MUSHROOM_BROWN:
        case BLOCK_MUSHROOM_RED: case BLOCK_WHEAT: case BLOCK_REEDS: case BLOCK_MELON_STEM:
            return MAT_PLANT;
        case BLOCK_TALLGRASS:
            return MAT_REPLACEABLE_PLANT;
        case BLOCK_BED: case BLOCK_WOOL:
            return MAT_CLOTH;
        case BLOCK_SAND: case BLOCK_GRAVEL:
            return MAT_SAND;
        case BLOCK_TORCH: case BLOCK_LADDER:
            return MAT_DECORATION;
        case BLOCK_GLASS: case BLOCK_GLOWSTONE: case BLOCK_GLASS_PANE:
            return MAT_GLASS;
        case BLOCK_TNT:
            return MAT_EXPLOSIVE;
        case BLOCK_ICE:
            return MAT_ICE;
        case BLOCK_TOPSNOW:
            return MAT_TOPSNOW;
        case BLOCK_SNOW_BLOCK:
            return MAT_SNOW;
        case BLOCK_CACTUS:
            return MAT_CACTUS;
        case BLOCK_CLAY:
            return MAT_CLAY;
        case BLOCK_MELON:
            return MAT_VEGETABLE;
        case BLOCK_COBWEB:
            return MAT_WEB;

        default: return MAT_STONE;
    }
}

#endif
