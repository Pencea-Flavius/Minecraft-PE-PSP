
#include "world/level/tile/tile_behavior.h"

bool reedCanSurvive(World* w, int x, int y, int z) {
    unsigned char below = worldBlock(w, x, y - 1, z);
    if (below == BLOCK_REEDS) return true;
    if (below != BLOCK_GRASS && below != BLOCK_DIRT && below != BLOCK_SAND) return false;
    return isWaterId(worldBlock(w, x - 1, y - 1, z)) || isWaterId(worldBlock(w, x + 1, y - 1, z)) ||
           isWaterId(worldBlock(w, x, y - 1, z - 1)) || isWaterId(worldBlock(w, x, y - 1, z + 1));
}

bool cactusCanSurvive(World* w, int x, int y, int z) {
    if (isSolidPhys(worldBlock(w, x - 1, y, z)) || isSolidPhys(worldBlock(w, x + 1, y, z)) ||
        isSolidPhys(worldBlock(w, x, y, z - 1)) || isSolidPhys(worldBlock(w, x, y, z + 1))) return false;
    unsigned char below = worldBlock(w, x, y - 1, z);
    return below == BLOCK_CACTUS || below == BLOCK_SAND;
}

void reedCactusGrow(World* w, int x, int y, int z, unsigned char id, int ageThreshold) {
    if (worldBlock(w, x, y + 1, z) != BLOCK_AIR) return;
    int height = 1;
    while (worldBlock(w, x, y - height, z) == id) height++;
    if (height >= 3) return;
    int age = worldData(w, x, y, z);
    if (age >= ageThreshold) {
        worldSetBlockAndData(w, x, y + 1, z, id, 0);
        worldSetDataNoUpdate(w, x, y, z, 0);
    } else {
        worldSetDataNoUpdate(w, x, y, z, (unsigned char)(age + 1));
    }
}
