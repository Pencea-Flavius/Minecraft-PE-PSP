
#include "world/level/world.h"

#include "gpu/texture.h"

#include <stdlib.h>
#include <malloc.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <math.h>

#include "client/renderer/level/frustum.h"

static inline void streamFreeSection(ChunkSection* s) {
    if (s->mesh)   { free(s->mesh);   s->mesh = 0; }
    if (s->water)  { free(s->water);  s->water = 0; }
    if (s->leaves) { free(s->leaves); s->leaves = 0; }
    if (s->noMip)  { free(s->noMip);  s->noMip = 0; }
    s->vertexCount = s->waterCount = s->leavesCount = s->noMipCount = 0;
    s->dirty = true;
}

struct OpaqueSec { float d2; const ChunkSection* s; };
static OpaqueSec g_opaqueList[WORLD_CHUNKS_X * WORLD_CHUNKS_Z * N_SECTIONS];
static int cmpOpaqueAsc(const void* a, const void* b) {
    float da = ((const OpaqueSec*)a)->d2, db = ((const OpaqueSec*)b)->d2;
    return (da > db) - (da < db);
}

extern float g_camX, g_camY, g_camZ;

volatile int g_meshOOM = 0;
int g_oomCount = 0;
int g_residentSections = 0;

int g_visibleSections = 0;

static const float MIP_CRISP_RADIUS     = 16.0f;
static const float MIP_BLOCKS_PER_LEVEL = 16.0f;

static int s_terrainMipCount = 0;

float g_fogCullDist = 0.0f;
static inline float drawCull(float viewDist) {
    return (g_fogCullDist > 0.0f && g_fogCullDist < viewDist) ? g_fogCullDist : viewDist;
}

void worldRebuildStep(const World* cw, float camX, float camY, float camZ, float viewDist) {
    World* w = (World*)cw;

    worldUpdateLights(w);
    worldDrainPlayerEdits(w, 6);

    extern int g_diagMode;
    if (g_diagMode == 3) {

    } else {

    static const int MAX_CAND = 48;
    static const unsigned int TIME_BUDGET_US = 2000;
    float buildD2 = viewDist * viewDist;

    struct Cand { ChunkMesh* c; int si; float d; } cand[MAX_CAND];
    int nc = 0; float worst = 1e30f;
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        ChunkMesh* c = &w->chunks[i];
        float dx = c->cx - camX, dz = c->cz - camZ;
        float hd = dx * dx + dz * dz;
        if (hd > buildD2) continue;
        if (nc == MAX_CAND && hd >= worst) continue;
        for (int si = 0; si < N_SECTIONS; si++) {
            if (!c->sec[si].dirty) continue;
            float dy = (float)(si * SECTION_SY + SECTION_SY / 2) - camY;
            float wd = hd + dy * dy * 4.0f;
            if (nc < MAX_CAND) {
                int j = nc++;
                for (; j > 0 && cand[j-1].d > wd; j--) cand[j] = cand[j-1];
                cand[j].c = c; cand[j].si = si; cand[j].d = wd;
                worst = cand[nc-1].d;
            } else if (wd < worst) {
                int j = MAX_CAND - 1;
                for (; j > 0 && cand[j-1].d > wd; j--) cand[j] = cand[j-1];
                cand[j].c = c; cand[j].si = si; cand[j].d = wd;
                worst = cand[MAX_CAND-1].d;
            }
        }
    }
    unsigned int tStart = sceKernelGetSystemTimeLow();
    for (int k = 0; k < nc; k++) {
        chunkBuildSection(cand[k].c, w, cand[k].si);
        if (sceKernelGetSystemTimeLow() - tStart >= TIME_BUDGET_US) break;
    }
    }

}

