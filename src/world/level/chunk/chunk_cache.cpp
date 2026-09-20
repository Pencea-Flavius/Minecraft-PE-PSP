#include "world/level/chunk/chunk_cache.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/levelgen/mcpegen.h"
#include "world/level/levelgen/level_source.h"
#include "world/level/storage/chunk_storage.h"

#include <string.h>
#include <math.h>
#include <pspkernel.h>
#include "util/prof.h"

unsigned int g_streamIn = 0, g_streamOut = 0;

struct GenScope {
    World* w; bool saved;
    GenScope(World* world) : w(world), saved(world->lightReady) { w->lightReady = false; }
    ~GenScope() { w->lightReady = saved; }
};

enum { ST_DONE = 0, ST_DECOR0 = 1, ST_LIGHT = 5, ST_MISC = 6 };

static void evict(World* w, int slotIdx) {
    LevelChunk* c = &w->slots[slotIdx];
    if (!c->resident) return;
    profBegin(PROF_SEVICT);

    if (c->unsaved && !worldSlotBusy(c)) chunkStorageSave(w, c->x, c->z);
    c->resident = false;

    worldEditQueueDropSlot(slotIdx);
    blockSlotRecycle(w, slotIdx);
    lightSlotRelease(w, slotIdx);

    for (int i = 0; i < 256; i++) {
        unsigned char** pg = &w->dataCol[(slotIdx << 8) | i];
        if (*pg) { free(*pg); *pg = 0; w->dataPages--; }
    }
    memset(&w->heightmap[slotIdx << 8], 0, 256);

    chunkFreeMesh(&w->chunks[slotIdx]);
    g_streamOut++;
    profEnd(PROF_SEVICT);
}

static bool postProcessPhase(World* w, int cx, int cz, int phase) {
    LevelChunk* c = worldSlot(w, cx, cz);
    if (!c->isAt(cx, cz)) return true;

    if (phase > 0) return chunkPostProcessPhase(w, cx, cz, phase);
    if (c->terrainPopulated) return true;

    if (!activeLevelSource().supportsGenFeatures()) { c->terrainPopulated = true; return true; }

    if (!worldNeighbourSettled(w, cx + 1, cz) || !worldNeighbourSettled(w, cx, cz + 1) ||
        !worldNeighbourSettled(w, cx + 1, cz + 1)) return true;
    c->terrainPopulated = true;

    for (int dz = 0; dz <= 1; dz++)
        for (int dx = 0; dx <= 1; dx++)
            if (worldChunkReady(w, cx + dx, cz + dz))
                worldSlot(w, cx + dx, cz + dz)->unsaved = true;
    return chunkPostProcessPhase(w, cx, cz, 0);
}

static void claim(World* w, int cx, int cz) {
    int slot = worldSlotIndex(w, cx, cz);
    evict(w, slot);

    lightSlotRelease(w, slot);
    LevelChunk* c = &w->slots[slot];
    c->x = cx; c->z = cz;
    c->unsaved = false;
    c->terrainPopulated = false;
    c->stage = ST_DONE;

    c->resident = true;
    chunkInitLazy(&w->chunks[slot], cx * 16, cz * 16);
}

static bool s_pend = false;
static int  s_pendX = 0, s_pendZ = 0;

static int  s_decorPhase = 0;

static void finishBegin(World* w, int cx, int cz) {
    LevelChunk* c = worldSlot(w, cx, cz);

    c->generating = false;
    c->stage = ST_DECOR0;
    s_decorPhase = 0;
    s_pend = true; s_pendX = cx; s_pendZ = cz;
}

bool worldStreamBusy() { return s_pend; }

static bool finishStep(World* w) {
    if (!s_pend) return false;
    const int cx = s_pendX, cz = s_pendZ;
    LevelChunk* c = worldSlot(w, cx, cz);

    if (!c->isAt(cx, cz)) { c->generating = false; c->stage = ST_DONE; s_pend = false; return false; }

    GenScope gen(w);
    switch (c->stage) {

    case 1: case 2: case 3: case 4: {
        static const int kDx[4] = { 0, -1,  0, -1 };
        static const int kDz[4] = { 0,  0, -1, -1 };
        int i = c->stage - ST_DECOR0;
        profBegin(PROF_SDECOR);

        const unsigned int DECOR_BUDGET_US = 2000;
        unsigned int t0 = sceKernelGetSystemTimeLow();
        bool decorDone;
        do {
            decorDone = postProcessPhase(w, cx + kDx[i], cz + kDz[i], s_decorPhase);
            s_decorPhase++;
        } while (!decorDone &&
                 (unsigned int)(sceKernelGetSystemTimeLow() - t0) < DECOR_BUDGET_US);
        profEnd(PROF_SDECOR);
        if (!decorDone) return true;
        s_decorPhase = 0;
        break;
    }

    case ST_LIGHT:
        profBegin(PROF_SLIGHT);
        worldInitChunkLight(w, cx, cz);
        profEnd(PROF_SLIGHT);
        break;
    case ST_MISC:
    default:
        profBegin(PROF_SMISC);

        worldPlaceMushrooms(w);
        worldPlaceFlowers(w);
        worldScheduleChunkTicks(w, cx, cz);
        profEnd(PROF_SMISC);
        c->stage = ST_DONE;
        s_pend = false;
        g_streamIn++;
        return true;
    }
    c->stage++;
    return true;
}

