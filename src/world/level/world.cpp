
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/levelgen/PerlinNoise.h"
#include "world/level/levelgen/mcpegen.h"
#include "world/level/levelgen/Random.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <pspkernel.h>

static int g_meshBuildCursor = 0;
static long g_worldSeed = 0;

int g_skyDarken = 0;

float worldTimeOfDay(long dayTime, float a) {
    int dayStep = (int)(dayTime % TICKS_PER_DAY);
    float td = (dayStep + a) / (float)TICKS_PER_DAY - 0.25f;
    if (td < 0.0f) td += 1.0f;
    if (td > 1.0f) td -= 1.0f;
    float tdo = td;
    td = 1.0f - (cosf(td * 3.14159265f) + 1.0f) * 0.5f;
    return tdo + (td - tdo) / 3.0f;
}

static int calcSkyDarken(long dayTime) {
    float td = worldTimeOfDay(dayTime, 1.0f);
    float br = 1.0f - (cosf(td * 2.0f * 3.14159265f) * 2.0f + 0.5f);
    if (br < 0.0f) br = 0.0f;
    if (br > 0.80f) br = 0.80f;
    return (int)(br * 11.0f);
}

static bool g_forceNight = false;
void worldSetNightMode(World* w, bool night) {
    if (g_forceNight == night) return;
    g_forceNight = night;
    worldUpdateSkyDarken(w);
}

void worldUpdateSkyDarken(World* w) {
    int nd = g_forceNight ? 8 : calcSkyDarken(w->dayTime);
    if (nd == g_skyDarken) return;
    g_skyDarken = nd;
    for (int ci = 0; ci < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; ci++)
        for (int si = 0; si < N_SECTIONS; si++)
            w->chunks[ci].sec[si].dirty = true;
}

volatile int g_terrainProgress = 0;
volatile bool g_terrainThreadDone = false;

bool worldAllocArrays(World* w) {
    memset(w->chunks, 0, sizeof(w->chunks));

    memset(w->unsaved, 0, sizeof(w->unsaved));

    size_t vol = (size_t)WORLD_W * WORLD_H * WORLD_D;
    w->blocks = (unsigned char*)malloc(vol);
    w->data = (unsigned char*)malloc(vol / 2);
    w->light = (unsigned char*)malloc(vol);
    w->heightmap = (unsigned char*)malloc((size_t)WORLD_W * WORLD_D);
    if (!w->blocks || !w->data || !w->light || !w->heightmap) return false;
    memset(w->blocks, 0, vol);
    memset(w->data, 0, vol / 2);
    memset(w->light, 0, vol);
    memset(w->heightmap, 0, (size_t)WORLD_W * WORLD_D);

    w->time = 0;
    w->dayTime = 0;
    g_skyDarken = 0;

    g_forceNight = false;
    w->tickNextTickList.clear();
    w->tickSet.clear();
    w->lightQueue.clear();
    w->preservedTileEntities.clear();
    w->lightReady = false;
    chunkInitBrightRamp();
    g_meshBuildCursor = 0;
    return true;
}

bool worldInitTerrain(World* w, long seed) {
    g_terrainProgress = 0;
    g_terrainThreadDone = false;
    if (!worldAllocArrays(w)) return false;

    g_worldSeed = seed;
    clock_t t0 = clock();
    worldGenerateMCPE(w, seed);
    worldSettleLiquids(w);

    g_terrainProgress = 60;
    worldInitLight(w);
    g_terrainProgress = 90;

    w->lightReady = true;
    worldPlaceMushrooms(w);
    worldPlaceFlowers(w);

    g_terrainProgress = 100;
    clock_t t1 = clock();

    printf("[WORLDGEN] noise gen: %d ms\n", (int)((t1 - t0) * 1000 / CLOCKS_PER_SEC));

    g_meshBuildCursor = 0;
    return true;
}

int worldBuildMeshesStep(World* w, int maxChunks) {

    static const float spawnX = WORLD_W * 0.5f, spawnZ = WORLD_D * 0.5f;

    extern float g_viewDist;
    const float maxD2 = g_viewDist * g_viewDist;
    const int total = WORLD_CHUNKS_X * WORLD_CHUNKS_Z;

    int budget = maxChunks;
    while (g_meshBuildCursor < total && budget-- > 0) {
        int cx = g_meshBuildCursor % WORLD_CHUNKS_X, cz = g_meshBuildCursor / WORLD_CHUNKS_X;
        ChunkMesh* c = &w->chunks[cz * WORLD_CHUNKS_X + cx];
        int ox = cx * CHUNK_SX, oz = cz * CHUNK_SZ;
        float dx = (ox + CHUNK_SX * 0.5f) - spawnX, dz = (oz + CHUNK_SZ * 0.5f) - spawnZ;
        if (dx * dx + dz * dz <= maxD2) chunkBuildMesh(c, w, ox, oz);
        else chunkInitLazy(c, ox, oz);
        g_meshBuildCursor++;
    }
    return g_meshBuildCursor;
}

static unsigned char columnTop(World* w, int x, int z, int* outY) {
    for (int y = WORLD_H - 1; y >= 0; y--) {
        unsigned char id = w->blocks[worldIndex(x, y, z)];
        if (id != BLOCK_AIR) { *outY = y; return id; }
    }
    *outY = 0; return BLOCK_AIR;
}

static bool isValidSpawn(World* w, int x, int z) {
    int ty; unsigned char top = columnTop(w, x, z, &ty);
    return isSolidPhys(top) && top != BLOCK_LEAVES;
}

void worldFindSpawn(World* w, int* outX, int* outZ, int* outFeetY) {
    Random random(g_worldSeed);
    int xSpawn = WORLD_W / 2, zSpawn = WORLD_D / 2;

    int guard = 0;
    while (!isValidSpawn(w, xSpawn, zSpawn) && guard++ < 10000) {
        xSpawn += random.nextInt(32) - random.nextInt(32);
        zSpawn += random.nextInt(32) - random.nextInt(32);
        if (xSpawn < 4) xSpawn += 32;
        if (xSpawn >= WORLD_W - 4) xSpawn -= 32;
        if (zSpawn < 4) zSpawn += 32;
        if (zSpawn >= WORLD_D - 4) zSpawn -= 32;
    }

    guard = 0;
    while (!isValidSpawn(w, xSpawn, zSpawn) && guard++ < 10000) {
        xSpawn += random.nextInt(8) - random.nextInt(8);
        zSpawn += random.nextInt(8) - random.nextInt(8);
        if (xSpawn < 4) xSpawn += 8;
        if (xSpawn >= WORLD_W - 4) xSpawn -= 8;
        if (zSpawn < 4) zSpawn += 8;
        if (zSpawn >= WORLD_D - 4) zSpawn -= 8;
    }

    int ty; columnTop(w, xSpawn, zSpawn, &ty);
    *outX = xSpawn; *outZ = zSpawn; *outFeetY = ty + 1;
}

void worldFree(World* w) {
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++)
        chunkFreeMesh(&w->chunks[i]);
    if (w->blocks) { free(w->blocks); w->blocks = 0; }
    if (w->data) { free(w->data); w->data = 0; }
    if (w->light) { free(w->light); w->light = 0; }
    if (w->heightmap) { free(w->heightmap); w->heightmap = 0; }
    w->tickNextTickList.clear();
    w->tickSet.clear();
    w->lightQueue.clear();
}
