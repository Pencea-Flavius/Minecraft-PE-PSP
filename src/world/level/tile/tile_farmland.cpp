
#include "world/level/tile/tile_behavior.h"

void farmlandCheckDry(World* w, int x, int y, int z) {
    if (worldBlock(w, x, y, z) != BLOCK_FARMLAND) return;
    if (!isOpaque(worldBlock(w, x, y + 1, z))) return;
    worldSetBlockAndData(w, x, y, z, BLOCK_DIRT, 0);
}

static bool farmlandHasNearbyWater(World* w, int x, int y, int z) {
    for (int dx = -4; dx <= 4; dx++)
    for (int dz = -4; dz <= 4; dz++)
    for (int dy = 0; dy <= 1; dy++)
        if (isWaterId(worldBlock(w, x + dx, y + dy, z + dz))) return true;
    return false;
}
void tickFarmland(World* w, int x, int y, int z) {
    unsigned char moisture = worldData(w, x, y, z);
    if (farmlandHasNearbyWater(w, x, y, z)) {
        if (moisture < 7) worldSetData(w, x, y, z, 7);
    } else if (moisture > 0) {
        worldSetData(w, x, y, z, moisture - 1);
    } else {
        worldSetBlockAndData(w, x, y, z, BLOCK_DIRT, 0);
        worldNotifyNeighborsChanged(w, x, y, z);
    }
}
