#include "world/level/world.h"
#include "util/prof.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/tile.h"
#include <pspkernel.h>

#define PLAYER_EDIT_QUEUE_CAP 128

struct EditSection { short ci, si; };
static EditSection g_editQueue[PLAYER_EDIT_QUEUE_CAP];
static int g_editQueueN = 0;

static bool g_inEditQueue[WORLD_CHUNKS_X * WORLD_CHUNKS_Z][N_SECTIONS];

static void editQueueRemoveAt(int i) {
    g_inEditQueue[g_editQueue[i].ci][g_editQueue[i].si] = false;
    for (int j = i + 1; j < g_editQueueN; j++) g_editQueue[j - 1] = g_editQueue[j];
    g_editQueueN--;
}

static void editQueuePushFront(int ci, int si) {
    for (int j = g_editQueueN; j > 0; j--) g_editQueue[j] = g_editQueue[j - 1];
    g_editQueue[0].ci = (short)ci; g_editQueue[0].si = (short)si;
    g_editQueueN++;
    g_inEditQueue[ci][si] = true;
}

static int editQueueFind(int ci, int si) {
    if (!g_inEditQueue[ci][si]) return -1;
    for (int i = 0; i < g_editQueueN; i++)
        if (g_editQueue[i].ci == ci && g_editQueue[i].si == si) return i;
    return -1;
}

void worldEditQueueDropSlot(int slotIdx) {
    for (int i = g_editQueueN - 1; i >= 0; i--)
        if (g_editQueue[i].ci == slotIdx) editQueueRemoveAt(i);
}

static inline void markSecDirty(World* w, int cx, int cz, int y) {

    if (!worldChunkSettled(w, cx, cz)) return;
    if (y < 0 || y >= WORLD_H) return;
    int si = y / SECTION_SY;

    ChunkSection* csec = &worldMesh(w, cx, cz)->sec[si];
    if (!csec->dirty) profAdd(PROFC_MARKED, 1);
    csec->dirty = true;

    if (!w->lightReady) return;

    worldSlot(w, cx, cz)->unsaved = true;

    if (w->simTick) return;

    int ci = worldSlotIndex(w, cx, cz);
    if (g_inEditQueue[ci][si]) return;
    if (g_editQueueN >= PLAYER_EDIT_QUEUE_CAP) return;
    g_editQueue[g_editQueueN].ci = (short)ci; g_editQueue[g_editQueueN].si = (short)si;
    g_editQueueN++;
    g_inEditQueue[ci][si] = true;
}

bool g_smoothLighting = true;

void worldMarkAllDirty(World* w) {
    for (int ci = 0; ci < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; ci++)
        for (int si = 0; si < N_SECTIONS; si++)
            w->chunks[ci].sec[si].dirty = true;
}

void worldMarkDirty(World* w, int x, int y, int z) {

    int cx[2], cz[2], sy[2], ncx = 1, ncz = 1, nsy = 1;
    cx[0] = (x - 1) >> 4;  if (((x + 1) >> 4) != cx[0]) cx[ncx++] = (x + 1) >> 4;
    cz[0] = (z - 1) >> 4;  if (((z + 1) >> 4) != cz[0]) cz[ncz++] = (z + 1) >> 4;
    int ylo = (y > 0) ? y - 1 : 0;
    int yhi = (y < WORLD_H - 1) ? y + 1 : WORLD_H - 1;
    sy[0] = ylo;           if ((yhi >> 4) != (ylo >> 4)) sy[nsy++] = yhi;
    for (int a = 0; a < ncx; a++)
        for (int b = 0; b < ncz; b++)
            for (int c = 0; c < nsy; c++)
                markSecDirty(w, cx[a], cz[b], sy[c]);
}

void worldSetData(World* w, int x, int y, int z, unsigned char data) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return;
    worldDataPut(w, worldIndex(w, x, y, z), data);
    worldMarkDirty(w, x, y, z);
}

void worldSetDataNoUpdate(World* w, int x, int y, int z, unsigned char data) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return;
    worldDataPut(w, worldIndex(w, x, y, z), data);
}

bool worldSetBlockAndData(World* w, int x, int y, int z, unsigned char id, unsigned char data) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return false;
    unsigned char was = worldBlock(w, x, y, z);

    if (!blockPut(w, x, y, z, id)) return false;

    if (was && Tile::isEntityTile[was]) Tile::tiles[was]->onRemove(w, x, y, z);
    worldDataPut(w, worldIndex(w, x, y, z), data);
    worldMarkDirty(w, x, y, z);
    if (w->lightReady) {
        lightOnBlockChanged(w, x, y, z);

        if (lightEmit(was) > 0 && lightEmit(id) == 0) worldRemoveBlockLight(w, x, y, z);
    }
    if (id && Tile::isEntityTile[id]) Tile::tiles[id]->onPlace(w, x, y, z);
    return true;
}

