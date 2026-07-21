#include "world/level/levelgen/features.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"

static inline bool isTreeClear(unsigned char b) {
    return b == BLOCK_AIR || isLeaf(b);
}

void treeBirch(World* w, Random& random, int x, int y, int z) {
    int treeHeight = random.nextInt(3) + 5;
    if (y < 1 || y + treeHeight + 1 > WORLD_H) return;
    for (int yy = y; yy <= y + 1 + treeHeight; yy++) {
        int r = (yy == y) ? 0 : 1;
        if (yy >= y + 1 + treeHeight - 2) r = 2;
        for (int xx = x - r; xx <= x + r; xx++)
        for (int zz = z - r; zz <= z + r; zz++)
            if (!isTreeClear(worldBlock(w, xx, yy, zz))) return;
    }
    unsigned char below = worldBlock(w, x, y - 1, z);
    if (below != BLOCK_GRASS && below != BLOCK_DIRT) return;
    setBlock(w, x, y - 1, z, BLOCK_DIRT);
    for (int yy = y - 3 + treeHeight; yy <= y + treeHeight; yy++) {
        int yo = yy - (y + treeHeight);
        int offs = 1 - yo / 2;
        for (int xx = x - offs; xx <= x + offs; xx++) {
            int axo = (xx - x < 0) ? -(xx - x) : (xx - x);
            for (int zz = z - offs; zz <= z + offs; zz++) {
                int azo = (zz - z < 0) ? -(zz - z) : (zz - z);
                if (axo == offs && azo == offs && (random.nextInt(2) == 0 || yo == 0)) continue;
                if (!isSolidGen(worldBlock(w, xx, yy, zz))) setBlock(w, xx, yy, zz, BLOCK_LEAVES, LEAF_BIRCH);
            }
        }
    }
    for (int hh = 0; hh < treeHeight; hh++) {
        unsigned char t = worldBlock(w, x, y + hh, z);
        if (!isSolidGen(t)) setBlock(w, x, y + hh, z, BLOCK_LOG, LOG_BIRCH);
    }
}
