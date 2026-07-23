#include "world/level/chunk/chunk.h"
#include "client/renderer/level/frustum.h"
#include <malloc.h>
#include <pspkernel.h>

extern float g_camX, g_camY, g_camZ;
extern int g_fancyGraphics;

extern volatile int g_meshOOM;

unsigned int g_tCount = 0, g_tAlloc = 0, g_tEmit = 0, g_tPack = 0;

#define SCRATCH_VERTS 65536

#define SCRATCH_VERTS_WL 16384

struct MeshScratch {
    ChunkVertex*   o;
    ChunkVertex*   w;
    ChunkVertex*   l;
    ChunkVertex*   n;

    int            capO;
    int            capWL;

    unsigned short visGen;
    unsigned short visited[16 * 16 * 16];
    short          stack[16 * 16 * 16];
};

MeshScratch* meshScratchCreate(int capOpaque, int capWaterLeaves) {
    MeshScratch* s = (MeshScratch*)malloc(sizeof(MeshScratch));
    if (!s) return 0;
    s->capO  = capOpaque;
    s->capWL = capWaterLeaves;
    s->o = (ChunkVertex*)memalign(16, (size_t)capOpaque      * sizeof(ChunkVertex));
    s->w = (ChunkVertex*)memalign(16, (size_t)capWaterLeaves * sizeof(ChunkVertex));
    s->l = (ChunkVertex*)memalign(16, (size_t)capWaterLeaves * sizeof(ChunkVertex));
    s->n = (ChunkVertex*)memalign(16, (size_t)capWaterLeaves * sizeof(ChunkVertex));
    s->visGen = 0;
    for (int i = 0; i < 16 * 16 * 16; i++) s->visited[i] = 0;

    if (!s->o && !s->w && !s->l && !s->n) { free(s); return 0; }
    return s;
}

static MeshScratch* g_mainScratch = 0;
static MeshScratch* mainScratch() {
    if (!g_mainScratch) g_mainScratch = meshScratchCreate(SCRATCH_VERTS, SCRATCH_VERTS_WL);
    return g_mainScratch;
}

static bool meshHeapReserveOk() {
    static const unsigned MESH_HEAP_RESERVE = 3u * 1024 * 1024;
    void* p = malloc(MESH_HEAP_RESERVE);
    if (!p) return false;
    free(p);
    return true;
}

static void buildLayer(const World* w, int ox, int oz, int y0, int y1, int layer,
                       DrawVertex** outMesh, int* outCount, bool leavesOpaque, bool leavesCull, bool* oom,
                       MeshScratch* sc) {
    if (!meshHeapReserveOk()) { *outMesh = 0; *outCount = 0; *oom = true; return; }

    if (sc && sc->o) {
        unsigned int t0 = sceKernelGetSystemTimeLow();
        int n = meshPass(w, ox, oz, y0, y1, sc->o, layer, sc->capO, leavesOpaque, leavesCull);
        unsigned int t1 = sceKernelGetSystemTimeLow(); g_tEmit += t1 - t0;
        if (n >= 0) {
            if (n == 0) { *outMesh = 0; *outCount = 0; return; }
            DrawVertex* d = chunkPack(sc->o, n, ox, y0, oz);
            unsigned int t2 = sceKernelGetSystemTimeLow(); g_tPack += t2 - t1;
            if (!d) { *outMesh = 0; *outCount = 0; *oom = true; return; }
            *outMesh = d; *outCount = n; return;
        }

    }

    unsigned int s0 = sceKernelGetSystemTimeLow();
    int count = meshPass(w, ox, oz, y0, y1, 0, layer, 0x7fffffff, leavesOpaque, leavesCull);
    unsigned int s1 = sceKernelGetSystemTimeLow(); g_tCount += s1 - s0;
    if (count == 0) { *outMesh = 0; *outCount = 0; return; }
    ChunkVertex* m = (ChunkVertex*)memalign(16, count * sizeof(ChunkVertex));
    if (!m) { *outMesh = 0; *outCount = 0; *oom = true; return; }

    int emitted = meshPass(w, ox, oz, y0, y1, m, layer, count, leavesOpaque, leavesCull);
    if (emitted < 0) { free(m); *outMesh = 0; *outCount = 0; *oom = true; return; }
    DrawVertex* d = chunkPack(m, emitted, ox, y0, oz);
    free(m);
    if (!d) { *outMesh = 0; *outCount = 0; *oom = true; return; }
    *outMesh = d; *outCount = emitted;
}

