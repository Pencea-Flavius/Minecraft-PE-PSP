
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
#include "client/renderer/level/near_patch.h"

void streamFreeSection(ChunkSection* s) {
    s->gen++;
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

int seaCellQuarter(int cx, int cz) {
    unsigned int h = (unsigned int)cx * 73856093u ^ (unsigned int)cz * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (int)(h & 3u);
}

#if WORLD_SIZE_CHUNKS
static const int SEA_RING_N = WORLD_CHUNKS_X + 2;
static unsigned char s_seaCol[WORLD_H];
static ChunkMesh s_seaTmpl;
static ChunkMesh s_seaRing[4 * (WORLD_CHUNKS_X + 2) - 4];
static int  s_seaDarken = -1;
static bool s_seaReady = false;
struct SeaCell { float d2; short cx, cz; short ring; };
static SeaCell s_seaCells[1024];
static int s_nSeaCells = 0;

static int seaRingIndex(int cx, int cz) {
    const int n = SEA_RING_N, x = cx + 1, z = cz + 1;
    if (x < 0 || z < 0 || x >= n || z >= n) return -1;
    if (z == 0)     return x;
    if (z == n - 1) return n + x;
    if (x == 0)     return 2 * n + (z - 1);
    if (x == n - 1) return 3 * n - 2 + (z - 1);
    return -1;
}

static bool seaSectionHasBlocks(int si) {
    for (int y = si * SECTION_SY; y < (si + 1) * SECTION_SY; y++)
        if (s_seaCol[y] != BLOCK_AIR) return true;
    return false;
}

static void seaOnEdgeBlockChanged(int x, int y, int z) {
    for (int dx = -1; dx <= 1; dx++)
    for (int dz = -1; dz <= 1; dz++) {
        const int r = seaRingIndex((x + dx) >> 4, (z + dz) >> 4);
        if (r < 0) continue;
        for (int dy = -1; dy <= 1; dy++) {
            const int yy = y + dy;
            if (yy >= 0 && yy < WORLD_H) s_seaRing[r].sec[yy / SECTION_SY].dirty = true;
        }
    }
}

static void seaRefresh(World* w) {
    unsigned char col[WORLD_H];
    int sky;
    activeBorderColumn(col, &sky);
    const bool colChanged = !s_seaReady || memcmp(col, s_seaCol, WORLD_H) != 0 || sky != g_edgeSkyFromY;
    if (colChanged || s_seaDarken != g_skyDarken) {
        memcpy(s_seaCol, col, WORLD_H);
        g_edgeSkyFromY = sky;
        g_seaColumn    = s_seaCol;
        g_onEdgeBlockChanged = seaOnEdgeBlockChanged;
        s_seaDarken    = g_skyDarken;

        const int nRing = (int)(sizeof(s_seaRing) / sizeof(s_seaRing[0]));
        for (int si = 0; si < N_SECTIONS; si++) {
            if (colChanged) streamFreeSection(&s_seaTmpl.sec[si]);
            s_seaTmpl.sec[si].dirty = true;
            for (int r = 0; r < nRing; r++) {
                if (colChanged) streamFreeSection(&s_seaRing[r].sec[si]);
                s_seaRing[r].sec[si].dirty = true;
            }
        }

        if (colChanged) {
            for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) {
                const LevelChunk* sl = &w->slots[i];
                if (sl->x != 0 && sl->z != 0 && sl->x != WORLD_CHUNKS_X - 1 && sl->z != WORLD_CHUNKS_Z - 1) continue;
                for (int si = 0; si < N_SECTIONS; si++) w->chunks[i].sec[si].dirty = true;
            }
        }
        s_seaReady = true;
    }
}

