#include "world/level/levelgen/features.h"
#include "world/level/world.h"

void setBlock(World* w, int x, int y, int z, unsigned char id, unsigned char data) {
    if (x < 0 || x >= WORLD_W || y < 0 || y >= WORLD_H || z < 0 || z >= WORLD_D) return;
    worldSetBlockAndData(w, x, y, z, id, data);
    if (id == BLOCK_WATER || id == BLOCK_LAVA) {
        worldScheduleTick(w, x, y, z, id, (id == BLOCK_WATER) ? 5 : 30);
    }
}

bool isSolidGen(unsigned char id) {
    return id != BLOCK_AIR && !isWaterId(id) && !isLeaf(id) &&
           id != BLOCK_FLOWER && id != BLOCK_ROSE &&
           id != BLOCK_MUSHROOM_BROWN && id != BLOCK_MUSHROOM_RED;
}

int heightmapAt(World* w, int x, int z) {
    for (int y = WORLD_H - 1; y >= 0; y--) {
        unsigned char b = worldBlock(w, x, y, z);
        if (b == BLOCK_GRASS || b == BLOCK_DIRT || b == BLOCK_SAND ||
            b == BLOCK_STONE || b == BLOCK_GRAVEL || b == BLOCK_SANDSTONE ||
            b == BLOCK_CLAY || b == BLOCK_BEDROCK)
            return y + 1;
    }
    return 0;
}
