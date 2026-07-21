#include "world/level/chunk/chunk.h"
#include "client/renderer/level/frustum.h"
#include <malloc.h>
#include <pspkernel.h>

extern float g_camX, g_camY, g_camZ;
extern int g_fancyGraphics;

extern volatile int g_meshOOM;

unsigned int g_tCount = 0, g_tAlloc = 0, g_tEmit = 0, g_tPack = 0;

#define SCRATCH_VERTS 65536
static ChunkVertex* g_scratch = 0;

#define SCRATCH_VERTS_WL 16384
static ChunkVertex* g_scratchW = 0;
static ChunkVertex* g_scratchL = 0;
static ChunkVertex* g_scratchN = 0;

static bool meshHeapReserveOk() {
    static const unsigned MESH_HEAP_RESERVE = 3u * 1024 * 1024;
    void* p = malloc(MESH_HEAP_RESERVE);
    if (!p) return false;
    free(p);
    return true;
}

static void buildLayer(const World* w, int ox, int oz, int y0, int y1, int layer,
                       DrawVertex** outMesh, int* outCount, bool leavesOpaque, bool leavesCull, bool* oom) {
    if (!meshHeapReserveOk()) { *outMesh = 0; *outCount = 0; *oom = true; return; }
    if (!g_scratch)
        g_scratch = (ChunkVertex*)memalign(16, SCRATCH_VERTS * sizeof(ChunkVertex));

    if (g_scratch) {
        unsigned int t0 = sceKernelGetSystemTimeLow();
        int n = meshPass(w, ox, oz, y0, y1, g_scratch, layer, SCRATCH_VERTS, leavesOpaque, leavesCull);
        unsigned int t1 = sceKernelGetSystemTimeLow(); g_tEmit += t1 - t0;
        if (n >= 0) {
            if (n == 0) { *outMesh = 0; *outCount = 0; return; }
            DrawVertex* d = chunkPack(g_scratch, n, ox, y0, oz);
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
    meshPass(w, ox, oz, y0, y1, m, layer, 0x7fffffff, leavesOpaque, leavesCull);
    DrawVertex* d = chunkPack(m, count, ox, y0, oz);
    free(m);
    if (!d) { *outMesh = 0; *outCount = 0; *oom = true; return; }
    *outMesh = d; *outCount = count;
}

static unsigned short g_visGen = 0;
static unsigned short g_visited[16 * 16 * 16];
void computeSectionVis(const World* w, int ox, int y0, int oz, unsigned char vis[6]) {
    static short stack[16 * 16 * 16];
    #define VIDX(x, y, z) (((x) << 8) | ((y) << 4) | (z))
    for (int i = 0; i < 6; i++) vis[i] = 0;
    if (++g_visGen == 0) {
        for (int i = 0; i < 16 * 16 * 16; i++) g_visited[i] = 0;
        g_visGen = 1;
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

void chunkBuildSection(ChunkMesh* c, const World* w, int si) {
    ChunkSection* s = &c->sec[si];

    if (!meshHeapReserveOk()) { s->dirty = true; return; }

    int y0 = si * SECTION_SY, y1 = y0 + SECTION_SY;
    int ox = c->ox, oz = c->oz;
    s->ox = ox; s->oy = y0; s->oz = oz;

    if (s->mesh)   { free(s->mesh);   s->mesh = 0; }
    if (s->water)  { free(s->water);  s->water = 0; }
    if (s->leaves) { free(s->leaves); s->leaves = 0; }
    if (s->noMip)  { free(s->noMip);  s->noMip = 0; }

    bool leavesOpaque = leafOpaqueBand(c, y0, y1, g_camX, g_camY, g_camZ, g_fancyGraphics != 0);
    bool leavesCull   = leafCullBand(c, y0, y1, g_camX, g_camY, g_camZ, g_fancyGraphics != 0);

    if (!g_scratch)  g_scratch  = (ChunkVertex*)memalign(16, SCRATCH_VERTS    * sizeof(ChunkVertex));
    if (!g_scratchW) g_scratchW = (ChunkVertex*)memalign(16, SCRATCH_VERTS_WL * sizeof(ChunkVertex));
    if (!g_scratchL) g_scratchL = (ChunkVertex*)memalign(16, SCRATCH_VERTS_WL * sizeof(ChunkVertex));
    if (!g_scratchN) g_scratchN = (ChunkVertex*)memalign(16, SCRATCH_VERTS_WL * sizeof(ChunkVertex));

    bool oom = false;
    bool fast = g_scratch && g_scratchW && g_scratchL && g_scratchN;
    if (fast) {
        int n0, n1, n2, n3;
        unsigned int t0 = sceKernelGetSystemTimeLow();
        int rc = meshSection(w, ox, oz, y0, y1, g_scratch, g_scratchW, g_scratchL, g_scratchN,
                             SCRATCH_VERTS, SCRATCH_VERTS_WL, SCRATCH_VERTS_WL, SCRATCH_VERTS_WL,
                             &n0, &n1, &n2, &n3, leavesOpaque, leavesCull);
        unsigned int t1 = sceKernelGetSystemTimeLow(); g_tEmit += t1 - t0;
        if (rc == 0) {
            s->mesh   = n0 ? chunkPack(g_scratch,  n0, ox, y0, oz) : 0; s->vertexCount = s->mesh   ? n0 : 0;
            s->water  = n1 ? chunkPack(g_scratchW, n1, ox, y0, oz) : 0; s->waterCount  = s->water  ? n1 : 0;
            s->leaves = n2 ? chunkPack(g_scratchL, n2, ox, y0, oz) : 0; s->leavesCount = s->leaves ? n2 : 0;
            s->noMip  = n3 ? chunkPack(g_scratchN, n3, ox, y0, oz) : 0; s->noMipCount  = s->noMip  ? n3 : 0;

            oom = (n0 && !s->mesh) || (n1 && !s->water) || (n2 && !s->leaves) || (n3 && !s->noMip);
            g_tPack += sceKernelGetSystemTimeLow() - t1;
        } else {
            fast = false;
        }
    }
    if (!fast) {
        buildLayer(w, ox, oz, y0, y1, 0, &s->mesh,   &s->vertexCount, leavesOpaque, leavesCull, &oom);
        buildLayer(w, ox, oz, y0, y1, 1, &s->water,  &s->waterCount,  leavesOpaque, leavesCull, &oom);
        buildLayer(w, ox, oz, y0, y1, 2, &s->leaves, &s->leavesCount, leavesOpaque, leavesCull, &oom);
        s->noMip = 0; s->noMipCount = 0;
    }
    s->leavesOpaqueBand = leavesOpaque;
    s->leavesCullBand = leavesCull;

    int totalVerts = s->vertexCount + s->waterCount + s->leavesCount + s->noMipCount;
    if (totalVerts == 0) {
        s->by0 = s->by1 = (float)y0;
        s->lby0 = s->lby1 = (float)y0;
        s->wby0 = s->wby1 = (float)y0;
        for (int f = 0; f < 6; f++) s->vis[f] = 0x3F;
        if (oom) { g_meshOOM = 1; s->dirty = true; }
        else       s->dirty = false;
        return;
    }

    float ylo = 1e9f, yhi = -1e9f;
    for (int i = 0; i < s->vertexCount; i++) { float y = s->mesh[i].y   / (float)POS_ENC + y0; if (y <ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < s->waterCount;  i++) { float y = s->water[i].y  / (float)POS_ENC + y0; if (y <ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < s->leavesCount; i++) { float y = s->leaves[i].y / (float)POS_ENC + y0; if (y <ylo) ylo = y; if (y > yhi) yhi = y; }
    for (int i = 0; i < s->noMipCount;  i++) { float y = s->noMip[i].y  / (float)POS_ENC + y0; if (y <ylo) ylo = y; if (y > yhi) yhi = y; }
    if (ylo > yhi) { ylo = (float)y0; yhi = (float)y0; }
    s->by0 = ylo; s->by1 = yhi;

    float lylo = 1e9f, lyhi = -1e9f;
    for (int i = 0; i < s->leavesCount; i++) { float y = s->leaves[i].y / (float)POS_ENC + y0; if (y <lylo) lylo = y; if (y > lyhi) lyhi = y; }
    if (lylo > lyhi) { lylo = (float)y0; lyhi = (float)y0; }
    s->lby0 = lylo; s->lby1 = lyhi;

    float wylo = 1e9f, wyhi = -1e9f;
    for (int i = 0; i < s->waterCount; i++) { float y = s->water[i].y / (float)POS_ENC + y0; if (y <wylo) wylo = y; if (y > wyhi) wyhi = y; }
    if (wylo > wyhi) { wylo = (float)y0; wyhi = (float)y0; }
    s->wby0 = wylo; s->wby1 = wyhi;

    static const float VIS_DIST = 40.0f;
    float sdx = (ox + CHUNK_SX * 0.5f) - g_camX, sdz = (oz + CHUNK_SZ * 0.5f) - g_camZ;
    if (sdx * sdx + sdz * sdz <= VIS_DIST * VIS_DIST)
        computeSectionVis(w, ox, y0, oz, s->vis);
    else
        for (int f = 0; f < 6; f++) s->vis[f] = 0x3F;

    if (oom) { g_meshOOM = 1; s->dirty = true; }
    else       s->dirty = false;
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
