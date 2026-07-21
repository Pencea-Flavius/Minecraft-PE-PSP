#include "world/level/levelgen/caves.h"
#include "world/level/levelgen/features.h"
#include "world/level/levelgen/Random.h"
#include "world/level/world.h"

#include <math.h>

#define MCPE_PI 3.14159265f

struct HeapRandom {
    Random* r;
    explicit HeapRandom(long seed) : r(new Random(seed)) {}
    ~HeapRandom() { delete r; }
};

static void caveAddTunnel(World* w, Random& parentRandom, int xOffs, int zOffs,
                           float xCave, float yCave, float zCave,
                           float thickness, float yRot, float xRot,
                           int step, int dist, float yScale) {
    float xMid = (float)(xOffs * 16 + 8);
    float zMid = (float)(zOffs * 16 + 8);

    float yRota = 0, xRota = 0;
    HeapRandom hr(parentRandom.nextLong());
    Random& random = *hr.r;

    if (dist <= 0) {
        int maxDist = 8 * 16 - 16;
        dist = maxDist - random.nextInt(maxDist / 4);
    }
    bool singleStep = false;
    if (step == -1) { step = dist / 2; singleStep = true; }

    int splitPoint = random.nextInt(dist / 2) + dist / 4;
    bool steep = random.nextInt(6) == 0;

    for (; step < dist; step++) {
        float rad = 1.5f + sinf(step * MCPE_PI / dist) * thickness;
        float yRad = rad * yScale;

        float xc = cosf(xRot), xs = sinf(xRot);
        xCave += cosf(yRot) * xc;
        yCave += xs;
        zCave += sinf(yRot) * xc;

        xRot *= steep ? 0.92f : 0.7f;
        xRot += xRota * 0.1f;
        yRot += yRota * 0.1f;
        xRota *= 0.90f; yRota *= 0.75f;
        xRota += (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 2;
        yRota += (random.nextFloat() - random.nextFloat()) * random.nextFloat() * 4;

        if (!singleStep && step == splitPoint && thickness > 1) {
            caveAddTunnel(w, random, xOffs, zOffs, xCave, yCave, zCave,
                          random.nextFloat() * 0.5f + 0.5f, yRot - MCPE_PI / 2, xRot / 3, step, dist, 1.0f);
            caveAddTunnel(w, random, xOffs, zOffs, xCave, yCave, zCave,
                          random.nextFloat() * 0.5f + 0.5f, yRot + MCPE_PI / 2, xRot / 3, step, dist, 1.0f);
            return;
        }
        if (!singleStep && random.nextInt(4) == 0) continue;

        {
            float xd = xCave - xMid, zd = zCave - zMid;
            float remaining = (float)(dist - step);
            float rr = (thickness + 2) + 16;
            if (xd * xd + zd * zd - (remaining * remaining) > rr * rr) return;
        }

        if (xCave < xMid - 16 - rad * 2 || zCave < zMid - 16 - rad * 2 ||
            xCave > xMid + 16 + rad * 2 || zCave > zMid + 16 + rad * 2) continue;

        int x0 = (int)floorf(xCave - rad) - xOffs * 16 - 1;
        int x1 = (int)floorf(xCave + rad) - xOffs * 16 + 1;
        int y0 = (int)floorf(yCave - yRad) - 1;
        int y1 = (int)floorf(yCave + yRad) + 1;
        int z0 = (int)floorf(zCave - rad) - zOffs * 16 - 1;
        int z1 = (int)floorf(zCave + rad) - zOffs * 16 + 1;

        if (x0 < 0) x0 = 0;
        if (x1 > 16) x1 = 16;
        if (y0 < 1) y0 = 1;
        if (y1 > 120) y1 = 120;
        if (z0 < 0) z0 = 0;
        if (z1 > 16) z1 = 16;

        bool detectedWater = false;
        for (int xx = x0; !detectedWater && xx < x1; xx++) {
            for (int zz = z0; !detectedWater && zz < z1; zz++) {
                for (int yy = y1 + 1; !detectedWater && yy >= y0 - 1; yy--) {
                    if (yy >= 0 && yy < WORLD_H &&
                        isWaterId(worldBlock(w, xOffs * 16 + xx, yy, zOffs * 16 + zz))) {
                        detectedWater = true;
                    }
                    if (yy != y0 - 1 && xx != x0 && xx != x1 - 1 && zz != z0 && zz != z1 - 1) yy = y0;
                }
            }
        }
        if (detectedWater) continue;

        for (int xx = x0; xx < x1; xx++) {
            float xd = ((xx + xOffs * 16 + 0.5f) - xCave) / rad;
            for (int zz = z0; zz < z1; zz++) {
                float zd = ((zz + zOffs * 16 + 0.5f) - zCave) / rad;
                if (xd * xd + zd * zd >= 1) continue;
                bool hasGrass = false;
                int gx = xOffs * 16 + xx, gz = zOffs * 16 + zz;
                for (int yy = y1 - 1; yy >= y0; yy--) {
                    float yd = (yy + 0.5f - yCave) / yRad;
                    if (yd > -0.7f && xd * xd + yd * yd + zd * zd < 1) {
                        unsigned char block = worldBlock(w, gx, yy, gz);
                        if (block == BLOCK_GRASS) hasGrass = true;
                        if (block == BLOCK_STONE || block == BLOCK_DIRT || block == BLOCK_GRASS) {

                            #define CAVE_LAVA_LEVEL 10
                            if (yy < CAVE_LAVA_LEVEL) {
                                setBlock(w, gx, yy, gz, BLOCK_LAVA);
                            } else {
                                setBlock(w, gx, yy, gz, BLOCK_AIR);
                                if (hasGrass && worldBlock(w, gx, yy - 1, gz) == BLOCK_DIRT)
                                    setBlock(w, gx, yy - 1, gz, BLOCK_GRASS);
                            }
                        }
                    }
                }
            }
        }
        if (singleStep) break;
    }
}

static void caveAddRoom(World* w, Random& random, int xOffs, int zOffs,
                         float xRoom, float yRoom, float zRoom) {
    caveAddTunnel(w, random, xOffs, zOffs, xRoom, yRoom, zRoom,
                  1 + random.nextFloat() * 6, 0, 0, -1, -1, 0.5f);
}

#define CAVE_RARITY 15

static void caveAddFeature(World* w, Random& random, int x, int z, int xOffs, int zOffs) {
    int caves = random.nextInt(random.nextInt(random.nextInt(40) + 1) + 1);
    if (random.nextInt(CAVE_RARITY) != 0) caves = 0;

    for (int cave = 0; cave < caves; cave++) {
        float xCave = (float)(x * 16 + random.nextInt(16));
        float yCave = (float)(random.nextInt(random.nextInt(120) + 8));
        float zCave = (float)(z * 16 + random.nextInt(16));

        int tunnels = 1;
        if (random.nextInt(4) == 0) {
            caveAddRoom(w, random, xOffs, zOffs, xCave, yCave, zCave);
            tunnels += random.nextInt(4);
        }
        for (int i = 0; i < tunnels; i++) {
            float yRot = random.nextFloat() * MCPE_PI * 2;
            float xRot = ((random.nextFloat() - 0.5f) * 2) / 8;
            float thickness = random.nextFloat() * 2 + random.nextFloat();
            caveAddTunnel(w, random, xOffs, zOffs, xCave, yCave, zCave, thickness, yRot, xRot, 0, 0, 1.0f);
        }
    }
}

void caveFeature(World* w, long worldSeed, int chunkX, int chunkZ) {
    HeapRandom hr(worldSeed);
    Random& random = *hr.r;
    long xScale = random.nextLong() / 2 * 2 + 1;
    long zScale = random.nextLong() / 2 * 2 + 1;

    const int r = 8;
    for (int x = chunkX - r; x <= chunkX + r; x++) {
        if (x < 0 || x >= WORLD_CHUNKS_X) continue;
        for (int z = chunkZ - r; z <= chunkZ + r; z++) {
            if (z < 0 || z >= WORLD_CHUNKS_Z) continue;
            random.setSeed((x * xScale + z * zScale) ^ worldSeed);
            caveAddFeature(w, random, x, z, chunkX, chunkZ);
        }
    }
}
