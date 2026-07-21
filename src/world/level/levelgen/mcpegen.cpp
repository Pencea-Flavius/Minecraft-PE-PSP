#include "world/level/levelgen/mcpegen.h"
#include "world/level/levelgen/features.h"
#include "world/level/levelgen/caves.h"
#include "world/level/levelgen/PerlinNoise.h"
#include "world/level/world.h"

#include <stdlib.h>
#include <pspkernel.h>
#include <pspthreadman.h>

#include "world/level/levelgen/mcpegen_internal.h"
#include "world/level/levelgen/biome.h"

#define MCPE_DEPTH    128
#define NCELL_W       4
#define NCELL_H       8

McpeGen::McpeGen(long seed)
  : random(seed),

    rndTemp((long)((unsigned int)seed * 9871u)),
    rndDownfall((long)((unsigned int)seed * 39811u)),
    rndNoise((long)((unsigned int)seed * 543321u)),
    lperlinNoise1(&random, 16), lperlinNoise2(&random, 16), perlinNoise1(&random, 8),
    perlinNoise2(&random, 4), perlinNoise3(&random, 4),
    scaleNoise(&random, 10), depthNoise(&random, 16), forestNoise(&random, 8),
    temperatureMap(&rndTemp, 4), downfallMap(&rndDownfall, 4), noiseMap(&rndNoise, 2),
    buffer(0), pnr(0), ar(0), br(0), sr(0), dr(0),
    rawTemp(0), rawDownfall(0), rawNoise(0), worldSeed(seed)
{
    buffer = new float[1024];
}
McpeGen::~McpeGen() {
    delete[] buffer; delete[] pnr; delete[] ar; delete[] br; delete[] sr; delete[] dr;
    delete[] rawTemp; delete[] rawDownfall; delete[] rawNoise;
}

float* McpeGen::getHeights(int x, int y, int z, int xSize, int ySize, int zSize) {
    float s = 1 * 684.412f;
    float hs = 1 * 684.412f;

    sr = scaleNoise.getRegion(sr, x, z, xSize, zSize, 1.121f, 1.121f, 0.5f);
    dr = depthNoise.getRegion(dr, x, z, xSize, zSize, 200.0f, 200.0f, 0.5f);

    pnr = perlinNoise1.getRegion(pnr, (float)x, (float)y, (float)z, xSize, ySize, zSize, s / 80.0f, hs / 160.0f, s / 80.0f);
    ar = lperlinNoise1.getRegion(ar, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);
    br = lperlinNoise2.getRegion(br, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);

    int p = 0;
    int pp = 0;

    int wScale = 16 / xSize;
    for (int xx = 0; xx < xSize; xx++) {
        int xp = xx * wScale + wScale / 2;
        for (int zz = 0; zz < zSize; zz++) {
            int zp = zz * wScale + wScale / 2;
            float temperature = mTemp[xp * 16 + zp];
            float downfall = mDownfall[xp * 16 + zp] * temperature;
            float dd = 1 - downfall;
            dd = dd * dd;
            dd = dd * dd;
            dd = 1 - dd;

            float scale = ((sr[pp] + 256.0f) / 512);
            scale *= dd;
            if (scale > 1) scale = 1;

            float depth = (dr[pp] / 8000.0f);
            if (depth < 0) depth = -depth * 0.3f;
            depth = depth * 3.0f - 2.0f;

            if (depth < 0) {
                depth = depth / 2;
                if (depth < -1) depth = -1;
                depth = depth / 1.4f;
                depth /= 2;
                scale = 0;
            } else {
                if (depth > 1) depth = 1;
                depth = depth / 8;
            }

            if (scale < 0) scale = 0;
            scale = (scale) + 0.5f;
            depth = depth * ySize / 16;

            float yCenter = ySize / 2.0f + depth * 4;

            pp++;

            for (int yy = 0; yy < ySize; yy++) {
                float val = 0;

                float yOffs = (yy - (yCenter)) * 12 / scale;
                if (yOffs < 0) yOffs *= 4;

                float bb = ar[p] / 512;
                float cc = br[p] / 512;

                float v = (pnr[p] / 10 + 1) / 2;
                if (v < 0) val = bb;
                else if (v > 1) val = cc;
                else val = bb + (cc - bb) * v;
                val -= yOffs;

                if (yy > ySize - 4) {
                    float slide = (yy - (ySize - 4)) / (4 - 1.0f);
                    val = val * (1 - slide) + -10 * slide;
                }

                buffer[p] = val;
                p++;
            }
        }
    }
    return buffer;
}

