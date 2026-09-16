#include "world/level/chunk/chunk.h"
#include "gpu/gu.h"
#include "world/level/chunk/mesh_sink.h"
#include "client/renderer/level/frustum.h"
#include <malloc.h>
#include <pspkernel.h>
#include "util/prof.h"

extern float g_camX, g_camY, g_camZ;
extern int g_fancyGraphics;

extern volatile int g_meshOOM;

#define MESH_PROFILE 0

#if MESH_PROFILE
unsigned int g_tCount = 0, g_tAlloc = 0, g_tEmit = 0, g_tPack = 0;
#endif

extern int g_lowMemHeap;

unsigned int g_meshFallbacks = 0;
static int scratchVerts()   { return g_lowMemHeap ? 24576 : 65536; }
static int scratchVertsWL() { return g_lowMemHeap ?  6144 : 16384; }
#define SCRATCH_VERTS    scratchVerts()
#define SCRATCH_VERTS_WL scratchVertsWL()

static ChunkVertex* g_scratch = 0;

#define TILE_VERTS(L) ((L) == 0 ? 512 : 128)
static ChunkVertex* g_tile[4] = { 0, 0, 0, 0 };

static DrawVertex* g_stage[4] = { 0, 0, 0, 0 };
static int stageVerts(int L) { return L == 0 ? SCRATCH_VERTS : SCRATCH_VERTS_WL; }

static bool tileAlloc() {
    if (!g_tile[0]) {
        size_t verts = 0;
        for (int L = 0; L < 4; L++) verts += (size_t)TILE_VERTS(L);
        ChunkVertex* base = (ChunkVertex*)memalign(16, verts * sizeof(ChunkVertex));
        if (!base) return false;
        size_t at = 0;
        for (int L = 0; L < 4; L++) { g_tile[L] = base + at; at += (size_t)TILE_VERTS(L); }
    }
    if (!g_stage[0]) {
        size_t verts = 0;
        for (int L = 0; L < 4; L++) verts += (size_t)stageVerts(L);
        DrawVertex* base = (DrawVertex*)memalign(16, verts * sizeof(DrawVertex));
        if (!base) { return false; }
        size_t at = 0;
        for (int L = 0; L < 4; L++) { g_stage[L] = base + at; at += (size_t)stageVerts(L); }
    }
    return true;
}

struct TileCtx {
    int ox, oy, oz;
    int* qlo;
    int* qhi;
};

static bool tileFlush(MeshSink* sk, int layer) {
    TileCtx* tc = (TileCtx*)sk->ctx;
    int n = sk->n[layer];
    if (!n) return true;

    if (sk->total[layer] + n > stageVerts(layer)) return false;
    chunkPackInto(g_stage[layer] + sk->total[layer], sk->buf[layer], n,
                  tc->ox, tc->oy, tc->oz, tc->qlo + layer, tc->qhi + layer);
    sk->total[layer] += n;
    sk->n[layer] = 0;
    return true;
}

static bool g_heapOk = true;

static bool meshHeapReserveProbe() {
    unsigned MESH_HEAP_RESERVE = g_lowMemHeap ? (1u * 1024 * 1024) : (3u * 1024 * 1024);
    void* p = malloc(MESH_HEAP_RESERVE);
    if (!p) return false;
    free(p);
    return true;
}

void chunkMeshHeapProbe() { g_heapOk = meshHeapReserveProbe(); }

static inline bool meshHeapReserveOk() { return g_heapOk; }

