#include "world/level/levelgen/features.h"
#include "world/level/levelgen/mcpegen.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include <vector>

struct PendingFlower { int x, y, z; unsigned char tile; };
static std::vector<PendingFlower> g_pendingFlowers;

void flowerFeature(World* w, Random& random, int x, int y, int z, unsigned char tile) {
    for (int i = 0; i < 64; i++) {
        int x2 = x + random.nextInt(8) - random.nextInt(8);
        int y2 = y + random.nextInt(4) - random.nextInt(4);
        int z2 = z + random.nextInt(8) - random.nextInt(8);
        if (worldBlock(w, x2, y2, z2) == BLOCK_AIR) {
            unsigned char below = worldBlock(w, x2, y2 - 1, z2);
            if (below == BLOCK_GRASS || below == BLOCK_DIRT) {
                PendingFlower pf = { x2, y2, z2, tile };
                g_pendingFlowers.push_back(pf);
            }
        }
    }
}

void worldPlaceFlowers(World* w) {
    for (size_t i = 0; i < g_pendingFlowers.size(); i++) {
        const PendingFlower& f = g_pendingFlowers[i];

        unsigned char here = worldBlock(w, f.x, f.y, f.z);
        if (here != BLOCK_AIR && here != BLOCK_TOPSNOW) continue;
        unsigned char below = worldBlock(w, f.x, f.y - 1, f.z);
        if (below != BLOCK_GRASS && below != BLOCK_DIRT) continue;
        if (lightRawAt(w, f.x, f.y, f.z) >= 8 || worldCanSeeSky(w, f.x, f.y, f.z))
            setBlock(w, f.x, f.y, f.z, f.tile);
    }
    g_pendingFlowers.clear();
}
