
#include "world/level/world.h"
#include "world/level/storage/chunk_storage.h"
#include "world/level/chunk/chunk_cache.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/levelgen/PerlinNoise.h"
#include "world/level/levelgen/mcpegen.h"
#include "world/level/levelgen/Random.h"

#include <stdlib.h>
#include <string.h>

#define WORLDGEN_PROFILE 0
#if WORLDGEN_PROFILE
#include <stdio.h>
#include <time.h>
#endif
#include <math.h>
#include <pspkernel.h>

static int g_meshBuildCursor = 0;
static long g_worldSeed = 0;

long worldSeed(void) { return g_worldSeed; }

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

static const long MIDDLE_OF_NIGHT_TIME = 12000;
static bool g_nightMode = false;
void worldSetNightMode(World* w, bool night) { (void)w; g_nightMode = night; }

bool worldNightModeTick(World* w) {
    if (!g_nightMode) return false;
    long curTime = w->dayTime;
    if (curTime % TICKS_PER_DAY != MIDDLE_OF_NIGHT_TIME) {
        if (curTime % TICKS_PER_DAY < MIDDLE_OF_NIGHT_TIME && (curTime + 20) % TICKS_PER_DAY > MIDDLE_OF_NIGHT_TIME)
            curTime = MIDDLE_OF_NIGHT_TIME;
        else
            curTime += 20;
        w->dayTime = curTime % TICKS_PER_DAY;
    }
    return true;
}

void worldUpdateSkyDarken(World* w) {
    int nd = calcSkyDarken(w->dayTime);
    if (nd == g_skyDarken) return;
    g_skyDarken = nd;
    for (int ci = 0; ci < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; ci++)
        for (int si = 0; si < N_SECTIONS; si++)
            if (w->chunks[ci].sec[si].skyLit)
                w->chunks[ci].sec[si].dirty = true;
}

volatile int g_terrainProgress = 0;
volatile bool g_terrainThreadDone = false;

bool worldAllocArrays(World* w) {

    worldSetWindow(w, WORLD_SLOT_BITS);
    memset(w->chunks, 0, sizeof(w->chunks));

    chunkStorageShutdown();

    memset(w->slots, 0, sizeof(w->slots));
    worldSlotsReset(w);

    blockAlloc(w);

    w->dataCol = (unsigned char**)calloc((size_t)WORLD_W * WORLD_D, sizeof(unsigned char*));
    w->dataPages = 0;

    bool lightOk = lightAlloc(w);
    w->heightmap = (unsigned char*)malloc((size_t)WORLD_W * WORLD_D);
    if (!w->dataCol || !lightOk || !w->heightmap) {

        worldFree(w);
        return false;
    }
    memset(w->heightmap, 0, (size_t)WORLD_W * WORLD_D);

    w->time = 0;
    w->dayTime = 0;
    g_skyDarken = 0;

    g_nightMode = false;
    w->tickNextTickList.clear();
    w->tickSet.clear();
    w->lightQueue.clear();
    lightQueuesReserve(w);
    w->preservedTileEntities.clear();
    w->lightReady = false;
    chunkInitBrightRamp();
    g_meshBuildCursor = 0;
    return true;
}

bool worldInitTerrain(World* w, long seed, int worldType) {
    g_terrainProgress = 0;
    g_terrainThreadDone = false;
    if (!worldAllocArrays(w)) return false;

    g_worldSeed = seed;
#if WORLDGEN_PROFILE
    clock_t t0 = clock();
#endif

    levelSourceFor(worldType).buildTerrain(w, seed);

    g_terrainProgress = 60;
    worldInitLight(w);
    g_terrainProgress = 90;

    w->lightReady = true;

    worldPlaceMushrooms(w);
    worldPlaceFlowers(w);

    g_terrainProgress = 100;
#if WORLDGEN_PROFILE
    printf("[WORLDGEN] noise gen: %d ms\n", (int)((clock() - t0) * 1000 / CLOCKS_PER_SEC));
#endif

    g_meshBuildCursor = 0;
    return true;
}

int worldBuildMeshesStep(World* w, int maxChunks) {

    extern float g_viewDist;
    const float maxD2 = g_viewDist * g_viewDist;
    const float px = g_level.player ? g_level.player->x : 0.0f;
    const float pz = g_level.player ? g_level.player->z : 0.0f;
    const int total = w->slotN * w->slotN;

    int budget = maxChunks;
    while (g_meshBuildCursor < total && budget-- > 0) {
        LevelChunk* lc = &w->slots[g_meshBuildCursor];
        ChunkMesh* c = &w->chunks[g_meshBuildCursor];
        g_meshBuildCursor++;
        if (!lc->resident) continue;
        int ox = lc->x * CHUNK_SX, oz = lc->z * CHUNK_SZ;
        float dx = (ox + CHUNK_SX * 0.5f) - px, dz = (oz + CHUNK_SZ * 0.5f) - pz;

        if (dx * dx + dz * dz <= maxD2) chunkBuildMesh(c, w, ox, oz);
        else chunkInitLazy(c, ox, oz);
    }
    return g_meshBuildCursor;
}

