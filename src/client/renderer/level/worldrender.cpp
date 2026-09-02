
#include "world/level/world.h"
#include "world/level/chunk/chunk_cache.h"

#include "gpu/texture.h"
#include "gpu/gu.h"
#include "util/prof.h"
#include "platform/time.h"

#include <stdlib.h>
#include <malloc.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <math.h>

#include "client/renderer/level/frustum.h"

static inline void streamFreeSection(ChunkSection* s) {
    if (s->mesh)   { guDeferFree(s->mesh);   s->mesh = 0; }
    if (s->water)  { guDeferFree(s->water);  s->water = 0; }
    if (s->leaves) { guDeferFree(s->leaves); s->leaves = 0; }
    if (s->noMip)  { guDeferFree(s->noMip);  s->noMip = 0; }
    s->vertexCount = s->waterCount = s->leavesCount = s->noMipCount = 0;
    s->noMipLavaStart = 0;
    s->dirty = true;
}

static const ChunkMesh* g_visChunks[WORLD_CHUNKS_X * WORLD_CHUNKS_Z];
static int g_nVisChunks = 0;

struct OpaqueSec { float d2; const ChunkSection* s; };
static OpaqueSec g_opaqueList[WORLD_CHUNKS_X * WORLD_CHUNKS_Z * N_SECTIONS];
static int cmpOpaqueAsc(const void* a, const void* b) {
    float da = ((const OpaqueSec*)a)->d2, db = ((const OpaqueSec*)b)->d2;
    return (da > db) - (da < db);
}

extern float g_camX, g_camY, g_camZ;

volatile int g_meshOOM = 0;

float g_viewDistEff = 0.0f;
static float s_lastSlider = 0.0f;
static int   s_oomFrames = 0;

#define OOM_FRAMES_BEFORE_BACKOFF 60

float worldViewDistEffective(float slider) {
    if (slider != s_lastSlider) {
        s_lastSlider = slider;
        g_viewDistEff = slider;
        s_oomFrames = 0;
    }
    return g_viewDistEff;
}

static const float MIP_CRISP_RADIUS     = 16.0f;
static const float MIP_BLOCKS_PER_LEVEL = 16.0f;

static int s_terrainMipCount = 0;

float g_fogCullDist = 0.0f;

bool g_eyeInLava = false;
static inline float drawCull(float viewDist) {
    return (g_fogCullDist > 0.0f && g_fogCullDist < viewDist) ? g_fogCullDist : viewDist;
}

bool worldColumnDrawn(const World* w, float x, float z) {
    int cx = ((int)floorf(x)) >> 4, cz = ((int)floorf(z)) >> 4;
    if (!worldChunkReady(w, cx, cz)) return false;
    return worldMesh(w, cx, cz)->drawn;
}

void worldRebuildStep(const World* cw, float camX, float camY, float camZ, float viewDist) {
    World* w = (World*)cw;

    chunkMeshHeapProbe();

    profBegin(PROF_STREAM);
    profAdd(PROFC_STREAMIN, worldStream(w, camX, camZ, 4));
    profEnd(PROF_STREAM);

    profBegin(PROF_LIGHT);
    worldUpdateLights(w);
    profEnd(PROF_LIGHT);
    profBegin(PROF_REBUILD);

    static const int MAX_HELD_FRAMES = 12;
    static int s_heldFrames = 0;
    bool lightSettling = !w->lightQueue.empty() && s_heldFrames < MAX_HELD_FRAMES;
    s_heldFrames = lightSettling ? s_heldFrames + 1 : 0;

    worldDrainPlayerEdits(w, lightSettling ? 0 : 6);

    lightCompactStep(w);

    if (lightSettling) {

    } else {

    static const int MAX_CAND = 48;

    static const unsigned int TIME_BUDGET_US = 2000;
    float buildD2 = viewDist * viewDist;

    profBegin(PROF_RSCAN);
    struct Cand { ChunkMesh* c; int si; float d; } cand[MAX_CAND];
    int nc = 0; float worst = 1e30f;
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        if (!w->slots[i].resident || worldSlotBusy(&w->slots[i])) continue;
        ChunkMesh* c = &w->chunks[i];
        float dx = c->cx - camX, dz = c->cz - camZ;
        float hd = dx * dx + dz * dz;
        if (hd > buildD2) continue;

        if (!worldChunkMeshable(w, w->slots[i].x, w->slots[i].z)) continue;
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
    profEnd(PROF_RSCAN);
    profBegin(PROF_RBUILD);
    unsigned int tStart = sceKernelGetSystemTimeLow();
    int built = 0;
    for (int k = 0; k < nc; k++) {
        chunkBuildSection(cand[k].c, w, cand[k].si);
        built++;
        if (sceKernelGetSystemTimeLow() - tStart >= TIME_BUDGET_US) break;
    }
    profAdd(PROFC_SECTIONS, built);
    profEnd(PROF_RBUILD);
    }
    profEnd(PROF_REBUILD);
}

