#include "world/level/levelgen/features.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"

void reedsFeature(World* w, Random& random, int x, int y, int z) {
    for (int i = 0; i < 20; i++) {
        int x2 = x + random.nextInt(4) - random.nextInt(4);
        int y2 = y;
        int z2 = z + random.nextInt(4) - random.nextInt(4);
        if (worldBlock(w, x2, y2, z2) == BLOCK_AIR) {
            if (isWaterId(worldBlock(w, x2-1, y2-1, z2)) ||
                isWaterId(worldBlock(w, x2+1, y2-1, z2)) ||
                isWaterId(worldBlock(w, x2, y2-1, z2-1)) ||
                isWaterId(worldBlock(w, x2, y2-1, z2+1))) {

                int h = 2 + random.nextInt(random.nextInt(3) + 1);
                for (int yy = 0; yy < h; yy++) {
                    unsigned char below = worldBlock(w, x2, y2 + yy - 1, z2);
                    if (below == BLOCK_REEDS || below == BLOCK_GRASS || below == BLOCK_DIRT || below == BLOCK_SAND) {
                        setBlock(w, x2, y2 + yy, z2, BLOCK_REEDS);
                    } else {
                        break;
                    }
                }
            }
        }
    }
}