static unsigned char columnTop(World* w, int x, int z, int* outY) {
    for (int y = WORLD_H - 1; y >= 0; y--) {
        unsigned char id = worldBlock(w, x, y, z);
        if (id != BLOCK_AIR) { *outY = y; return id; }
    }
    *outY = 0; return BLOCK_AIR;
}

static bool isValidSpawn(World* w, int x, int z) {

    if (!worldChunkSettled(w, x >> 4, z >> 4)) return false;
    int ty; unsigned char top = columnTop(w, x, z, &ty);
    return isSolidPhys(top) && top != BLOCK_LEAVES;
}

#define SPAWN_SEARCH_CHUNKS 2

static void clampToArea(int cx0, int cz0, int* x, int* z, int step) {
    const int lo = 4, hi = (SPAWN_SEARCH_CHUNKS * 2 + 1) * 16 - 4;
    int ox = (cx0 - SPAWN_SEARCH_CHUNKS) * 16, oz = (cz0 - SPAWN_SEARCH_CHUNKS) * 16;
    if (*x < ox + lo) *x += step;
    if (*x >= ox + hi) *x -= step;
    if (*z < oz + lo) *z += step;
    if (*z >= oz + hi) *z -= step;
}

static bool nearestValidSpawn(World* w, int* x, int* z) {
    const int maxR = w->slotN * 16;
    for (int r = 0; r <= maxR; r += 2)
        for (int dx = -r; dx <= r; dx += 2)
            for (int dz = -r; dz <= r; dz += 2) {
                if (dx != -r && dx != r && dz != -r && dz != r) continue;
                if (isValidSpawn(w, *x + dx, *z + dz)) { *x += dx; *z += dz; return true; }
            }
    return false;
}

static void spawnSearchArea(World* w, int x, int z) {
    worldEnsureArea(w, x >> 4, z >> 4, SPAWN_SEARCH_CHUNKS);
}

void worldValidateSpawn(World* w, int* x, int* y, int* z) {
    if (*y <= 0) *y = 64;

    spawnSearchArea(w, *x, *z);
    const int cx0 = *x >> 4, cz0 = *z >> 4;
    Random random(g_worldSeed);
    int xs = *x, zs = *z;
    int guard = 0;
    while (!isValidSpawn(w, xs, zs) && guard++ < 10000) {
        xs += random.nextInt(8) - random.nextInt(8);
        zs += random.nextInt(8) - random.nextInt(8);
        clampToArea(cx0, cz0, &xs, &zs, 8);
    }
    if (!isValidSpawn(w, xs, zs)) nearestValidSpawn(w, &xs, &zs);

    const bool standing = isSolidPhys(worldBlock(w, xs, *y - 2, zs)) &&
                          !isSolidPhys(worldBlock(w, xs, *y - 1, zs)) &&
                          !isSolidPhys(worldBlock(w, xs, *y, zs));
    if (xs != *x || zs != *z || !standing) {
        int ty; columnTop(w, xs, zs, &ty);
        *y = ty + 2;
    }
    *x = xs; *z = zs;
}

void worldFindSpawn(World* w, int* outX, int* outZ, int* outFeetY) {
    Random random(g_worldSeed);

    int wox, woz; worldWindowOrigin(w, &wox, &woz);
    int xSpawn = wox + w->slotN * 8, zSpawn = woz + w->slotN * 8;

    spawnSearchArea(w, xSpawn, zSpawn);
    const int cx0 = xSpawn >> 4, cz0 = zSpawn >> 4;

    int guard = 0;
    while (!isValidSpawn(w, xSpawn, zSpawn) && guard++ < 10000) {
        xSpawn += random.nextInt(32) - random.nextInt(32);
        zSpawn += random.nextInt(32) - random.nextInt(32);
        clampToArea(cx0, cz0, &xSpawn, &zSpawn, 32);
    }

    guard = 0;
    while (!isValidSpawn(w, xSpawn, zSpawn) && guard++ < 10000) {
        xSpawn += random.nextInt(8) - random.nextInt(8);
        zSpawn += random.nextInt(8) - random.nextInt(8);
        clampToArea(cx0, cz0, &xSpawn, &zSpawn, 8);
    }

    if (!isValidSpawn(w, xSpawn, zSpawn)) nearestValidSpawn(w, &xSpawn, &zSpawn);
    int ty; columnTop(w, xSpawn, zSpawn, &ty);

    if (!isValidSpawn(w, xSpawn, zSpawn)) ty = 63;
    *outX = xSpawn; *outZ = zSpawn; *outFeetY = ty + 1;
}

void worldFree(World* w) {

    chunkStorageShutdown();

    worldGenWorkerStop();
    worldGenFree();
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++)
        chunkFreeMesh(&w->chunks[i]);
    blockFree(w);
    if (w->dataCol) {

        for (int i = 0; i < WORLD_W * WORLD_D; i++)
            if (w->dataCol[i]) free(w->dataCol[i]);
        free(w->dataCol); w->dataCol = 0; w->dataPages = 0;
    }
    lightFree(w);
    if (w->heightmap) { free(w->heightmap); w->heightmap = 0; }
    w->tickNextTickList.clear();
    w->tickSet.clear();
    w->lightQueue.clear();

    w->preservedTileEntities.clear();
}
