#include "world/level/levelgen/features.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"
#include <math.h>

// Jungle trees: tall bare trunk with a wide, rounded canopy that bulges out
// in the middle and tapers at top and bottom, mimicking the broad, spreading
// crown of a real jungle tree. Cocoa pods sprout on free trunk faces, and
// vines hang thickly from the canopy and trunk once the tree is placed.

static int jungleRadiusAt(int layerFromBottom, int treeHeight, int maxRadius) {
    // Only the top ~40% of the tree (the canopy band) needs clearance wider
    // than the bare trunk; layerFromBottom counts up from the base.
    int canopyStart = treeHeight - (treeHeight * 2) / 5;
    if (layerFromBottom < canopyStart) return 1;
    return maxRadius;
}

void treeJungle(World* w, Random& random, int x, int y, int z) {
    int treeHeight = random.nextInt(5) + 8;       // 8-12 tall
    int canopyHeight = 5 + random.nextInt(2);      // canopy band spans top 5-6 blocks
    int maxCanopyRadius = 3 + random.nextInt(2);   // widest point of the spread, 3-4 out

    if (!treeSpaceClear(w, x, y, z, treeHeight, jungleRadiusAt, maxCanopyRadius)) return;

    unsigned char below = worldBlock(w, x, y - 1, z);
    if (below != BLOCK_GRASS && below != BLOCK_DIRT) return;
    setBlock(w, x, y - 1, z, BLOCK_DIRT);

    // Canopy: radius bulges widest at the vertical center of the canopy band
    // and tapers toward both the top and bottom, giving a rounded spread
    // instead of a flat-topped column. Each layer is circularly masked (not
    // square) so it reads as rounded from above, with a randomized soft edge.
    float center = (canopyHeight - 1) / 2.0f;
    for (int li = 0; li < canopyHeight; li++) {
        int yy = y + treeHeight - li;
        float distFromCenter = (li < center) ? (center - li) : (li - center);
        int r = (int)(maxCanopyRadius - distFromCenter + 0.5f);
        if (r < 2) r = 2;
        if (r > maxCanopyRadius) r = maxCanopyRadius;

        for (int xx = x - r; xx <= x + r; xx++) {
            int axo = xx - x;
            for (int zz = z - r; zz <= z + r; zz++) {
                int azo = zz - z;
                float dist = sqrtf((float)(axo * axo + azo * azo));
                if (dist > r + 0.5f) continue;
                if (dist > r - 0.5f && random.nextInt(3) == 0) continue;
                if (!isSolidGen(worldBlock(w, xx, yy, zz))) setBlock(w, xx, yy, zz, BLOCK_LEAVES, LEAF_JUNGLE);
            }
        }
    }

    // Trunk: solid straight bare log up to the canopy. Occasionally sprout a
    // cocoa pod on a free side face partway up the trunk.
    static const int dx[4] = {  0,  1, 0, -1 };
    static const int dz[4] = { -1,  0, 1,  0 };
    for (int hh = 0; hh < treeHeight; hh++) {
        unsigned char t = worldBlock(w, x, y + hh, z);
        if (!isSolidGen(t)) setBlock(w, x, y + hh, z, BLOCK_LOG, LOG_JUNGLE);

        if (hh >= 1 && hh < treeHeight - 1 && random.nextInt(6) == 0) {
            int dir = random.nextInt(4);
            int px = x + dx[dir], pz = z + dz[dir];
            if (worldBlock(w, px, y + hh, pz) == BLOCK_AIR) {
                int age = random.nextInt(3);
                setBlock(w, px, y + hh, pz, BLOCK_COCOA, (unsigned char)(dir | (age << COCOA_AGE_SHIFT)));
            }
        }
    }

    // Vines: thickly hung from the canopy and upper trunk. Several starting
    // points around the tree's footprint each drop a chain of vine blocks,
    // with a high per-block continuation chance so chains commonly run long.
    int vineStarts = 10 + random.nextInt(6);
    for (int i = 0; i < vineStarts; i++) {
        int vx = x + random.nextInt(maxCanopyRadius * 2 + 1) - maxCanopyRadius;
        int vz = z + random.nextInt(maxCanopyRadius * 2 + 1) - maxCanopyRadius;
        int vy = y + treeHeight - random.nextInt(canopyHeight + 2);

        unsigned char above = worldBlock(w, vx, vy + 1, vz);
        if (above != BLOCK_LEAVES && above != BLOCK_LOG) continue;

        int chainLen = 3 + random.nextInt(6);
        for (int c = 0; c < chainLen; c++) {
            int cy = vy - c;
            if (worldBlock(w, vx, cy, vz) != BLOCK_AIR) break;
            setBlock(w, vx, cy, vz, BLOCK_VINE, 0);
            if (random.nextInt(5) == 0) break; // occasionally end a chain early
        }
    }
}