void worldDraw(const World* cw, float camX, float camY, float camZ, float viewDist, const Texture* terrain) {
    World* w = (World*)cw;

    if (g_meshOOM) { g_oomCount++; g_meshOOM = 0; }

    worldRebuildStep(w, camX, camY, camZ, viewDist);

    float keepD2 = (viewDist + 32.0f) * (viewDist + 32.0f);
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        ChunkMesh* c = &w->chunks[i];
        float dx = c->cx - camX, dz = c->cz - camZ;
        if (dx * dx + dz * dz <= keepD2) continue;
        for (int si = 0; si < N_SECTIONS; si++) {
            ChunkSection* s = &c->sec[si];
            if (s->mesh || s->water || s->leaves || s->noMip) streamFreeSection(s);
        }
    }

    float maxD2 = drawCull(viewDist) * drawCull(viewDist);

    int resident = 0, vis = 0;
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        ChunkMesh* c = &w->chunks[i];
        float dx = c->cx - camX, dz = c->cz - camZ;
        bool off = (dx * dx + dz * dz > maxD2 || !columnVisible(c));
        for (int si = 0; si < N_SECTIONS; si++) {
            ChunkSection* s = &c->sec[si];
            if (s->mesh || s->water || s->leaves || s->noMip) resident++;
            s->visible = off ? false : sectionVisible(c, s);
            if (s->visible) vis++;
        }
    }
    g_residentSections = resident;
    g_visibleSections = vis;

    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++)
        for (int si = 0; si < N_SECTIONS; si++) w->chunks[i].sec[si].reachable = false;

    int ccx = (int)floorf(camX / (float)CHUNK_SX); ccx = ccx < 0 ? 0 : (ccx >= WORLD_CHUNKS_X ? WORLD_CHUNKS_X - 1 : ccx);
    int ccz = (int)floorf(camZ / (float)CHUNK_SZ); ccz = ccz < 0 ? 0 : (ccz >= WORLD_CHUNKS_Z ? WORLD_CHUNKS_Z - 1 : ccz);
    int csi = (int)floorf(camY / (float)SECTION_SY); csi = csi < 0 ? 0 : (csi >= N_SECTIONS ? N_SECTIONS - 1 : csi);

    struct VisNode { unsigned char col, si, entered, dirs; };
    static VisNode bfs[WORLD_CHUNKS_X * WORLD_CHUNKS_Z * N_SECTIONS];
    int qt = 0, qh = 0;
    int startCol = ccz * WORLD_CHUNKS_X + ccx;
    w->chunks[startCol].sec[csi].reachable = true;
    bfs[qt++] = { (unsigned char)startCol, (unsigned char)csi, 6, 0 };

    while (qh < qt) {
        VisNode n = bfs[qh++];
        int ncx = n.col % WORLD_CHUNKS_X, ncz = n.col / WORLD_CHUNKS_X;
        ChunkSection* cs = &w->chunks[n.col].sec[n.si];
        for (int d = 0; d < 6; d++) {
            if (n.entered != 6 && !(cs->vis[n.entered] & (1 << d))) continue;
            if (n.dirs & (1 << (d ^ 1))) continue;
            int tcx = ncx, tcz = ncz, tsi = n.si;
            if (d == 0) tcx--; else if (d == 1) tcx++;
            else if (d == 2) tsi--; else if (d == 3) tsi++;
            else if (d == 4) tcz--; else tcz++;
            if (tcx < 0 || tcx >= WORLD_CHUNKS_X || tcz < 0 || tcz >= WORLD_CHUNKS_Z) continue;
            if (tsi < 0 || tsi >= N_SECTIONS) continue;
            int tcol = tcz * WORLD_CHUNKS_X + tcx;
            ChunkMesh* tc = &w->chunks[tcol];
            float dx = tc->cx - camX, dz = tc->cz - camZ;
            if (dx * dx + dz * dz > maxD2) continue;
            ChunkSection* ts = &tc->sec[tsi];
            if (ts->reachable) continue;
            ts->reachable = true;
            bfs[qt++] = { (unsigned char)tcol, (unsigned char)tsi,
                          (unsigned char)(d ^ 1), (unsigned char)(n.dirs | (1 << d)) };
        }
    }

    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++)
        for (int si = 0; si < N_SECTIONS; si++)
            if (!w->chunks[i].sec[si].reachable) w->chunks[i].sec[si].visible = false;

    int nOpaque = 0;
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        const ChunkMesh* c = &w->chunks[i];
        float dx = c->cx - camX, dz = c->cz - camZ;
        for (int si = 0; si < N_SECTIONS; si++) {
            const ChunkSection* s = &c->sec[si];
            if (s->vertexCount == 0 || !s->visible) continue;
            float dy = (float)(si * SECTION_SY + SECTION_SY / 2) - camY;
            g_opaqueList[nOpaque].d2 = dx * dx + dy * dy + dz * dz;
            g_opaqueList[nOpaque].s = s;
            nOpaque++;
        }
    }
    qsort(g_opaqueList, nOpaque, sizeof(OpaqueSec), cmpOpaqueAsc);
    sceGuDisable(GU_ALPHA_TEST);

    extern int g_noMipmap;
    bool distMip = !g_noMipmap && terrain && terrain->mipCount > 0;
    float maxLvl = distMip ? (float)terrain->mipCount : 0.0f;
    s_terrainMipCount = terrain ? terrain->mipCount : 0;

    if (terrain) {
        if (g_noMipmap) textureBindNoMip(terrain);
        else            textureBind(terrain);
    }
    for (int i = 0; i < nOpaque; i++) {
        if (distMip) {
            float lvl = (sqrtf(g_opaqueList[i].d2) - MIP_CRISP_RADIUS) * (1.0f / MIP_BLOCKS_PER_LEVEL);
            if (lvl < 0.0f) lvl = 0.0f; else if (lvl > maxLvl) lvl = maxLvl;
            sceGuTexLevelMode(GU_TEXTURE_CONST, lvl);
        }
        chunkDrawSection(g_opaqueList[i].s);
    }
    if (distMip) sceGuTexLevelMode(GU_TEXTURE_AUTO, 0.0f);
    sceGuEnable(GU_ALPHA_TEST);

    if (terrain) {
        bool any = false;
        for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
            const ChunkMesh* c = &w->chunks[i];
            float dx = c->cx - camX, dz = c->cz - camZ;
            for (int si = 0; si < N_SECTIONS; si++) {
                const ChunkSection* s = &c->sec[si];
                if (s->noMipCount == 0 || !s->visible) continue;
                if (!any) {
                    if (distMip) {
                        textureBind(terrain);

                        sceGuTexFilter(GU_NEAREST_MIPMAP_NEAREST, GU_NEAREST);
                    } else {
                        textureBindNoMip(terrain);
                    }
                    any = true;
                }
                if (distMip) {
                    float dy = (float)(si * SECTION_SY + SECTION_SY / 2) - camY;
                    float lvl = (sqrtf(dx * dx + dy * dy + dz * dz) - MIP_CRISP_RADIUS) * (1.0f / MIP_BLOCKS_PER_LEVEL);
                    if (lvl < 0.0f) lvl = 0.0f; else if (lvl > maxLvl) lvl = maxLvl;
                    sceGuTexLevelMode(GU_TEXTURE_CONST, lvl);
                }
                chunkDrawNoMipSection(s);
            }
        }
        if (distMip) sceGuTexLevelMode(GU_TEXTURE_AUTO, 0.0f);
        if (any) {
            extern int g_noMipmap;
            if (g_noMipmap) textureBindNoMip(terrain);
            else textureBind(terrain);
        }
    }

    extern int g_fancyGraphics;
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        ChunkMesh* c = &w->chunks[i];
        for (int si = 0; si < N_SECTIONS; si++) {
            ChunkSection* s = &c->sec[si];
            if ((s->leavesCount == 0 && s->noMipCount == 0) || !s->visible || s->dirty) continue;
            int y0 = si * SECTION_SY, y1 = y0 + SECTION_SY;
            bool wantOpaque = leafOpaqueBand(c, y0, y1, camX, camY, camZ, g_fancyGraphics != 0);
            bool wantCull   = leafCullBand(c, y0, y1, camX, camY, camZ, g_fancyGraphics != 0);
            if (wantOpaque != s->leavesOpaqueBand || wantCull != s->leavesCullBand) s->dirty = true;
        }
    }

    if (distMip)
        sceGuTexFilter(g_fancyGraphics ? GU_NEAREST_MIPMAP_NEAREST
                                       : GU_NEAREST_MIPMAP_LINEAR, GU_NEAREST);
    sceGuEnable(GU_ALPHA_TEST);
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        const ChunkMesh* c = &w->chunks[i];
        float dx = c->cx - camX, dz = c->cz - camZ;
        for (int si = 0; si < N_SECTIONS; si++) {
            const ChunkSection* s = &c->sec[si];
            if (s->leavesCount == 0 || !s->visible) continue;
            if (distMip) {
                float dy = (float)(si * SECTION_SY + SECTION_SY / 2) - camY;
                float lvl = (sqrtf(dx * dx + dy * dy + dz * dz) - MIP_CRISP_RADIUS) * (1.0f / MIP_BLOCKS_PER_LEVEL);
                if (lvl < 0.0f) lvl = 0.0f; else if (lvl > maxLvl) lvl = maxLvl;
                sceGuTexLevelMode(GU_TEXTURE_CONST, lvl);
            }
            chunkDrawLeavesSection(s);
        }
    }

    if (distMip) {
        sceGuTexFilter(GU_NEAREST_MIPMAP_LINEAR, GU_NEAREST);
        sceGuTexLevelMode(GU_TEXTURE_AUTO, 0.0f);
    }
}

