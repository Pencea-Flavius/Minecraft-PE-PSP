#include "world/item/bonemeal_item.h"
#include "world/level/world.h"
#include "world/level/tile/tile_behavior.h"
#include <stdlib.h>

static void bonemealGrass(World* w, int x, int y, int z) {
    for (int j = 0; j < 32; j++) {
        int xx = x, yy = y + 1, zz = z;
        bool blocked = false;
        for (int i = 0; i < j / 16; i++) {
            xx += rand() % 3 - 1;
            yy += (rand() % 3 - 1) * (rand() % 3) / 2;
            zz += rand() % 3 - 1;
            if (worldBlock(w, xx, yy - 1, zz) != BLOCK_GRASS || isOpaque(worldBlock(w, xx, yy, zz))) {
                blocked = true;
                break;
            }
        }
        if (blocked) continue;
        if (worldBlock(w, xx, yy, zz) == BLOCK_AIR)
            worldSetBlockAndData(w, xx, yy, zz, (rand() % 3 != 0) ? BLOCK_FLOWER : BLOCK_ROSE, 0);
    }
}

static bool bonemealReed(World* w, int x, int y, int z) {
    while (worldBlock(w, x, y + 1, z) == BLOCK_REEDS) y++;
    int height = 1;
    while (worldBlock(w, x, y - height, z) == BLOCK_REEDS) height++;
    if (height >= 3 || worldBlock(w, x, y + 1, z) != BLOCK_AIR) return false;
    worldSetBlockAndData(w, x, y + 1, z, BLOCK_REEDS, 0);
    return true;
}

bool BonemealItem::useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face) {

    if (item->data != DYE_WHITE) return false;
    switch (worldBlock(world, x, y, z)) {
        case BLOCK_SAPLING:
            saplingGrow(world, x, y, z);
            worldUpdateLights(world);
            worldRebuildAroundNow(world, x, y, z);
            return true;
        case BLOCK_WHEAT:
        case BLOCK_MELON_STEM:
            worldSetData(world, x, y, z, 7);
            return true;
        case BLOCK_GRASS:
            bonemealGrass(world, x, y, z);
            return true;
        case BLOCK_REEDS:
            return bonemealReed(world, x, y, z);
    }
    return false;
}