static int g_editBurst = 0;

static bool editQueuePromote(int ci, int si) {
    int i = editQueueFind(ci, si);
    if (i < 0) return false;
    editQueueRemoveAt(i);
    editQueuePushFront(ci, si);
    return true;
}

static void editQueueForceHead(int ci, int si) {
    if (editQueuePromote(ci, si)) return;
    if (g_editQueueN >= PLAYER_EDIT_QUEUE_CAP) {

        editQueueRemoveAt(g_editQueueN - 1);
    }
    editQueuePushFront(ci, si);
}

void worldRebuildRegionNow(World* w, int x0, int y0, int z0, int x1, int y1, int z1) {
    if (!w->lightReady) return;

    if (--y0 < 0) y0 = 0;
    if (++y1 > WORLD_H - 1) y1 = WORLD_H - 1;
    x0--; z0--; x1++; z1++;
    int burst[8][2], nb = 0;
    for (int cx = x0 >> 4; cx <= (x1 >> 4); cx++)
    for (int cz = z0 >> 4; cz <= (z1 >> 4); cz++) {
        if (!worldChunkSettled(w, cx, cz)) continue;
        int ci = worldSlotIndex(w, cx, cz);
        for (int si = y0 / SECTION_SY; si <= y1 / SECTION_SY; si++) {
            editQueueForceHead(ci, si);
            bool seen = false;
            for (int k = 0; k < nb; k++) if (burst[k][0] == ci && burst[k][1] == si) { seen = true; break; }
            if (!seen && nb < 8) { burst[nb][0] = ci; burst[nb][1] = si; nb++; }
        }
    }

    int mx = (x0 + x1) / 2, my = (y0 + y1) / 2, mz = (z0 + z1) / 2;
    if (worldChunkSettled(w, mx >> 4, mz >> 4))
        editQueueForceHead(worldSlotIndex(w, mx >> 4, mz >> 4), my / SECTION_SY);
    g_editBurst = nb;
}

void worldRebuildAroundNow(World* w, int x, int y, int z) {
    if (y < 0 || y >= WORLD_H) return;

    if (w->simTick || !w->lightReady) return;

    int burst[8][2], nb = 0;
    for (int dz = 1; dz >= -1; dz--)
    for (int dx = 1; dx >= -1; dx--)
    for (int dy = 1; dy >= -1; dy--) {
        int nx = x + dx, ny = y + dy, nz = z + dz;
        if (ny < 0 || ny >= WORLD_H) continue;
        int cx = nx >> 4, cz = nz >> 4;
        if (!worldChunkSettled(w, cx, cz)) continue;
        int ci = worldSlotIndex(w, cx, cz), si = ny / SECTION_SY;
        editQueueForceHead(ci, si);
        bool seen = false;
        for (int k = 0; k < nb; k++) if (burst[k][0] == ci && burst[k][1] == si) { seen = true; break; }
        if (!seen && nb < 8) { burst[nb][0] = ci; burst[nb][1] = si; nb++; }
    }

    if (worldChunkSettled(w, x >> 4, z >> 4))
        editQueueForceHead(worldSlotIndex(w, x >> 4, z >> 4), y / SECTION_SY);
    g_editBurst = nb;
}

int worldEditQueueDepth() { return g_editQueueN; }
int worldEditQueueFront(int field) {
    if (!g_editQueueN) return -1;
    return field ? g_editQueue[0].si : g_editQueue[0].ci;
}

void worldDrainPlayerEdits(World* w, int maxSections) {

    static const unsigned int TIME_BUDGET_US = 1000;

    unsigned int tStart = sceKernelGetSystemTimeLow();
    int burst = g_editBurst; g_editBurst = 0;
    int n = g_editQueueN < maxSections ? g_editQueueN : maxSections;
    if (n < burst) n = g_editQueueN < burst ? g_editQueueN : burst;
    for (int i = 0; i < n; i++) {
        int ci = g_editQueue[0].ci, si = g_editQueue[0].si;
        editQueueRemoveAt(0);
        ChunkMesh* c = &w->chunks[ci];
        if (c->sec[si].dirty) chunkBuildSection(c, w, si);
        if (i + 1 >= burst && sceKernelGetSystemTimeLow() - tStart >= TIME_BUDGET_US)
            break;
    }
}

void worldScheduleTick(World* w, int x, int y, int z, unsigned char id, int tickDelay) {

    if (y < 0 || y >= WORLD_H || !worldChunkSettled(w, x >> 4, z >> 4)) return;

    unsigned int key = (unsigned int)worldIndex(w, x, y, z);
    if (!w->tickSet.insert(key).second) return;
    TickNextTickData td = {x, y, z, id, w->time + tickDelay};
    w->tickNextTickList.push_back(td);
}
