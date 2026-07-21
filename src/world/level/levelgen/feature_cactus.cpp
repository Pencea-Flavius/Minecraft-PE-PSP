#include "world/level/levelgen/features.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"

static bool cactusCanSurvive(World* w, int x, int y, int z) {
    if (isSolidGen(worldBlock(w, x - 1, y, z))) return false;
    if (isSolidGen(worldBlock(w, x + 1, y, z))) return false;
    if (isSolidGen(worldBlock(w, x, y, z - 1))) return false;
    if (isSolidGen(worldBlock(w, x, y, z + 1))) return false;
    unsigned char below = worldBlock(w, x, y - 1, z);
    return below == BLOCK_SAND || below == BLOCK_CACTUS;
}

void cactusFeature(World* w, Random& random, int x, int y, int z) {
    for (int i = 0; i < 10; i++) {
        int x2 = x + random.nextInt(8) - random.nextInt(8);
        int y2 = y + random.nextInt(4) - random.nextInt(4);
        int z2 = z + random.nextInt(8) - random.nextInt(8);
        if (worldBlock(w, x2, y2, z2) == BLOCK_AIR) {
            int h = 1 + random.nextInt(random.nextInt(3) + 1);
            for (int yy = 0; yy < h; yy++)
                if (cactusCanSurvive(w, x2, y2 + yy, z2))
                    setBlock(w, x2, y2 + yy, z2, BLOCK_CACTUS);
        }
    }
}
