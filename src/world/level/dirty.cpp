#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include <pspkernel.h>

unsigned int g_editWorstUs = 0, g_drainWorstUs = 0;
static unsigned int g_editWindowStart = 0, g_drainWindowStart = 0;
void worldRecordEditUs(unsigned int us) {
    unsigned int now = sceKernelGetSystemTimeLow();
    if (now - g_editWindowStart > 1000000) { g_editWorstUs = 0; g_editWindowStart = now; }
    if (us > g_editWorstUs) g_editWorstUs = us;
}
static void recordDrainUs(unsigned int us) {
    unsigned int now = sceKernelGetSystemTimeLow();
    if (now - g_drainWindowStart > 1000000) { g_drainWorstUs = 0; g_drainWindowStart = now; }
    if (us > g_drainWorstUs) g_drainWorstUs = us;
}

unsigned int g_lightWorstUs = 0, g_rebuildWorstUs = 0;
static unsigned int g_lightWindowStart = 0, g_rebuildWindowStart = 0;
void worldRecordLightUs(unsigned int us) {
    unsigned int now = sceKernelGetSystemTimeLow();
    if (now - g_lightWindowStart > 1000000) { g_lightWorstUs = 0; g_lightWindowStart = now; }
    if (us > g_lightWorstUs) g_lightWorstUs = us;
}
void worldRecordRebuildUs(unsigned int us) {
    unsigned int now = sceKernelGetSystemTimeLow();
    if (now - g_rebuildWindowStart > 1000000) { g_rebuildWorstUs = 0; g_rebuildWindowStart = now; }
    if (us > g_rebuildWorstUs) g_rebuildWorstUs = us;
}

#define PLAYER_EDIT_QUEUE_CAP 128
static int g_editQueue[PLAYER_EDIT_QUEUE_CAP][2];
static int g_editQueueN = 0;

static bool g_inEditQueue[WORLD_CHUNKS_X * WORLD_CHUNKS_Z][N_SECTIONS];

static inline void markSecDirty(World* w, int cx, int cz, int y) {
    if (cx < 0 || cx >= WORLD_CHUNKS_X || cz < 0 || cz >= WORLD_CHUNKS_Z) return;
    if (y < 0 || y >= WORLD_H) return;
    int si = y / SECTION_SY;
    w->chunks[cz * WORLD_CHUNKS_X + cx].sec[si].dirty = true;

    w->unsaved[cz * WORLD_CHUNKS_X + cx] = true;

    int ci = cz * WORLD_CHUNKS_X + cx;
    if (g_inEditQueue[ci][si]) return;
    if (g_editQueueN >= PLAYER_EDIT_QUEUE_CAP) return;
    g_editQueue[g_editQueueN][0] = ci; g_editQueue[g_editQueueN][1] = si; g_editQueueN++;
    g_inEditQueue[ci][si] = true;
}

void worldMarkDirty(World* w, int x, int y, int z) {
    for (int dx = -1; dx <= 1; dx++)
    for (int dz = -1; dz <= 1; dz++)
    for (int dy = -1; dy <= 1; dy++)
        markSecDirty(w, (x + dx) / CHUNK_SX, (z + dz) / CHUNK_SZ, y + dy);
}

void worldSetData(World* w, int x, int y, int z, unsigned char data) {
    if (y < 0 || y >= WORLD_H || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return;
    worldDataPut(w, worldIndex(x, y, z), data);
    worldMarkDirty(w, x, y, z);
}

void worldSetDataNoUpdate(World* w, int x, int y, int z, unsigned char data) {
    if (y < 0 || y >= WORLD_H || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return;
    worldDataPut(w, worldIndex(x, y, z), data);
}

bool worldSetBlockAndData(World* w, int x, int y, int z, unsigned char id, unsigned char data) {
    if (y < 0 || y >= WORLD_H || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return false;
    int idx = worldIndex(x, y, z);
    unsigned char was = w->blocks[idx];
    w->blocks[idx] = id;
    worldDataPut(w, idx, data);
    worldMarkDirty(w, x, y, z);
    if (w->lightReady) {
        lightOnBlockChanged(w, x, y, z);

        if (lightEmit(was) > 0 && lightEmit(id) == 0) worldRemoveBlockLight(w, x, y, z);
    }
    return true;
}

void worldRebuildAroundNow(World* w, int x, int y, int z) {

    static const int FACE7[7][3] = { {0,0,0}, {-1,0,0},{1,0,0}, {0,-1,0},{0,1,0}, {0,0,-1},{0,0,1} };
    static const int MAX_SYNC_SECTIONS = 2;
    int done[7][2]; int nd = 0;
    for (int i = 0; i < 7; i++) {
        int cx = (x + FACE7[i][0]) / CHUNK_SX, cz = (z + FACE7[i][2]) / CHUNK_SZ, yy = y + FACE7[i][1];
        if (cx < 0 || cx >= WORLD_CHUNKS_X || cz < 0 || cz >= WORLD_CHUNKS_Z) continue;
        if (yy < 0 || yy >= WORLD_H) continue;
        int ci = cz * WORLD_CHUNKS_X + cx, si = yy / SECTION_SY;
        bool seen = false;
        for (int j = 0; j < nd; j++) if (done[j][0] == ci && done[j][1] == si) { seen = true; break; }
        if (seen) continue;
        done[nd][0] = ci; done[nd][1] = si; nd++;
        if (nd > MAX_SYNC_SECTIONS) continue;
        ChunkMesh* c = &w->chunks[ci];
        if (c->sec[si].dirty) chunkBuildSection(c, w, si);
    }
}

void worldDrainPlayerEdits(World* w, int maxSections) {

    static const unsigned int TIME_BUDGET_US = 1000;
    unsigned int tStart = sceKernelGetSystemTimeLow();
    int n = g_editQueueN < maxSections ? g_editQueueN : maxSections;
    for (int i = 0; i < n; i++) {
        int ci = g_editQueue[0][0], si = g_editQueue[0][1];
        for (int j = 1; j < g_editQueueN; j++) { g_editQueue[j-1][0] = g_editQueue[j][0]; g_editQueue[j-1][1] = g_editQueue[j][1]; }
        g_editQueueN--;
        g_inEditQueue[ci][si] = false;
        ChunkMesh* c = &w->chunks[ci];
        if (c->sec[si].dirty) chunkBuildSection(c, w, si);
        if (sceKernelGetSystemTimeLow() - tStart >= TIME_BUDGET_US) break;
    }

    recordDrainUs(sceKernelGetSystemTimeLow() - tStart);
}

void worldScheduleTick(World* w, int x, int y, int z, unsigned char id, int tickDelay) {
    if (y < 0 || y >= WORLD_H || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return;

    unsigned int key = (unsigned int)worldIndex(x, y, z);
    if (!w->tickSet.insert(key).second) return;
    TickNextTickData td = {x, y, z, id, w->time + tickDelay};
    w->tickNextTickList.push_back(td);
}