void McpeGen::prepareChunk(World* w, int chunkX, int chunkZ) {
    int xChunks = 16 / NCELL_W;
    int xSize = xChunks + 1;
    int ySize = 128 / NCELL_H + 1;
    int zSize = xChunks + 1;

    getHeights(chunkX * xChunks, 0, chunkZ * xChunks, xSize, ySize, zSize);

    for (int xc = 0; xc < xChunks; xc++) {
        for (int zc = 0; zc < xChunks; zc++) {
            for (int yc = 0; yc < 128 / NCELL_H; yc++) {
                float yStep = 1 / (float) NCELL_H;
                float s0 = buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s1 = buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 0)];
                float s2 = buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s3 = buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 0)];

                float s0a = (buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 1)] - s0) * yStep;
                float s1a = (buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 1)] - s1) * yStep;
                float s2a = (buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 1)] - s2) * yStep;
                float s3a = (buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 1)] - s3) * yStep;

                for (int y = 0; y < NCELL_H; y++) {
                    float xStep = 1 / (float) NCELL_W;
                    float _s0 = s0;
                    float _s1 = s1;
                    float _s0a = (s2 - s0) * xStep;
                    float _s1a = (s3 - s1) * xStep;

                    for (int x = 0; x < NCELL_W; x++) {
                        float zStep = 1 / (float) NCELL_W;
                        float val = _s0;
                        float vala = (_s1 - _s0) * zStep;

                        for (int z = 0; z < NCELL_W; z++) {
                            int lx = xc * NCELL_W + x;
                            int lz = zc * NCELL_W + z;
                            int gx = chunkX * 16 + lx;
                            int gz = chunkZ * 16 + lz;
                            int gy = yc * NCELL_H + y;

                            float temp = mTemp[lx * 16 + lz];
                            unsigned char id;
                            if (val > 0)        id = BLOCK_STONE;
                            else if (gy < 64)   id = (temp < 0.5f && gy >= 63) ? BLOCK_ICE : BLOCK_CALM_WATER;
                            else                id = BLOCK_AIR;
                            w->blocks[(gx * WORLD_D + gz) * WORLD_H + gy] = id;

                            val += vala;
                        }
                        _s0 += _s0a;
                        _s1 += _s1a;
                    }

                    s0 += s0a; s1 += s1a; s2 += s2a; s3 += s3a;
                }
            }
        }
    }
}

void McpeGen::buildSurfacesChunk(World* w, int chunkX, int chunkZ) {
    const int waterHeight = 64;
    int xOffs = chunkX, zOffs = chunkZ;
    random.setSeed((long)(xOffs * 341872712l + zOffs * 132899541l));
    float s = 1 / 32.0f;
    perlinNoise2.getRegion(sandBuffer,   (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s,     s,     1);
    perlinNoise2.getRegion(gravelBuffer, (float)(xOffs * 16), 109.01340f, (float)(zOffs * 16), 16, 1, 16, s,     1,     s);
    perlinNoise3.getRegion(depthBuffer,  (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s * 2, s * 2, s * 2);

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            float temp = 1;

            BiomeId biome = classifyBiome(mTemp[z + x * 16], mDownfall[z + x * 16]);
            unsigned char bTop, bMat;
            biomeSurface(biome, &bTop, &bMat);

            bool sand   = (sandBuffer[z + x * 16]   + random.nextFloat() * 0.2f) > 0;
            bool gravel = (gravelBuffer[z + x * 16] + random.nextFloat() * 0.2f) > 3;
            int  runDepth = (int)(depthBuffer[z + x * 16] / 3 + 3 + random.nextFloat() * 0.25f);

            int run = -1;
            unsigned char top = bTop;
            unsigned char material = bMat;

            int gx = chunkX * 16 + x, gz = chunkZ * 16 + z;
            for (int y = 127; y >= 0; y--) {
                unsigned char* cell = &w->blocks[(gx * WORLD_D + gz) * WORLD_H + y];

                if (y <= random.nextInt(5)) {
                    *cell = BLOCK_BEDROCK;
                } else {
                    unsigned char old = *cell;
                    if (old == BLOCK_AIR) {
                        run = -1;
                    } else if (old == BLOCK_STONE) {
                        if (run == -1) {
                            if (runDepth <= 0) {
                                top = BLOCK_AIR;
                                material = BLOCK_STONE;
                            } else if (y >= waterHeight - 4 && y <= waterHeight + 1) {
                                top = bTop; material = bMat;
                                if (gravel) { top = BLOCK_AIR;  material = BLOCK_GRAVEL; }
                                if (sand)   { top = BLOCK_SAND; material = BLOCK_SAND; }
                            }
                            if (y < waterHeight && top == BLOCK_AIR) {
                                top = (temp < 0.15f) ? BLOCK_ICE : BLOCK_CALM_WATER;
                            }
                            run = runDepth;
                            *cell = (y >= waterHeight - 1) ? top : material;
                        } else if (run > 0) {
                            run--;
                            *cell = material;
                            if (run == 0 && material == BLOCK_SAND) {
                                run = random.nextInt(4);
                                material = BLOCK_SANDSTONE;
                            }
                        }
                    }
                }
            }
        }
    }
}