static void buildLayer(const World* w, int ox, int oz, int y0, int y1, int layer,
                       DrawVertex** outMesh, int* outCount, bool leavesOpaque, bool leavesCull, bool* oom,
                       int* lavaStart = 0, float* ylo = 0, float* yhi = 0) {
    if (lavaStart) *lavaStart = 0;
    if (!meshHeapReserveOk()) { *outMesh = 0; *outCount = 0; *oom = true; return; }
    if (!g_scratch)
        g_scratch = (ChunkVertex*)memalign(16, SCRATCH_VERTS * sizeof(ChunkVertex));

    if (g_scratch) {
#if MESH_PROFILE
        unsigned int t0 = sceKernelGetSystemTimeLow();
#endif
        int n = meshPass(w, ox, oz, y0, y1, g_scratch, layer, SCRATCH_VERTS, leavesOpaque, leavesCull, lavaStart);
#if MESH_PROFILE
        unsigned int t1 = sceKernelGetSystemTimeLow(); g_tEmit += t1 - t0;
#endif
        if (n >= 0) {
            if (n == 0) { *outMesh = 0; *outCount = 0; return; }
            DrawVertex* d = chunkPack(g_scratch, n, ox, y0, oz, ylo, yhi);
#if MESH_PROFILE
            g_tPack += sceKernelGetSystemTimeLow() - t1;
#endif
            if (!d) { *outMesh = 0; *outCount = 0; *oom = true; return; }
            *outMesh = d; *outCount = n; return;
        }

    }

#if MESH_PROFILE
    unsigned int s0 = sceKernelGetSystemTimeLow();
#endif
    int count = meshPass(w, ox, oz, y0, y1, 0, layer, 0x7fffffff, leavesOpaque, leavesCull);
#if MESH_PROFILE
    g_tCount += sceKernelGetSystemTimeLow() - s0;
#endif
    if (count == 0) { *outMesh = 0; *outCount = 0; return; }
    ChunkVertex* m = (ChunkVertex*)memalign(16, count * sizeof(ChunkVertex));
    if (!m) { *outMesh = 0; *outCount = 0; *oom = true; return; }

    int emitted = meshPass(w, ox, oz, y0, y1, m, layer, 0x7fffffff, leavesOpaque, leavesCull, lavaStart);
    if (emitted <= 0) { free(m); *outMesh = 0; *outCount = 0; return; }
    DrawVertex* d = chunkPack(m, emitted, ox, y0, oz, ylo, yhi);
    free(m);
    if (!d) { *outMesh = 0; *outCount = 0; *oom = true; return; }
    *outMesh = d; *outCount = emitted;
}

