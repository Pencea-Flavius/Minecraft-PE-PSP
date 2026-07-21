#include "world/level/levelgen/features.h"
#include "world/level/levelgen/mcpegen.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include <vector>

struct PendingMushroom { int x, y, z; unsigned char tile; };
static std::vector<PendingMushroom> g_pendingMushrooms;

void mushroomFeature(World* w, Random& random, int x, int y, int z, unsigned char tile) {
    for (int i = 0; i < 64; i++) {
        int x2 = x + random.nextInt(8) - random.nextInt(8);
        int y2 = y + random.nextInt(4) - random.nextInt(4);
        int z2 = z + random.nextInt(8) - random.nextInt(8);
        if (worldBlock(w, x2, y2, z2) == BLOCK_AIR) {
            unsigned char below = worldBlock(w, x2, y2 - 1, z2);
            if (isSolidGen(below)) {
                PendingMushroom pm = { x2, y2, z2, tile };
                g_pendingMushrooms.push_back(pm);
            }
        }
    }
}

void worldPlaceMushrooms(World* w) {
    for (size_t i = 0; i < g_pendingMushrooms.size(); i++) {
        const PendingMushroom& m = g_pendingMushrooms[i];
        if (worldBlock(w, m.x, m.y, m.z) != BLOCK_AIR) continue;
        if (!isSolidGen(worldBlock(w, m.x, m.y - 1, m.z))) continue;
        if (lightRawAt(w, m.x, m.y, m.z) < 13)
            setBlock(w, m.x, m.y, m.z, m.tile);
    }
    g_pendingMushrooms.clear();
}