void McpeGen::postProcessChunk(World* w, int chunkX, int chunkZ) {
    int xo = chunkX * 16, zo = chunkZ * 16;
    computeBiome(chunkX, chunkZ);
    BiomeId biome = classifyBiome(mTemp[8 * 16 + 8], mDownfall[8 * 16 + 8]);

    random.setSeed(worldSeed);
    int xScale = random.nextInt() / 2 * 2 + 1;
    int zScale = random.nextInt() / 2 * 2 + 1;
    unsigned int h = (unsigned int)chunkX * (unsigned int)xScale + (unsigned int)chunkZ * (unsigned int)zScale;
    random.setSeed((long)(int)(h ^ (unsigned int)worldSeed));

    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16), y = random.nextInt(128), z = zo + random.nextInt(16);
        clayFeature(w, random, x, y, z);
    }

    for (int i = 0; i < 20; i++) { int x = xo + random.nextInt(16), y = random.nextInt(128), z = zo + random.nextInt(16); oreFeature(w, random, x, y, z, BLOCK_DIRT, 32); }
    for (int i = 0; i < 10; i++) { int x = xo + random.nextInt(16), y = random.nextInt(128), z = zo + random.nextInt(16); oreFeature(w, random, x, y, z, BLOCK_GRAVEL, 32); }
    for (int i = 0; i < 20; i++) { int x = xo + random.nextInt(16), y = random.nextInt(128), z = zo + random.nextInt(16); oreFeature(w, random, x, y, z, BLOCK_ORE_COAL, 16); }
    for (int i = 0; i < 20; i++) { int x = xo + random.nextInt(16), y = random.nextInt(64), z = zo + random.nextInt(16); oreFeature(w, random, x, y, z, BLOCK_ORE_IRON, 8); }
    for (int i = 0; i < 2; i++) { int x = xo + random.nextInt(16), y = random.nextInt(32), z = zo + random.nextInt(16); oreFeature(w, random, x, y, z, BLOCK_ORE_GOLD, 8); }
    for (int i = 0; i < 8; i++) { int x = xo + random.nextInt(16), y = random.nextInt(16), z = zo + random.nextInt(16); oreFeature(w, random, x, y, z, BLOCK_ORE_REDSTONE, 7); }
    for (int i = 0; i < 1; i++) { int x = xo + random.nextInt(16), y = random.nextInt(16), z = zo + random.nextInt(16); oreFeature(w, random, x, y, z, BLOCK_ORE_EMERALD, 7); }
    for (int i = 0; i < 1; i++) { int x = xo + random.nextInt(16), y = random.nextInt(16) + random.nextInt(16), z = zo + random.nextInt(16); oreFeature(w, random, x, y, z, BLOCK_ORE_LAPIS, 6); }

    float fss = 0.5f;
    int oFor = (int)((forestNoise.getValue(xo * fss, zo * fss) / 8 + random.nextFloat() * 4 + 4) / 3);
    int forests = 0;
    if (random.nextInt(10) == 0) forests += 1;
    if (biome == B_FOREST)   forests += oFor + 2;
    if (biome == B_RAIN)     forests += oFor + 2;
    if (biome == B_SEASONAL) forests += oFor + 1;
    if (biome == B_TAIGA)    forests += oFor + 1;
    if (biome == B_DESERT)   forests -= 20;
    if (biome == B_TUNDRA)   forests -= 20;
    if (biome == B_PLAINS)   forests -= 20;
    for (int i = 0; i < forests; i++) {
        int tx = xo + random.nextInt(16) + 8, tz = zo + random.nextInt(16) + 8;
        int ty = heightmapAt(w, tx, tz);

        if (biome == B_TAIGA) {
            if (random.nextInt(3) == 0) treePine(w, random, tx, ty, tz);
            else                        treeSpruce(w, random, tx, ty, tz);
        } else if (biome == B_FOREST) {
            if (random.nextInt(5) == 0) treeBirch(w, random, tx, ty, tz);
            else { random.nextInt(3); treeOak(w, random, tx, ty, tz); }
        } else if (biome == B_RAIN) {
            random.nextInt(3);
            treeOak(w, random, tx, ty, tz);
        } else {
            random.nextInt(10);
            treeOak(w, random, tx, ty, tz);
        }
    }

    for (int i = 0; i < 2; i++) { int x = xo + random.nextInt(16) + 8, y = random.nextInt(128), z = zo + random.nextInt(16) + 8; flowerFeature(w, random, x, y, z, BLOCK_FLOWER); }
    if (random.nextInt(2) == 0) { int x = xo + random.nextInt(16) + 8, y = random.nextInt(128), z = zo + random.nextInt(16) + 8; flowerFeature(w, random, x, y, z, BLOCK_ROSE); }
    if (random.nextInt(4) == 0) { int x = xo + random.nextInt(16) + 8, y = random.nextInt(128), z = zo + random.nextInt(16) + 8; mushroomFeature(w, random, x, y, z, BLOCK_MUSHROOM_BROWN); }
    if (random.nextInt(8) == 0) { int x = xo + random.nextInt(16) + 8, y = random.nextInt(128), z = zo + random.nextInt(16) + 8; mushroomFeature(w, random, x, y, z, BLOCK_MUSHROOM_RED); }

    for (int i = 0; i < 10; i++) { int x = xo + random.nextInt(16) + 8, y = random.nextInt(128), z = zo + random.nextInt(16) + 8; reedsFeature(w, random, x, y, z); }

    int cacti = (biome == B_DESERT) ? 5 : 0;
    for (int i = 0; i < cacti; i++) { int x = xo + random.nextInt(16) + 8, y = random.nextInt(128), z = zo + random.nextInt(16) + 8; cactusFeature(w, random, x, y, z); }

    #define SPRING_WATER_TRIES 50
    #define SPRING_LAVA_TRIES  20
    for (int i = 0; i < SPRING_WATER_TRIES; i++) {
        int x = xo + random.nextInt(16) + 8, y = random.nextInt(random.nextInt(120) + 8), z = zo + random.nextInt(16) + 8;
        springFeature(w, x, y, z, BLOCK_WATER);
    }
    for (int i = 0; i < SPRING_LAVA_TRIES; i++) {
        int x = xo + random.nextInt(16) + 8, y = random.nextInt(random.nextInt(random.nextInt(112) + 8) + 8), z = zo + random.nextInt(16) + 8;
        springFeature(w, x, y, z, BLOCK_LAVA);
    }

    snowCap(w, chunkX, chunkZ, mTemp);
}

void worldGenerateMCPE(World* w, long seed) {
    McpeGen* g = new McpeGen(seed);
    int totalChunks = WORLD_CHUNKS_X * WORLD_CHUNKS_Z;
    int doneChunks = 0;
    for (int cz = 0; cz < WORLD_CHUNKS_Z; cz++)
    for (int cx = 0; cx < WORLD_CHUNKS_X; cx++) {

        g->random.setSeed((long)(int)((unsigned int)cx * 341872712u + (unsigned int)cz * 132899541u));
        g->computeBiome(cx, cz);
        g->prepareChunk(w, cx, cz);
        g->buildSurfacesChunk(w, cx, cz);
        caveFeature(w, seed, cx, cz);

        doneChunks++;
        g_terrainProgress = (doneChunks * 50) / totalChunks;

        sceKernelDelayThread(100);
    }

    for (int cz = 0; cz < WORLD_CHUNKS_Z; cz++)
    for (int cx = 0; cx < WORLD_CHUNKS_X; cx++)
        g->postProcessChunk(w, cx, cz);
    delete g;
}
