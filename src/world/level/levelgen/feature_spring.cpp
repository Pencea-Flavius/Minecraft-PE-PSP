#include "world/level/levelgen/features.h"
#include "world/level/world.h"

void springFeature(World* w, int x, int y, int z, unsigned char tile) {
    if (worldBlock(w, x, y + 1, z) != BLOCK_STONE) return;
    if (worldBlock(w, x, y - 1, z) != BLOCK_STONE) return;

    unsigned char current = worldBlock(w, x, y, z);
    if (current != BLOCK_AIR && current != BLOCK_STONE) return;

    int rockCount = 0;
    if (worldBlock(w, x - 1, y, z) == BLOCK_STONE) rockCount++;
    if (worldBlock(w, x + 1, y, z) == BLOCK_STONE) rockCount++;
    if (worldBlock(w, x, y, z - 1) == BLOCK_STONE) rockCount++;
    if (worldBlock(w, x, y, z + 1) == BLOCK_STONE) rockCount++;

    int holeCount = 0;
    if (worldBlock(w, x - 1, y, z) == BLOCK_AIR) holeCount++;
    if (worldBlock(w, x + 1, y, z) == BLOCK_AIR) holeCount++;
    if (worldBlock(w, x, y, z - 1) == BLOCK_AIR) holeCount++;
    if (worldBlock(w, x, y, z + 1) == BLOCK_AIR) holeCount++;

    if (rockCount == 3 && holeCount == 1) {
        setBlock(w, x, y, z, tile);
    }
}
