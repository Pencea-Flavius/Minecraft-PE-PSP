#include "client/renderer/level/mesh_async.h"
#include "world/level/chunk/chunk.h"
#include "platform/me/me.h"
#include <malloc.h>
#include <pspkernel.h>

struct AsyncSlot {
    ChunkMesh* c;  int si;  int layer;
    ChunkVertex* buf; int count; int wait;
    ChunkVertex* pend[3]; int pendN[3];
    bool active;
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

    s->mesh   = g_as.pendN[0] ? chunkPack(g_as.pend[0], g_as.pendN[0], ox, y0, oz) : 0;
    s->water  = g_as.pendN[1] ? chunkPack(g_as.pend[1], g_as.pendN[1], ox, y0, oz) : 0;
    s->leaves = g_as.pendN[2] ? chunkPack(g_as.pend[2], g_as.pendN[2], ox, y0, oz) : 0;
    if (g_as.pend[0]) free(g_as.pend[0]);
    if (g_as.pend[1]) free(g_as.pend[1]);
    if (g_as.pend[2]) free(g_as.pend[2]);
    s->vertexCount = s->mesh   ? g_as.pendN[0] : 0;
    s->waterCount  = s->water  ? g_as.pendN[1] : 0;
    s->leavesCount = s->leaves ? g_as.pendN[2] : 0;

    bool oom = (g_as.pendN[0] && !s->mesh) || (g_as.pendN[1] && !s->water) || (g_as.pendN[2] && !s->leaves);

    float ylo=1e9f, yhi=-1e9f;
    for (int i=0;i<s->vertexCount;i++){float y=s->mesh[i].y  /(float)POS_ENC+y0; if(y<ylo)ylo=y; if(y>yhi)yhi=y;}
    for (int i=0;i<s->waterCount; i++){float y=s->water[i].y /(float)POS_ENC+y0; if(y<ylo)ylo=y; if(y>yhi)yhi=y;}
    for (int i=0;i<s->leavesCount;i++){float y=s->leaves[i].y/(float)POS_ENC+y0; if(y<ylo)ylo=y; if(y>yhi)yhi=y;}
    if (ylo>yhi){ylo=(float)y0;yhi=(float)y0;} s->by0=ylo; s->by1=yhi;
    float lylo=1e9f,lyhi=-1e9f;
    for (int i=0;i<s->leavesCount;i++){float y=s->leaves[i].y/(float)POS_ENC;if(y<lylo)lylo=y; if(y>lyhi)lyhi=y;}
    if (lylo>lyhi){lylo=(float)y0;lyhi=(float)y0;} s->lby0=lylo; s->lby1=lyhi;
    float wylo=1e9f,wyhi=-1e9f;
    for (int i=0;i<s->waterCount;i++){float y=s->water[i].y/(float)POS_ENC;if(y<wylo)wylo=y; if(y>wyhi)wyhi=y;}
    if (wylo>wyhi){wylo=(float)y0;wyhi=(float)y0;} s->wby0=wylo; s->wby1=wyhi;

    computeSectionVis(w, ox, y0, oz, s->vis);

    if (oom) { g_meshOOM = 1; s->dirty = true; }
    else       s->dirty = false;
    g_as.active = false;
}

static bool asyncStartLayer(World* w) {
    ChunkMesh* c = g_as.c; int L = g_as.layer;
    int ox = c->ox, oz = c->oz;
    int y0 = g_as.si * SECTION_SY, y1 = y0 + SECTION_SY;

    int cnt = meMeshCount(w, ox, oz, y0, y1, L);
    if (cnt < 0) {
        int c2 = meshPass(w, ox, oz, y0, y1, 0, L);
        if (c2 <= 0) { g_as.pend[L]=0; g_as.pendN[L]=0; return false; }
        ChunkVertex* m2 = (ChunkVertex*)memalign(16, (size_t)c2 * sizeof(ChunkVertex));
        if (!m2) { g_as.pend[L]=0; g_as.pendN[L]=0; return false; }
        meshPass(w, ox, oz, y0, y1, m2, L);
        sceKernelDcacheWritebackInvalidateRange(m2, (size_t)c2 * sizeof(ChunkVertex));
        g_as.pend[L] = m2; g_as.pendN[L] = c2; g_asyncCpuN++;
        return false;
    }
    if (cnt == 0) { g_as.pend[L]=0; g_as.pendN[L]=0; return false; }

    ChunkVertex* m = (ChunkVertex*)memalign(16, (size_t)cnt * sizeof(ChunkVertex));
    if (!m) { g_as.pend[L]=0; g_as.pendN[L]=0; return false; }

    if (meEmitStart(w, ox, oz, y0, y1, L, m)) {
        g_as.buf = m; g_as.count = cnt; g_as.wait = 0;
        g_asyncMeN++; g_asyncDispatch++;
        return true;
    }
    meshPass(w, ox, oz, y0, y1, m, L);
    sceKernelDcacheWritebackInvalidateRange(m, (size_t)cnt * sizeof(ChunkVertex));
    g_as.pend[L] = m; g_as.pendN[L] = cnt;
    g_asyncCpuN++;
    return false;
}

void worldAsyncStep(World* w, float camX, float camY, float camZ, float viewDist) {
    g_asyncFrames++;
    g_asyncState = (g_as.active ? 100 : 0) + (g_as.buf ? 10 : 0) + g_as.layer;
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

    if (g_as.buf) {
        if (!meEmitReady()) {
            if (++g_as.wait > 240) {
                meEmitAbort();
                free(g_as.buf); g_as.buf = 0;
                g_as.c->sec[g_as.si].dirty = true; g_as.active = false;
            }
            return;
        }
        int n = meEmitFinish(g_as.buf, g_as.count);
        if (n == g_as.count) {
            g_as.pend[g_as.layer] = g_as.buf; g_as.pendN[g_as.layer] = g_as.count;
        } else {
            free(g_as.buf);
            g_as.pend[g_as.layer] = 0; g_as.pendN[g_as.layer] = 0;
            g_as.c->sec[g_as.si].dirty = true;
            g_asyncMismatch++;
        }
        g_as.buf = 0; g_as.layer++;
    }

    if (!g_as.active) {
        float buildD2 = viewDist * viewDist;
        ChunkMesh* best = 0; int bestSi = -1; float bestD = 1e30f;
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
        ChunkSection* s = &best->sec[bestSi];
        if (s->mesh)   { free(s->mesh);   s->mesh = 0; }
        if (s->water)  { free(s->water);  s->water = 0; }
        if (s->leaves) { free(s->leaves); s->leaves = 0; }
        if (s->noMip)  { free(s->noMip);  s->noMip = 0; s->noMipCount = 0; }
        s->dirty = false;
        g_as.c = best; g_as.si = bestSi; g_as.layer = 0; g_as.active = true;
        g_as.pend[0]=g_as.pend[1]=g_as.pend[2]=0;
        g_as.pendN[0]=g_as.pendN[1]=g_as.pendN[2]=0;
    }

    while (g_as.layer <= 2) {
        if (asyncStartLayer(w)) return;
        g_as.layer++;
    }
    asyncFinalize(w);
}