struct WaterSec { float d2; const ChunkSection* s; };
static WaterSec g_waterList[WORLD_CHUNKS_X * WORLD_CHUNKS_Z * N_SECTIONS];

static int cmpWaterDesc(const void* a, const void* b) {
    float da = ((const WaterSec*)a)->d2, db = ((const WaterSec*)b)->d2;
    return (da < db) - (da > db);
}

void worldDrawWater(const World* w, float camX, float camY, float camZ, float viewDist) {

    float maxD2 = drawCull(viewDist) * drawCull(viewDist);

    int cnt = 0;
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        const ChunkMesh* c = &w->chunks[i];
        float dx = c->cx - camX, dz = c->cz - camZ;
        if (dx * dx + dz * dz > maxD2) continue;
        for (int si = 0; si < N_SECTIONS; si++) {
            const ChunkSection* s = &c->sec[si];
            if (s->waterCount == 0 || !s->visible) continue;
            float scy = (float)(si * SECTION_SY + SECTION_SY / 2);
            float dy = scy - camY;
            g_waterList[cnt].d2 = dx * dx + dy * dy + dz * dz;
            g_waterList[cnt].s = s;
            cnt++;
        }
    }

    qsort(g_waterList, cnt, sizeof(WaterSec), cmpWaterDesc);

    extern int g_noMipmap;
    bool distMip = !g_noMipmap && s_terrainMipCount > 0;
    float maxLvl = (float)s_terrainMipCount;
    for (int i = 0; i < cnt; i++) {
        if (distMip) {
            float lvl = (sqrtf(g_waterList[i].d2) - MIP_CRISP_RADIUS) * (1.0f / MIP_BLOCKS_PER_LEVEL);
            if (lvl < 0.0f) lvl = 0.0f; else if (lvl > maxLvl) lvl = maxLvl;
            sceGuTexLevelMode(GU_TEXTURE_CONST, lvl);
        }
        chunkDrawWaterSection(g_waterList[i].s);
    }
    if (distMip) sceGuTexLevelMode(GU_TEXTURE_AUTO, 0.0f);
}