void chunkBuildSection(ChunkMesh* c, const World* w, int si) {

    struct EdgeScope {
        const unsigned char* prev;
        EdgeScope() : prev(g_edgeColumn) { if (!prev) g_edgeColumn = g_seaColumn; }
        ~EdgeScope() { g_edgeColumn = prev; }
    } edgeScope;
    if (g_onEdgeSectionBuilt) {
        const int cx = c->ox >> 4, cz = c->oz >> 4;
        if (worldChunkInBounds(cx, cz) &&
            (cx == 0 || cz == 0 || cx == WORLD_CHUNKS_X - 1 || cz == WORLD_CHUNKS_Z - 1))
            g_onEdgeSectionBuilt(cx, cz, si);
    }
    ChunkSection* s = &c->sec[si];

    if (!meshHeapReserveOk()) { s->dirty = true; return; }
    s->gen++;

    int y0 = si * SECTION_SY, y1 = y0 + SECTION_SY;
    int ox = c->ox, oz = c->oz;
    s->ox = ox; s->oy = y0; s->oz = oz;

    if (sectionCannotEmit(w, ox, oz, si)) {
        if (s->mesh)   { guDeferFree(s->mesh);   s->mesh = 0; }
        if (s->water)  { guDeferFree(s->water);  s->water = 0; }
        if (s->leaves) { guDeferFree(s->leaves); s->leaves = 0; }
        if (s->noMip)  { guDeferFree(s->noMip);  s->noMip = 0; }
        s->vertexCount = s->waterCount = s->leavesCount = s->noMipCount = 0;
        s->noMipLavaStart = 0;
        s->skyLit = false;
        s->dirty = false;

        s->by0 = s->by1 = s->lby0 = s->lby1 = s->wby0 = s->wby1 = (float)y0;
        return;
    }

    s->skyLit = false;
    {
        int sy0 = y0 - 1, sy1 = y1;
        if (sy0 < 0) sy0 = 0;
        if (sy1 > WORLD_H - 1) sy1 = WORLD_H - 1;

        for (int yy = sy0; yy <= sy1 && !s->skyLit; yy++)
            if (!lightPlaneAllDark(w, 0, ox, yy, oz)) s->skyLit = true;

        for (int gx = ox - 1; gx <= ox + CHUNK_SX && !s->skyLit; gx++) {
            for (int gz = oz - 1; gz <= oz + CHUNK_SZ && !s->skyLit; gz++) {

                if (!worldReady(w, gx, gz)) continue;
                if (gx >= ox && gx < ox + CHUNK_SX &&
                    gz >= oz && gz < oz + CHUNK_SZ) continue;
                for (int yy = sy0; yy <= sy1; yy++)
                    if (lightSkyGet(w, gx, yy, gz)) { s->skyLit = true; break; }
            }
        }
    }

    if (s->mesh)   { guDeferFree(s->mesh);   s->mesh = 0; }
    if (s->water)  { guDeferFree(s->water);  s->water = 0; }
    if (s->leaves) { guDeferFree(s->leaves); s->leaves = 0; }
    if (s->noMip)  { guDeferFree(s->noMip);  s->noMip = 0; }
    s->noMipLavaStart = 0;

    bool leavesOpaque = leafOpaqueBand(c, y0, y1, g_camX, g_camY, g_camZ, g_fancyGraphics != 0);
    bool leavesCull   = leafCullBand(c, y0, y1, g_camX, g_camY, g_camZ, g_fancyGraphics != 0);

    if (!tileAlloc()) {

    }

    bool oom = false;

    int qlo[4] = { 32767, 32767, 32767, 32767 }, qhi[4] = { -32768, -32768, -32768, -32768 };
    float plo[4] = { 1e9f, 1e9f, 1e9f, 1e9f }, phi[4] = { -1e9f, -1e9f, -1e9f, -1e9f };
    bool fast = g_tile[0] && g_tile[1] && g_tile[2] && g_tile[3] &&
                g_stage[0] && g_stage[1] && g_stage[2] && g_stage[3];
    if (fast) {
        int nLava;
        TileCtx tc;
        tc.ox = ox; tc.oy = y0; tc.oz = oz; tc.qlo = qlo; tc.qhi = qhi;
        MeshSink sk;
        for (int L = 0; L < 4; L++) {
            sk.buf[L]   = g_tile[L];
            sk.cap[L]   = TILE_VERTS(L);
            sk.n[L]     = 0;
            sk.total[L] = 0;
        }
        sk.flush = tileFlush;
        sk.ctx   = &tc;
#if MESH_PROFILE
        unsigned int t0 = sceKernelGetSystemTimeLow();
#endif

        profBegin(PROF_MEMIT);
        int rc = meshSectionSink(w, ox, oz, y0, y1, &sk, &nLava, leavesOpaque, leavesCull);
        profEnd(PROF_MEMIT);
#if MESH_PROFILE
        unsigned int t1 = sceKernelGetSystemTimeLow(); g_tEmit += t1 - t0;
#endif
        if (rc == 0) {
            int n0 = sinkCount(&sk, 0), n1 = sinkCount(&sk, 1);
            int n2 = sinkCount(&sk, 2), n3 = sinkCount(&sk, 3);
            profBegin(PROF_MPACK);
            s->mesh   = n0 ? chunkPackFinish(g_stage[0], n0) : 0; s->vertexCount = s->mesh   ? n0 : 0;
            s->water  = n1 ? chunkPackFinish(g_stage[1], n1) : 0; s->waterCount  = s->water  ? n1 : 0;
            s->leaves = n2 ? chunkPackFinish(g_stage[2], n2) : 0; s->leavesCount = s->leaves ? n2 : 0;
            s->noMip  = n3 ? chunkPackFinish(g_stage[3], n3) : 0; s->noMipCount  = s->noMip  ? n3 : 0;
            s->noMipLavaStart = s->noMip ? nLava : 0;
            profEnd(PROF_MPACK);
            for (int L = 0; L < 4; L++)
                if (qhi[L] >= qlo[L]) { plo[L] = chunkPackDecodeY(qlo[L], y0); phi[L] = chunkPackDecodeY(qhi[L], y0); }

            oom = (n0 && !s->mesh) || (n1 && !s->water) || (n2 && !s->leaves) || (n3 && !s->noMip);
#if MESH_PROFILE
            g_tPack += sceKernelGetSystemTimeLow() - t1;
#endif
        } else {
            fast = false;
            g_meshFallbacks++;
        }
    }
    if (!fast) {
        buildLayer(w, ox, oz, y0, y1, 0, &s->mesh,   &s->vertexCount, leavesOpaque, leavesCull, &oom, 0, plo+0, phi+0);
        buildLayer(w, ox, oz, y0, y1, 1, &s->water,  &s->waterCount,  leavesOpaque, leavesCull, &oom, 0, plo+1, phi+1);
        buildLayer(w, ox, oz, y0, y1, 2, &s->leaves, &s->leavesCount, leavesOpaque, leavesCull, &oom, 0, plo+2, phi+2);

        buildLayer(w, ox, oz, y0, y1, 3, &s->noMip,  &s->noMipCount,  leavesOpaque, leavesCull, &oom,
                   &s->noMipLavaStart, plo+3, phi+3);
    }
    s->leavesOpaqueBand = leavesOpaque;
    s->leavesCullBand = leavesCull;

    int totalVerts = s->vertexCount + s->waterCount + s->leavesCount + s->noMipCount;
    if (totalVerts == 0) {
        s->by0 = s->by1 = (float)y0;
        s->lby0 = s->lby1 = (float)y0;
        s->wby0 = s->wby1 = (float)y0;
        if (oom) { g_meshOOM = 1; s->dirty = true; }
        else       s->dirty = false;
        return;
    }

    float ylo = plo[0], yhi = phi[0];
    for (int k = 1; k < 4; k++) { if (plo[k] < ylo) ylo = plo[k]; if (phi[k] > yhi) yhi = phi[k]; }
    if (ylo > yhi) { ylo = (float)y0; yhi = (float)y0; }
    s->by0 = ylo; s->by1 = yhi;

    float lylo = plo[2], lyhi = phi[2];
    if (lylo > lyhi) { lylo = (float)y0; lyhi = (float)y0; }
    s->lby0 = lylo; s->lby1 = lyhi;

    float wylo = plo[1], wyhi = phi[1];
    if (wylo > wyhi) { wylo = (float)y0; wyhi = (float)y0; }
    s->wby0 = wylo; s->wby1 = wyhi;

    if (oom) { g_meshOOM = 1; s->dirty = true; }
    else       s->dirty = false;
}

void chunkBuildMesh(ChunkMesh* c, const World* w, int ox, int oz) {
    chunkMeshHeapProbe();
    c->ox = ox; c->oz = oz;
    c->cx = ox + CHUNK_SX * 0.5f;
    c->cz = oz + CHUNK_SZ * 0.5f;
    for (int si = 0; si < N_SECTIONS; si++) {

        chunkBuildSection(c, w, si);
    }
}

void chunkInitLazy(ChunkMesh* c, int ox, int oz) {
    c->ox = ox; c->oz = oz;
    c->cx = ox + CHUNK_SX * 0.5f;
    c->cz = oz + CHUNK_SZ * 0.5f;
    for (int si = 0; si < N_SECTIONS; si++) {
        ChunkSection* s = &c->sec[si];
        s->by0 = s->lby0 = s->wby0 = (float)(si * SECTION_SY);
        s->by1 = s->lby1 = s->wby1 = s->by0;
        s->dirty = true;
    }
}