void worldDraw(const World* cw, float camX, float camY, float camZ, float viewDist, const Texture* terrain) {
    World* w = (World*)cw;

    if (g_meshOOM) {
        g_meshOOM = 0;
        if (++s_oomFrames >= OOM_FRAMES_BEFORE_BACKOFF) {
            s_oomFrames = 0;

            float next = (g_viewDistEff > 32.0f) ? 32.0f : 16.0f;
            if (next < g_viewDistEff) g_viewDistEff = next;
        }
    } else if (s_oomFrames > 0) {
        s_oomFrames--;
    }

    if (!gameFrozen()) worldRebuildStep(w, camX, camY, camZ, viewDist);

    profBegin(PROF_CULL);

    float keepD2 = (viewDist + 32.0f) * (viewDist + 32.0f);
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        if (!w->slots[i].resident || worldSlotBusy(&w->slots[i])) continue;
        ChunkMesh* c = &w->chunks[i];
        float dx = c->cx - camX, dz = c->cz - camZ;
        if (dx * dx + dz * dz <= keepD2) continue;
        for (int si = 0; si < N_SECTIONS; si++) {
            ChunkSection* s = &c->sec[si];
            if (s->mesh || s->water || s->leaves || s->noMip) streamFreeSection(s);
        }
    }

    float maxD2 = drawCull(viewDist) * drawCull(viewDist);

    g_nVisChunks = 0;
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
        ChunkMesh* c = &w->chunks[i];

        if (!w->slots[i].resident || worldSlotBusy(&w->slots[i])) {
            c->drawn = false;
            for (int si = 0; si < N_SECTIONS; si++) c->sec[si].visible = false;
            continue;
        }
        float dx = c->cx - camX, dz = c->cz - camZ;
        bool off = (dx * dx + dz * dz > maxD2 || !columnVisible(c));

        c->drawn = (dx * dx + dz * dz <= maxD2);
        for (int si = 0; si < N_SECTIONS; si++) {
            ChunkSection* s = &c->sec[si];
            s->visible = off ? false : sectionVisible(c, s);
        }
        if (!off) g_visChunks[g_nVisChunks++] = c;
    }
    profEnd(PROF_CULL);

    int nOpaque = 0;
    for (int i = 0; i < g_nVisChunks; i++) {
        const ChunkMesh* c = g_visChunks[i];
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
    if (distMip) textureMipAuto();
    sceGuEnable(GU_ALPHA_TEST);

    if (terrain) {
        bool any = false;
        for (int i = 0; i < g_nVisChunks; i++) {
            const ChunkMesh* c = g_visChunks[i];
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
                chunkDrawNoMipSection(s, g_eyeInLava ? NOMIP_NO_LAVA : NOMIP_ALL);
            }
        }

        if (g_eyeInLava) {

            if (distMip) sceGuTexLevelMode(GU_TEXTURE_CONST, 0.0f);
            sceGuFrontFace(GU_CW);
            for (int i = 0; i < g_nVisChunks; i++) {
                const ChunkMesh* c = g_visChunks[i];
                for (int si = 0; si < N_SECTIONS; si++) {
                    const ChunkSection* s = &c->sec[si];
                    if (s->noMipCount == 0 || !s->visible) continue;
                    chunkDrawNoMipSection(s, NOMIP_LAVA);
                }
            }
            sceGuFrontFace(GU_CCW);
        }
        if (distMip) textureMipAuto();
        if (any) {
            extern int g_noMipmap;
            if (g_noMipmap) textureBindNoMip(terrain);
            else textureBind(terrain);
        }
    }

    extern int g_fancyGraphics, g_fancyLeaves;
    static int s_prevLeafMode = -1;
    int leafMode = g_fancyGraphics | (g_fancyLeaves << 1);
    if (leafMode != s_prevLeafMode) {
        s_prevLeafMode = leafMode;
        for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
            ChunkMesh* c = &w->chunks[i];
            for (int si = 0; si < N_SECTIONS; si++) {
                ChunkSection* s = &c->sec[si];
                if (s->leavesCount || s->noMipCount) s->dirty = true;
            }
        }
    }

    if (distMip)
        sceGuTexFilter(g_fancyGraphics ? GU_NEAREST_MIPMAP_NEAREST
                                       : GU_NEAREST_MIPMAP_LINEAR, GU_NEAREST);
    sceGuEnable(GU_ALPHA_TEST);
    for (int i = 0; i < g_nVisChunks; i++) {
        const ChunkMesh* c = g_visChunks[i];
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
        textureMipAuto();
    }

    guListSync();
    guGlobalsCheck(GU_PHASE_TERRAIN);
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
    for (int i = 0; i < g_nVisChunks; i++) {
        const ChunkMesh* c = g_visChunks[i];
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
    if (distMip) textureMipAuto();
    guListSync();
    guGlobalsCheck(GU_PHASE_WATER);
}