static void seaCollect(const World* w, float camX, float camZ, float dist) {
    s_nSeaCells = 0;
    if (!s_seaReady) return;
    for (int si = 0; si < N_SECTIONS; si++) {
        ChunkSection* s = &s_seaTmpl.sec[si];
        if (s->dirty && edgeSectionVisible(s_seaCol, si)) {
            s_seaTmpl.ox = s_seaTmpl.oz = -8 * CHUNK_SX;
            chunkBuildSection(&s_seaTmpl, w, si);
        }
    }
    const float d2max = dist * dist;
    const int cx0 = (int)floorf((camX - dist) / CHUNK_SX) - 1, cx1 = (int)floorf((camX + dist) / CHUNK_SX) + 1;
    const int cz0 = (int)floorf((camZ - dist) / CHUNK_SZ) - 1, cz1 = (int)floorf((camZ + dist) / CHUNK_SZ) + 1;
    int ringBuilds = 2;
    for (int cx = cx0; cx <= cx1; cx++)
    for (int cz = cz0; cz <= cz1; cz++) {
        if (worldChunkInBounds(cx, cz)) continue;
        float dx = cx * CHUNK_SX + 8 - camX, dz = cz * CHUNK_SZ + 8 - camZ;
        float d2 = dx * dx + dz * dz;
        if (d2 > d2max) continue;
        const int ring = seaRingIndex(cx, cz);
        ChunkMesh* m = ring >= 0 ? &s_seaRing[ring] : &s_seaTmpl;
        m->ox = cx * CHUNK_SX; m->oz = cz * CHUNK_SZ;
        if (ring >= 0) {
            for (int si = 0; si < N_SECTIONS && ringBuilds > 0; si++) {
                if (!m->sec[si].dirty || !seaSectionHasBlocks(si)) continue;
                chunkBuildSection(m, w, si);
                ringBuilds--;
            }
        }
        if (!columnVisible(m)) continue;
        if (s_nSeaCells == (int)(sizeof(s_seaCells) / sizeof(s_seaCells[0]))) return;
        s_seaCells[s_nSeaCells].d2 = d2;
        s_seaCells[s_nSeaCells].cx = (short)cx;
        s_seaCells[s_nSeaCells].cz = (short)cz;
        s_seaCells[s_nSeaCells].ring = (short)ring;
        s_nSeaCells++;
    }
}

const ChunkSection* seaSectionAt(int cx, int cz, int si, int* quarter) {
    *quarter = 0;
    if (!s_seaReady || worldChunkInBounds(cx, cz)) return 0;
    const int ring = seaRingIndex(cx, cz);
    const ChunkSection* s = ring >= 0 ? &s_seaRing[ring].sec[si] : &s_seaTmpl.sec[si];
    if (!(s->vertexCount || s->waterCount || s->leavesCount || s->noMipCount)) return 0;
    if (ring < 0 && edgeColumnTopRotates(s_seaCol)) *quarter = seaCellQuarter(cx, cz);
    return s;
}

static void seaDraw(bool water, bool distMip, float maxLvl) {
    for (int i = 0; i < s_nSeaCells; i++) {
        const SeaCell& cell = s_seaCells[i];
        const bool ring = cell.ring >= 0;
        ChunkMesh* m = ring ? &s_seaRing[cell.ring] : &s_seaTmpl;
        if (distMip) {
            float lvl = (sqrtf(cell.d2) - MIP_CRISP_RADIUS) * (1.0f / MIP_BLOCKS_PER_LEVEL);
            if (lvl < 0.0f) lvl = 0.0f; else if (lvl > maxLvl) lvl = maxLvl;
            sceGuTexLevelMode(GU_TEXTURE_CONST, lvl);
        }
        for (int si = 0; si < N_SECTIONS; si++) {
            ChunkSection* s = &m->sec[si];
            if (!(water ? s->waterCount : s->vertexCount)) continue;
            if (water && nearPatchOwnsWater(s, cell.cx * CHUNK_SX, cell.cz * CHUNK_SZ)) continue;

            if (ring) {
                m->ox = cell.cx * CHUNK_SX; m->oz = cell.cz * CHUNK_SZ;
                if (!sectionVisible(m, s)) continue;
            } else {
                s->ox = cell.cx * CHUNK_SX;
                s->oz = cell.cz * CHUNK_SZ;
                if (edgeColumnTopRotates(s_seaCol)) g_chunkDrawQuarter = seaCellQuarter(cell.cx, cell.cz);
            }
            if (water) chunkDrawWaterSection(s); else chunkDrawSection(s);
            g_chunkDrawQuarter = 0;
        }
    }
}
#else
const ChunkSection* seaSectionAt(int, int, int, int* quarter) { *quarter = 0; return 0; }
#endif

bool worldColumnDrawn(const World* w, float x, float z) {
    int cx = ((int)floorf(x)) >> 4, cz = ((int)floorf(z)) >> 4;
    if (!worldChunkReady(w, cx, cz)) return false;
    return worldMesh(w, cx, cz)->drawn;
}

