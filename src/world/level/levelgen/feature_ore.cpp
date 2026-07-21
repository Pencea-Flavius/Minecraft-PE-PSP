#include "world/level/levelgen/features.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"

#define MCPE_PI 3.14159265f

void oreFeature(World* w, Random& random, int x, int y, int z, unsigned char tile, int count) {
    float dir = random.nextFloat() * MCPE_PI;

    float x0 = x + 8 + sinf(dir) * count / 8.0f;
    float x1 = x + 8 - sinf(dir) * count / 8.0f;
    float z0 = z + 8 + cosf(dir) * count / 8.0f;
    float z1 = z + 8 - cosf(dir) * count / 8.0f;

    float y0 = (float)(y + random.nextInt(3) + 2);
    float y1 = (float)(y + random.nextInt(3) + 2);

    for (int D = 0; D <= count; D++) {
        float d = (float)D;
        float xx = x0 + (x1 - x0) * d / count;
        float yy = y0 + (y1 - y0) * d / count;
        float zz = z0 + (z1 - z0) * d / count;

        float ss = random.nextFloat() * count / 16.0f;
        float r = (sinf(d * MCPE_PI / count) + 1.0f) * ss + 1.0f;
        float hr = (sinf(d * MCPE_PI / count) + 1.0f) * ss + 1.0f;

        int xt0 = (int) (xx - r / 2.0f);
        int yt0 = (int) (yy - hr / 2.0f);
        int zt0 = (int) (zz - r / 2.0f);

        int xt1 = (int) (xx + r / 2.0f);
        int yt1 = (int) (yy + hr / 2.0f);
        int zt1 = (int) (zz + r / 2.0f);

        for (int x2 = xt0; x2 <= xt1; x2++) {
            float xd = ((x2 + 0.5f) - xx) / (r / 2.0f);
            if (xd * xd < 1.0f) {
                for (int y2 = yt0; y2 <= yt1; y2++) {
                    float yd = ((y2 + 0.5f) - yy) / (hr / 2.0f);
                    if (xd * xd + yd * yd < 1.0f) {
                        for (int z2 = zt0; z2 <= zt1; z2++) {
                            float zd = ((z2 + 0.5f) - zz) / (r / 2.0f);
                            if (xd * xd + yd * yd + zd * zd < 1.0f) {
                                if (worldBlock(w, x2, y2, z2) == BLOCK_STONE) {
                                    setBlock(w, x2, y2, z2, tile);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