void worldGetChunk(World* w, int cx, int cz) {

    if (!worldChunkInBounds(cx, cz)) return;
    if (worldChunkReady(w, cx, cz)) return;

    if (chunkStorageUnreachable()) return;
    claim(w, cx, cz);
    {
        GenScope gen(w);
        bool gotLight = false, populated = true;
        profBegin(PROF_SDISK);
        bool fromDisk = chunkStorageLoad(w, cx, cz, &gotLight, &populated);
        profEnd(PROF_SDISK);
        if (fromDisk) {

            worldSlot(w, cx, cz)->terrainPopulated = populated;
        } else {
            profBegin(PROF_SGEN);
            activeLevelSource().buildChunk(w, cx, cz);
            profEnd(PROF_SGEN);

        }
    }

    finishBegin(w, cx, cz);
    while (finishStep(w)) {}
}

void worldEnsureArea(World* w, int cx, int cz, int r) {
    for (int dz = -r; dz <= r; dz++)
        for (int dx = -r; dx <= r; dx++)
            worldGetChunk(w, cx + dx, cz + dz);
}

static volatile bool g_jobPending = false, g_jobDone = false, g_workerQuit = false;
static volatile int  g_jobX = 0, g_jobZ = 0;
static int s_workerThid = -1;

static World* volatile s_genWorld = 0;

static int genWorker(SceSize, void*) {
    while (!g_workerQuit) {
        if (!g_jobPending) { sceKernelDelayThread(2000); continue; }
        activeLevelSource().buildChunk(s_genWorld, g_jobX, g_jobZ);
        g_jobPending = false;
        g_jobDone = true;
    }
    return 0;
}

void worldGenWorkerStart(World* w) {
    if (s_workerThid >= 0) return;

    if (worldFitsInWindow(w)) return;
    g_workerQuit = false; g_jobPending = false; g_jobDone = false; s_pend = false;

    s_workerThid = sceKernelCreateThread("chunk_gen", genWorker, 0x24, 0x10000, 0, 0);
    if (s_workerThid >= 0) sceKernelStartThread(s_workerThid, 0, 0);
}

void worldGenWorkerStop() {
    if (s_workerThid < 0) return;
    g_workerQuit = true;

    sceKernelWaitThreadEnd(s_workerThid, 0);
    sceKernelDeleteThread(s_workerThid);
    s_workerThid = -1;

    g_jobPending = false; g_jobDone = false; s_pend = false;
}

static int loadRadius(const World* w) {
    extern float g_viewDistEff;
    float d = (g_viewDistEff > 0.0f) ? g_viewDistEff : WORLD_VIEW_DIST;
    int r = (int)(d / 16.0f) + 2;

    if (r < 4) r = 4;
    int cap = w->slotN / 2;
    return r > cap ? cap : r;
}

int worldStream(World* w, float px, float pz, int budgetMs) {

    if (worldFitsInWindow(w)) return 0;
    const int pcx = (int)floorf(px) >> 4, pcz = (int)floorf(pz) >> 4;
    const int R = loadRadius(w);
    const int E = R + 1;
    const unsigned int tStart = sceKernelGetSystemTimeLow();
    int brought = 0;

    const unsigned int EVICT_BUDGET_US = 2000;
    for (int i = 0; i < w->slotN * w->slotN; i++) {
        LevelChunk* c = &w->slots[i];
        if (!c->resident) continue;
        if (c->x >= pcx - E && c->x <= pcx + E && c->z >= pcz - E && c->z <= pcz + E) continue;
        if (worldSlotBusy(c)) continue;
        evict(w, i);
        if ((unsigned int)(sceKernelGetSystemTimeLow() - tStart) > EVICT_BUDGET_US) break;
    }

    if (g_jobDone && !s_pend) {
        g_jobDone = false;
        LevelChunk* c = worldSlot(w, g_jobX, g_jobZ);

        if (c->x == g_jobX && c->z == g_jobZ && c->resident) finishBegin(w, g_jobX, g_jobZ);
        else                                                 c->generating = false;
    }

    if (finishStep(w)) {
        if (!s_pend) brought++;
        return brought;
    }
    if (g_jobPending) return brought;

    {
        if ((unsigned int)(sceKernelGetSystemTimeLow() - tStart) > (unsigned int)budgetMs * 1000u)
            return brought;
        int bestX = 0, bestZ = 0, bestD = 0x7FFFFFFF;
        for (int dz = -R; dz <= R; dz++)
            for (int dx = -R; dx <= R; dx++) {
                int cx = pcx + dx, cz = pcz + dz;
                if (!worldChunkInBounds(cx, cz)) continue;
                if (worldChunkReady(w, cx, cz)) continue;
                int d = dx * dx + dz * dz;
                if (d < bestD) { bestD = d; bestX = cx; bestZ = cz; }
            }
        if (bestD == 0x7FFFFFFF) return brought;

        if (chunkStorageUnreachable()) return brought;
        claim(w, bestX, bestZ);

        bool gotLight = false, populated = true;
        profBegin(PROF_SDISK);
        bool onDisk = chunkStorageLoad(w, bestX, bestZ, &gotLight, &populated);
        profEnd(PROF_SDISK);
        if (onDisk) {
            worldSlot(w, bestX, bestZ)->terrainPopulated = populated;
            finishBegin(w, bestX, bestZ);
            return brought;
        }
        if (s_workerThid < 0) {
            GenScope gen(w);
            profBegin(PROF_SGEN);
            activeLevelSource().buildChunk(w, bestX, bestZ);
            profEnd(PROF_SGEN);
            finishBegin(w, bestX, bestZ);
            return brought;
        }

        worldSlot(w, bestX, bestZ)->generating = true;
        s_genWorld = w;
        g_jobX = bestX; g_jobZ = bestZ;
        g_jobPending = true;
    }
    return brought;
}

void worldSaveResident(World* w) {
    for (int i = 0; i < w->slotN * w->slotN; i++) {
        LevelChunk* c = &w->slots[i];

        if (c->resident && c->unsaved && !worldSlotBusy(c)) chunkStorageSave(w, c->x, c->z);
    }
}
