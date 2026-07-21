#include "world/level/levelgen/features.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"

#define MCPE_PI 3.14159265f

void clayFeature(World* w, Random& random, int x, int y, int z) {
    const int count = 32;
    if (!isWaterId(worldBlock(w, x, y, z))) return;

    float dir = random.nextFloat() * MCPE_PI;
    float x0 = x + 8 + sinf(dir) * (count / 8);
    float x1 = x + 8 - sinf(dir) * (count / 8);
    float z0 = z + 8 + cosf(dir) * (count / 8);
    float z1 = z + 8 - cosf(dir) * (count / 8);
    float y0 = (float)(y + random.nextInt(3) + 2);
    float y1 = (float)(y + random.nextInt(3) + 2);

    for (int d = 0; d <= count; d++) {
        float xx = x0 + (x1 - x0) * d / count;
        float yy = y0 + (y1 - y0) * d / count;
        float zz = z0 + (z1 - z0) * d / count;
        float ss = random.nextFloat() * (float)(count >> 4);
        float r  = (sinf(d * MCPE_PI / count) + 1) * ss + 1;

        for (int x2 = (int)(xx - r / 2); x2 <= (int)(xx + r / 2); x2++)
        for (int y2 = (int)(yy - r / 2); y2 <= (int)(yy + r / 2); y2++)
        for (int z2 = (int)(zz - r / 2); z2 <= (int)(zz + r / 2); z2++) {
            float xd = ((x2 + 0.5f) - xx) / (r / 2);
            float yd = ((y2 + 0.5f) - yy) / (r / 2);
            float zd = ((z2 + 0.5f) - zz) / (r / 2);
            if (xd * xd + yd * yd + zd * zd < 1 && worldBlock(w, x2, y2, z2) == BLOCK_SAND)
                setBlock(w, x2, y2, z2, BLOCK_CLAY);
        }
    }
}
