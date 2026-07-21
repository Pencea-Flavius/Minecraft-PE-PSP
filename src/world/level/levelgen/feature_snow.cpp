#include "world/level/levelgen/features.h"
#include "world/level/world.h"

void snowCap(World* w, int chunkX, int chunkZ, float* mTemp) {
    const float SNOW_CUTOFF = 0.5f, SNOW_SCALE = 0.3f;
    for (int lx = 0; lx < 16; lx++)
    for (int lz = 0; lz < 16; lz++) {
        int gx = chunkX * 16 + lx, gz = chunkZ * 16 + lz;
        int y = heightmapAt(w, gx, gz);
        if (y <= 0 || y >= WORLD_H) continue;
        float temp = mTemp[lx * 16 + lz] - (y - 64) / 64.0f * SNOW_SCALE;
        if (temp >= SNOW_CUTOFF) continue;
        if (worldBlock(w, gx, y, gz) != BLOCK_AIR) continue;
        unsigned char below = worldBlock(w, gx, y - 1, gz);
        if (below == BLOCK_AIR || isWaterId(below) || below == BLOCK_ICE) continue;
        setBlock(w, gx, y, gz, BLOCK_TOPSNOW);
    }
}