static void computeSectionVisScratch(const World* w, int ox, int y0, int oz,
                                     unsigned char vis[6], MeshScratch* sc) {
    unsigned short* g_visited = sc->visited;
    short* stack = sc->stack;
    #define VIDX(x, y, z) (((x) << 8) | ((y) << 4) | (z))
    for (int i = 0; i < 6; i++) vis[i] = 0;
    unsigned short g_visGen = ++sc->visGen;
    if (g_visGen == 0) {
        for (int i = 0; i < 16 * 16 * 16; i++) g_visited[i] = 0;
        g_visGen = sc->visGen = 1;
    }

    for (int sx = 0; sx < 16; sx++)
    for (int sy = 0; sy < 16; sy++)
    for (int sz = 0; sz < 16; sz++) {
        int start = VIDX(sx, sy, sz);
        if (g_visited[start] == g_visGen) continue;
        g_visited[start] = g_visGen;
        if (isOpaque(worldBlock(w, ox + sx, y0 + sy, oz + sz))) continue;

        unsigned char faces = 0;
        int sp = 0; stack[sp++] = start;
        while (sp > 0) {
            int cur = stack[--sp];
            int cx = (cur >> 8) & 15, cy = (cur >> 4) & 15, cz = cur & 15;
            if (cx == 0) faces |= 1 << 0; if (cx == 15) faces |= 1 << 1;
            if (cy == 0) faces |= 1 << 2; if (cy == 15) faces |= 1 << 3;
            if (cz == 0) faces |= 1 << 4; if (cz == 15) faces |= 1 << 5;
            static const int nb[6][3] = {{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
            for (int d = 0; d < 6; d++) {
                int nx = cx + nb[d][0], ny = cy + nb[d][1], nz = cz + nb[d][2];
                if (nx < 0 || nx > 15 || ny < 0 || ny > 15 || nz < 0 || nz > 15) continue;
                int ni = VIDX(nx, ny, nz);
                if (g_visited[ni] == g_visGen) continue;
                g_visited[ni] = g_visGen;
                if (isOpaque(worldBlock(w, ox + nx, y0 + ny, oz + nz))) continue;
                stack[sp++] = ni;
            }
        }

        for (int a = 0; a < 6; a++) if (faces & (1 << a)) vis[a] |= faces;
    }
    #undef VIDX
}

void computeSectionVis(const World* w, int ox, int y0, int oz, unsigned char vis[6]) {
    MeshScratch* sc = mainScratch();
    if (!sc) { for (int f = 0; f < 6; f++) vis[f] = 0x3F; return; }
    computeSectionVisScratch(w, ox, y0, oz, vis, sc);
}

void sectionMeshResultFree(SectionMeshResult* r) {
    if (r->mesh)   { free(r->mesh);   r->mesh = 0; }
    if (r->water)  { free(r->water);  r->water = 0; }
    if (r->leaves) { free(r->leaves); r->leaves = 0; }
    if (r->noMip)  { free(r->noMip);  r->noMip = 0; }
    r->vertexCount = r->waterCount = r->leavesCount = r->noMipCount = 0;
}

void chunkComputeSection(const ChunkMesh* c, const World* w, int si,
                         MeshScratch* sc, SectionMeshResult* out) {
    out->mesh = out->water = out->leaves = out->noMip = 0;
    out->vertexCount = out->waterCount = out->leavesCount = out->noMipCount = 0;
    out->oom = false;

    out->leavesOpaqueBand = false;
    out->leavesCullBand   = false;

    int y0 = si * SECTION_SY, y1 = y0 + SECTION_SY;
    int ox = c->ox, oz = c->oz;

    out->skyLit = false;
    {
        int sy0 = y0 - 1, sy1 = y1;
        if (sy0 < 0) sy0 = 0;
        if (sy1 > WORLD_H - 1) sy1 = WORLD_H - 1;
        for (int gx = ox - 1; gx <= ox + CHUNK_SX && !out->skyLit; gx++) {
            if (gx < 0 || gx >= WORLD_W) continue;
            for (int gz = oz - 1; gz <= oz + CHUNK_SZ && !out->skyLit; gz++) {
                if (gz < 0 || gz >= WORLD_D) continue;
                const unsigned char* col = w->light + worldIndex(gx, 0, gz);
                for (int yy = sy0; yy <= sy1; yy++)
                    if (col[yy] >> 4) { out->skyLit = true; break; }
            }
        }
    }

    if (!meshHeapReserveOk()) { out->oom = true; goto bounds; }

    {

    bool leavesOpaque = leafOpaqueBand(c, y0, y1, g_camX, g_camY, g_camZ, g_fancyGraphics != 0);
    bool leavesCull   = leafCullBand(c, y0, y1, g_camX, g_camY, g_camZ, g_fancyGraphics != 0);
    out->leavesOpaqueBand = leavesOpaque;
    out->leavesCullBand   = leavesCull;

    bool fast = sc && sc->o && sc->w && sc->l && sc->n;
    if (fast) {
        int n0, n1, n2, n3;
        unsigned int t0 = sceKernelGetSystemTimeLow();
        int rc = meshSection(w, ox, oz, y0, y1, sc->o, sc->w, sc->l, sc->n,
                             sc->capO, sc->capWL, sc->capWL, sc->capWL,
                             &n0, &n1, &n2, &n3, leavesOpaque, leavesCull);
        unsigned int t1 = sceKernelGetSystemTimeLow(); g_tEmit += t1 - t0;
        if (rc == 0) {
            out->mesh   = n0 ? chunkPack(sc->o, n0, ox, y0, oz) : 0; out->vertexCount = out->mesh   ? n0 : 0;
            out->water  = n1 ? chunkPack(sc->w, n1, ox, y0, oz) : 0; out->waterCount  = out->water  ? n1 : 0;
            out->leaves = n2 ? chunkPack(sc->l, n2, ox, y0, oz) : 0; out->leavesCount = out->leaves ? n2 : 0;
            out->noMip  = n3 ? chunkPack(sc->n, n3, ox, y0, oz) : 0; out->noMipCount  = out->noMip  ? n3 : 0;

            out->oom = (n0 && !out->mesh) || (n1 && !out->water) || (n2 && !out->leaves) || (n3 && !out->noMip);
            g_tPack += sceKernelGetSystemTimeLow() - t1;
        } else {
            fast = false;
        }
    }
    if (!fast) {
        buildLayer(w, ox, oz, y0, y1, 0, &out->mesh,   &out->vertexCount, leavesOpaque, leavesCull, &out->oom, sc);
        buildLayer(w, ox, oz, y0, y1, 1, &out->water,  &out->waterCount,  leavesOpaque, leavesCull, &out->oom, sc);
        buildLayer(w, ox, oz, y0, y1, 2, &out->leaves, &out->leavesCount, leavesOpaque, leavesCull, &out->oom, sc);
        out->noMip = 0; out->noMipCount = 0;
    }
    }

bounds:

    {
    int totalVerts = out->vertexCount + out->waterCount + out->leavesCount + out->noMipCount;
    if (totalVerts == 0) {
        out->by0 = out->by1 = (float)y0;
        out->lby0 = out->lby1 = (float)y0;
        out->wby0 = out->wby1 = (float)y0;

        bool anyOpaque = false;
        for (int sx = 0; sx < 16 && !anyOpaque; sx++)
            for (int sy = 0; sy < 16 && !anyOpaque; sy++)
                for (int sz = 0; sz < 16; sz++)
                    if (isOpaque(worldBlock(w, ox + sx, y0 + sy, oz + sz))) { anyOpaque = true; break; }
        if (anyOpaque && sc) computeSectionVisScratch(w, ox, y0, oz, out->vis, sc);
        else for (int f = 0; f < 6; f++) out->vis[f] = 0x3F;
        return;
    }

    float ylo = 1e9f, yhi = -1e9f;
    for (int i = 0; i < out->vertexCount; i++) { float y = out->mesh[i].y   / (float)POS_ENC + y0; if (y <ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < out->waterCount;  i++) { float y = out->water[i].y  / (float)POS_ENC + y0; if (y <ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < out->leavesCount; i++) { float y = out->leaves[i].y / (float)POS_ENC + y0; if (y <ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < out->noMipCount;  i++) { float y = out->noMip[i].y  / (float)POS_ENC + y0; if (y <ylo) ylo = y; if (y > yhi) yhi = y; }
    if (ylo > yhi) { ylo = (float)y0; yhi = (float)y0; }
    out->by0 = ylo; out->by1 = yhi;

    float lylo = 1e9f, lyhi = -1e9f;
    for (int i = 0; i < out->leavesCount; i++) { float y = out->leaves[i].y / (float)POS_ENC + y0; if (y <lylo) lylo = y; if (y > lyhi) lyhi = y; }
    if (lylo > lyhi) { lylo = (float)y0; lyhi = (float)y0; }
    out->lby0 = lylo; out->lby1 = lyhi;

    float wylo = 1e9f, wyhi = -1e9f;
    for (int i = 0; i < out->waterCount; i++) { float y = out->water[i].y / (float)POS_ENC + y0; if (y <wylo) wylo = y; if (y > wyhi) wyhi = y; }
    if (wylo > wyhi) { wylo = (float)y0; wyhi = (float)y0; }
    out->wby0 = wylo; out->wby1 = wyhi;

    if (sc) computeSectionVisScratch(w, ox, y0, oz, out->vis, sc);
    else    for (int f = 0; f < 6; f++) out->vis[f] = 0x3F;
    }
}

void chunkApplySection(ChunkMesh* c, int si, const SectionMeshResult* r) {
    ChunkSection* s = &c->sec[si];

    if (r->oom && !r->mesh && !r->water && !r->leaves && !r->noMip) {
        g_meshOOM = 1;
        s->dirty = true;
        return;
    }
    int y0 = si * SECTION_SY;
    s->ox = c->ox; s->oy = y0; s->oz = c->oz;

    if (s->mesh)   free(s->mesh);
    if (s->water)  free(s->water);
    if (s->leaves) free(s->leaves);
    if (s->noMip)  free(s->noMip);

    s->mesh   = r->mesh;   s->vertexCount = r->vertexCount;
    s->water  = r->water;  s->waterCount  = r->waterCount;
    s->leaves = r->leaves; s->leavesCount = r->leavesCount;
    s->noMip  = r->noMip;  s->noMipCount  = r->noMipCount;

    s->by0 = r->by0; s->by1 = r->by1;
    s->lby0 = r->lby0; s->lby1 = r->lby1;
    s->wby0 = r->wby0; s->wby1 = r->wby1;
    for (int f = 0; f < 6; f++) s->vis[f] = r->vis[f];
    s->leavesOpaqueBand = r->leavesOpaqueBand;
    s->leavesCullBand   = r->leavesCullBand;
    s->skyLit           = r->skyLit;

    if (r->oom) { g_meshOOM = 1; s->dirty = true; }
    else          s->dirty = false;
}

void chunkBuildSection(ChunkMesh* c, const World* w, int si) {

    c->sec[si].meshSeq++;

    MeshScratch* sc = mainScratch();
    SectionMeshResult r;
    chunkComputeSection(c, w, si, sc, &r);
    chunkApplySection(c, si, &r);
}

void chunkBuildMesh(ChunkMesh* c, const World* w, int ox, int oz) {
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

        for (int f = 0; f < 6; f++) s->vis[f] = 0x3F;
        s->dirty = true;
    }
}
