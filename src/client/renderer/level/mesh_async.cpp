

#include "client/renderer/level/mesh_async.h"
#include "world/level/chunk/chunk.h"
#include "platform/me/me.h"
#include <malloc.h>
#include <pspkernel.h>

#define UNCACHED_USER_MASK  0x40000000

#define ME_SCRATCH_VERTS  16384
static ChunkVertex s_meScratch[ME_SCRATCH_VERTS] __attribute__((aligned(64)));

enum class MePhase : unsigned char { IDLE, COUNTING, EMITTING };

struct AsyncSlot {
    ChunkMesh* c;
    int si;
    int layer;
    MePhase phase;
    int meCount;
    int wait;
    DrawVertex* pend[4];
    int pendN[4];
    bool active;
    bool retry;
};
static AsyncSlot g_as = {};

extern volatile int g_meshOOM;

int g_asyncMeN = 0, g_asyncCpuN = 0, g_asyncDispatch = 0, g_asyncMismatch = 0;
int g_asyncDirty = 0;
int g_asyncFrames = 0;
int g_asyncState = 0;

static void asyncFinalize(World* w) {
    ChunkSection* s = &g_as.c->sec[g_as.si];
    int y0 = g_as.si * SECTION_SY;
    int ox = g_as.c->ox, oz = g_as.c->oz;
    s->ox = ox; s->oy = y0; s->oz = oz;

    DrawVertex* oldMesh   = s->mesh;
    DrawVertex* oldWater  = s->water;
    DrawVertex* oldLeaves = s->leaves;
    DrawVertex* oldNoMip  = s->noMip;

    s->mesh   = g_as.pend[0]; s->vertexCount = g_as.pendN[0];
    s->water  = g_as.pend[1]; s->waterCount  = g_as.pendN[1];
    s->leaves = g_as.pend[2]; s->leavesCount = g_as.pendN[2];
    s->noMip  = g_as.pend[3]; s->noMipCount  = g_as.pendN[3];

    if (oldMesh)   free(oldMesh);
    if (oldWater)  free(oldWater);
    if (oldLeaves) free(oldLeaves);
    if (oldNoMip)  free(oldNoMip);

    int totalVerts = s->vertexCount + s->waterCount + s->leavesCount + s->noMipCount;
    if (totalVerts == 0) {
        s->by0 = (float)y0; s->by1 = (float)(y0 + SECTION_SY);
        s->lby0 = s->lby1 = (float)y0;
        s->wby0 = s->wby1 = (float)y0;
        for (int f = 0; f < 6; f++) s->vis[f] = 0x3F;
        if (g_as.retry) { g_meshOOM = 1; s->dirty = true; }
        else              s->dirty = false;
        g_as.active = false;
        return;
    }

    float ylo = 1e9f, yhi = -1e9f;
    for (int i = 0; i < s->vertexCount; i++) { float y = s->mesh[i].y   / (float)POS_ENC + y0; if (y < ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < s->waterCount;  i++) { float y = s->water[i].y  / (float)POS_ENC + y0; if (y < ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < s->leavesCount; i++) { float y = s->leaves[i].y / (float)POS_ENC + y0; if (y < ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < s->noMipCount;  i++) { float y = s->noMip[i].y  / (float)POS_ENC + y0; if (y < ylo) ylo = y; if (y > yhi) yhi = y; }
    if (ylo > yhi) { ylo = (float)y0; yhi = (float)(y0 + SECTION_SY); }
    s->by0 = ylo; s->by1 = yhi;

    float lylo = 1e9f, lyhi = -1e9f;
    for (int i = 0; i < s->leavesCount; i++) { float y = s->leaves[i].y / (float)POS_ENC + y0; if (y < lylo) lylo = y; if (y > lyhi) lyhi = y; }
    if (lylo > lyhi) { lylo = (float)y0; lyhi = (float)y0; }
    s->lby0 = lylo; s->lby1 = lyhi;

    float wylo = 1e9f, wyhi = -1e9f;
    for (int i = 0; i < s->waterCount; i++) { float y = s->water[i].y / (float)POS_ENC + y0; if (y < wylo) wylo = y; if (y > wyhi) wyhi = y; }
    if (wylo > wyhi) { wylo = (float)y0; wyhi = (float)y0; }
    s->wby0 = wylo; s->wby1 = wyhi;

    computeSectionVis(w, ox, y0, oz, s->vis);

    if (g_as.retry) { g_meshOOM = 1; s->dirty = true; }
    else              s->dirty = false;
    g_as.active = false;
}

static bool startCount(World* w) {
    int L = g_as.layer;
    int ox = g_as.c->ox, oz = g_as.c->oz;
    int y0 = g_as.si * SECTION_SY, y1 = y0 + SECTION_SY;

    if (!meEmitStartCapped(w, ox, oz, y0, y1, L, nullptr, 0x7fffffff))
        return false;
    g_as.phase = MePhase::COUNTING;
    g_as.wait = 0;
    g_asyncDispatch++;
    return true;
}

static bool startEmit(World* w) {
    int L = g_as.layer;
    int ox = g_as.c->ox, oz = g_as.c->oz;
    int y0 = g_as.si * SECTION_SY, y1 = y0 + SECTION_SY;
    ChunkVertex* uScratch = (ChunkVertex*)(UNCACHED_USER_MASK | (u32)s_meScratch);
    if (!meEmitStartCapped(w, ox, oz, y0, y1, L, uScratch, ME_SCRATCH_VERTS))
        return false;
    g_as.phase = MePhase::EMITTING;
    g_as.wait = 0;
    g_asyncMeN++;
    return true;
}

void worldAsyncStep(World* w, float camX, float camY, float camZ, float viewDist) {
    g_asyncFrames++;
    g_asyncState = (g_as.active ? 100 : 0)
                 + ((g_as.phase != MePhase::IDLE) ? 10 : 0)
                 + g_as.layer;
    {
        float bd2 = viewDist * viewDist; int dcount = 0;
        for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
            ChunkMesh* c = &w->chunks[i];
            float dx = c->cx - camX, dz = c->cz - camZ;
            if (dx*dx + dz*dz > bd2) continue;
            for (int si = 0; si < N_SECTIONS; si++) if (c->sec[si].dirty) dcount++;
        }
        g_asyncDirty = dcount;
    }

    if (g_as.phase != MePhase::IDLE) {
        if (!meEmitReady()) {

            if (++g_as.wait > 240) {
                meEmitAbort();

                g_as.c->sec[g_as.si].dirty = true;
                for (int i = 0; i < 4; i++) {
                    if (g_as.pend[i]) { free(g_as.pend[i]); g_as.pend[i] = 0; }
                    g_as.pendN[i] = 0;
                }
                g_as.active = false;
                g_as.phase = MePhase::IDLE;
            }
            return;
        }

        if (g_as.phase == MePhase::COUNTING) {
            int count = meEmitFinish(nullptr, 0);
            g_as.phase = MePhase::IDLE;
            if (count <= 0) {

                g_as.pend[g_as.layer] = nullptr;
                g_as.pendN[g_as.layer] = 0;
                g_as.layer++;
            } else if (count <= ME_SCRATCH_VERTS) {

                g_as.meCount = count;
                if (!startEmit(w)) {

                    int ox = g_as.c->ox, oz = g_as.c->oz;
                    int y0 = g_as.si * SECTION_SY, y1 = y0 + SECTION_SY;
                    meshPass(w, ox, oz, y0, y1, s_meScratch, g_as.layer);
                    sceKernelDcacheWritebackInvalidateRange(s_meScratch, (size_t)count * sizeof(ChunkVertex));
                    g_as.pend[g_as.layer] = chunkPack(s_meScratch, count, ox, y0, oz);
                    g_as.pendN[g_as.layer] = g_as.pend[g_as.layer] ? count : 0;
                    g_as.layer++;
                    g_asyncCpuN++;
                }

                return;
            } else {

                g_as.pend[g_as.layer] = nullptr;
                g_as.pendN[g_as.layer] = 0;
                g_as.retry = true;
                g_as.layer++;
                g_asyncMismatch++;
            }

        } else {
            int n = meEmitFinish(s_meScratch, g_as.meCount);
            g_as.phase = MePhase::IDLE;
            if (n == g_as.meCount && n > 0) {
                int ox = g_as.c->ox, oz = g_as.c->oz;
                int y0 = g_as.si * SECTION_SY;
                g_as.pend[g_as.layer] = chunkPack(s_meScratch, n, ox, y0, oz);
                g_as.pendN[g_as.layer] = g_as.pend[g_as.layer] ? n : 0;
                if (!g_as.pend[g_as.layer] && n > 0) g_as.retry = true;
            } else {
                g_as.pend[g_as.layer] = nullptr;
                g_as.pendN[g_as.layer] = 0;
                if (n != 0) g_as.retry = true;
                g_asyncMismatch++;
            }
            g_as.layer++;
        }
    }

    if (!g_as.active) {
        float buildD2 = viewDist * viewDist;
        ChunkMesh* best = nullptr; int bestSi = -1; float bestD = 1e30f;
        for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
            ChunkMesh* c = &w->chunks[i];
            float dx = c->cx - camX, dz = c->cz - camZ;
            if (dx*dx + dz*dz > buildD2) continue;
            for (int si = 0; si < N_SECTIONS; si++) {
                if (!c->sec[si].dirty) continue;
                float dy = (float)(si*SECTION_SY + SECTION_SY/2) - camY;
                float wd = dx*dx + dy*dy*4.0f + dz*dz;
                if (wd < bestD) { bestD = wd; best = c; bestSi = si; }
            }
        }
        if (!best) return;

        g_as.c = best; g_as.si = bestSi; g_as.layer = 0;
        g_as.phase = MePhase::IDLE;
        g_as.meCount = 0; g_as.retry = false; g_as.wait = 0;
        g_as.pend[0] = g_as.pend[1] = g_as.pend[2] = g_as.pend[3] = nullptr;
        g_as.pendN[0] = g_as.pendN[1] = g_as.pendN[2] = g_as.pendN[3] = 0;
        g_as.active = true;
    }

    if (g_as.layer > 3) {
        asyncFinalize(w);
        return;
    }

    if (!startCount(w)) {

        g_as.c->sec[g_as.si].dirty = true;
        g_as.active = false;
    }
}