void worldRebuildStep(const World* cw, float camX, float camY, float camZ, float viewDist) {
    World* w = (World*)cw;

    chunkMeshHeapProbe();
#if WORLD_SIZE_CHUNKS
    seaRefresh(w);
#endif

    profBegin(PROF_STREAM);
    extern volatile int g_powerSuspended;
    if (!g_powerSuspended) profAdd(PROFC_STREAMIN, worldStream(w, camX, camZ, 4));
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

    nearPatchRefresh(w);

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
#if WORLD_SIZE_CHUNKS
    seaCollect(w, camX, camZ, drawCull(viewDist));
#endif
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
#if WORLD_SIZE_CHUNKS
    seaDraw(false, distMip, maxLvl);
#endif

    if (nearPatchHas(NEAR_PATCH_OPAQUE)) {
        if (distMip) sceGuTexLevelMode(GU_TEXTURE_CONST, 0.0f);
        nearPatchDraw(NEAR_PATCH_OPAQUE);
    }
    if (distMip) textureMipAuto();
    sceGuEnable(GU_ALPHA_TEST);

    if (terrain) {
        bool any = false;
        const auto bindCutout = [&]() {
            if (any) return;
            if (distMip) {
                textureBind(terrain);
                sceGuTexFilter(GU_NEAREST_MIPMAP_NEAREST, GU_NEAREST);
            } else {
                textureBindNoMip(terrain);
            }
            any = true;
        };
        for (int i = 0; i < g_nVisChunks; i++) {
            const ChunkMesh* c = g_visChunks[i];
            float dx = c->cx - camX, dz = c->cz - camZ;
            for (int si = 0; si < N_SECTIONS; si++) {
                const ChunkSection* s = &c->sec[si];
                if (s->noMipCount == 0 || !s->visible) continue;
                bindCutout();
                if (distMip) {
                    float dy = (float)(si * SECTION_SY + SECTION_SY / 2) - camY;
                    float lvl = (sqrtf(dx * dx + dy * dy + dz * dz) - MIP_CRISP_RADIUS) * (1.0f / MIP_BLOCKS_PER_LEVEL);
                    if (lvl < 0.0f) lvl = 0.0f; else if (lvl > maxLvl) lvl = maxLvl;
                    sceGuTexLevelMode(GU_TEXTURE_CONST, lvl);
                }
                chunkDrawNoMipSection(s, g_eyeInLava ? NOMIP_NO_LAVA : NOMIP_ALL);
            }
        }

        if (nearPatchHas(NEAR_PATCH_CUTOUT) || (!g_eyeInLava && nearPatchHas(NEAR_PATCH_LAVA))) {
            bindCutout();
            if (distMip) sceGuTexLevelMode(GU_TEXTURE_CONST, 0.0f);
            nearPatchDraw(NEAR_PATCH_CUTOUT);
            if (!g_eyeInLava) nearPatchDraw(NEAR_PATCH_LAVA);
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
            nearPatchDraw(NEAR_PATCH_LAVA);
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

    if (nearPatchHas(NEAR_PATCH_LEAVES)) {
        if (distMip) sceGuTexLevelMode(GU_TEXTURE_CONST, 0.0f);
        nearPatchDraw(NEAR_PATCH_LEAVES);
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
            if (nearPatchOwnsWater(s, s->ox, s->oz)) continue;
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
#if WORLD_SIZE_CHUNKS

    seaDraw(true, distMip, maxLvl);
#endif
    for (int i = 0; i < cnt; i++) {
        if (distMip) {
            float lvl = (sqrtf(g_waterList[i].d2) - MIP_CRISP_RADIUS) * (1.0f / MIP_BLOCKS_PER_LEVEL);
            if (lvl < 0.0f) lvl = 0.0f; else if (lvl > maxLvl) lvl = maxLvl;
            sceGuTexLevelMode(GU_TEXTURE_CONST, lvl);
        }
        chunkDrawWaterSection(g_waterList[i].s);
    }

    if (nearPatchHas(NEAR_PATCH_WATER)) {
        if (distMip) sceGuTexLevelMode(GU_TEXTURE_CONST, 0.0f);
        nearPatchDraw(NEAR_PATCH_WATER);
    }
    if (distMip) textureMipAuto();
    guListSync();
    guGlobalsCheck(GU_PHASE_WATER);
}
