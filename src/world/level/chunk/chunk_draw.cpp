#include "world/level/chunk/chunk.h"
#include "gpu/gu.h"
#include "platform/dcache.h"
#include "util/prof.h"
#include "world/level/chunk/mesh_sink.h"
#include "util/fast_memcpy.h"
#include <pspgu.h>
#include <pspgum.h>
#include <malloc.h>
#include <pspkernel.h>
#include <pspgum.h>

void chunkPackInto(DrawVertex* d, const ChunkVertex* s, int n,
                   int ox, int oy, int oz, int* qlo, int* qhi) {
    profAdd(PROFC_PACKVERTS, n);
    profBegin(PROF_MCONV);
    int lo = *qlo, hi = *qhi;
    for (int i = 0; i < n; i++) {
        d[i].u = uvQ(s[i].u); d[i].v = uvQ(s[i].v); d[i].color = s[i].color;
        d[i].x = posQ(s[i].x - ox); d[i].y = posQ(s[i].y - oy); d[i].z = posQ(s[i].z - oz); d[i].w = 0;
        if (d[i].y < lo) lo = d[i].y;
        if (d[i].y > hi) hi = d[i].y;
    }
    *qlo = lo; *qhi = hi;
    profEnd(PROF_MCONV);
}

float chunkPackDecodeY(int q, int oy) { return (float)q / (float)POS_ENC + oy; }

DrawVertex* chunkPackFinish(const DrawVertex* staging, int n) {
    profBegin(PROF_MALLOC);
    DrawVertex* d = (DrawVertex*)memalign(64, (size_t)n * sizeof(DrawVertex));
    profEnd(PROF_MALLOC);
    if (!d) return 0;

    memcpy_vfpu(d, staging, (size_t)n * sizeof(DrawVertex));
    dcacheFlush(d, (size_t)n * sizeof(DrawVertex));
    return d;
}

DrawVertex* chunkPack(const ChunkVertex* s, int n, int ox, int oy, int oz,
                      float* ylo, float* yhi) {
    profBegin(PROF_MALLOC);
    DrawVertex* d = (DrawVertex*)memalign(64, (size_t)n * sizeof(DrawVertex));
    profEnd(PROF_MALLOC);
    if (!d) return 0;
    int qlo = 32767, qhi = -32768;
    chunkPackInto(d, s, n, ox, oy, oz, &qlo, &qhi);
    if (ylo) *ylo = chunkPackDecodeY(qlo, oy);
    if (yhi) *yhi = chunkPackDecodeY(qhi, oy);
    dcacheFlush(d, (size_t)n * sizeof(DrawVertex));
    return d;
}

float g_relBaseX = 0.0f, g_relBaseY = 0.0f, g_relBaseZ = 0.0f;
int   g_chunkDrawQuarter = 0;

static inline void chunkSetModelAt(int ox, int oy, int oz, float scaleMul) {
    const float sm = POS_MODEL_SCALE * scaleMul;
    ScePspFMatrix4 m;
    m.x.x = sm;   m.x.y = 0.0f; m.x.z = 0.0f; m.x.w = 0.0f;
    m.y.x = 0.0f; m.y.y = sm;   m.y.z = 0.0f; m.y.w = 0.0f;
    m.z.x = 0.0f; m.z.y = 0.0f; m.z.z = sm;   m.z.w = 0.0f;
    m.w.x = (float)ox - g_relBaseX;
    m.w.y = (float)oy - g_relBaseY;
    m.w.z = (float)oz - g_relBaseZ;
    m.w.w = 1.0f;

    if (g_chunkDrawQuarter) {
        const float W = (float)CHUNK_SX;
        switch (g_chunkDrawQuarter & 3) {
            case 1: m.x.x = 0.0f; m.x.z = -sm; m.z.x = sm;  m.z.z = 0.0f; m.w.z += W; break;
            case 2: m.x.x = -sm;  m.z.z = -sm; m.w.x += W;  m.w.z += W; break;
            case 3: m.x.x = 0.0f; m.x.z = sm;  m.z.x = -sm; m.z.z = 0.0f; m.w.x += W; break;
        }
    }
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadMatrix(&m);
}
static inline void chunkSetModel(const ChunkSection* s, float scaleMul) {
    chunkSetModelAt(s->ox, s->oy, s->oz, scaleMul);
}
void chunkSetModelOrigin(int ox, int oy, int oz, float scaleMul) {
    chunkSetModelAt(ox, oy, oz, scaleMul);
}

void chunkDrawSection(const ChunkSection* s) {
    if (s->vertexCount <= 0 || !s->mesh) return;
    chunkSetModel(s, SEAM_OVERSCALE_OPAQUE);
    const unsigned int fmt = GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_3D;
    sceGumDrawArray(GU_TRIANGLES, fmt, s->vertexCount, 0, s->mesh);
}

void chunkDrawWaterSection(const ChunkSection* s) {
    if (s->waterCount > 0 && s->water) {
        chunkSetModel(s, SEAM_OVERSCALE_TRANS);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_3D,
                        s->waterCount, 0, s->water);
    }
}

void chunkDrawLeavesSection(const ChunkSection* s) {
    if (s->leavesCount > 0 && s->leaves) {
        chunkSetModel(s, SEAM_OVERSCALE_OPAQUE);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_3D,
                        s->leavesCount, 0, s->leaves);
    }
}

void chunkDrawNoMipSection(const ChunkSection* s, int part) {
    if (s->noMipCount <= 0 || !s->noMip) return;

    int first = 0, count = s->noMipCount;
    if (part == NOMIP_NO_LAVA) count = s->noMipLavaStart;
    else if (part == NOMIP_LAVA) { first = s->noMipLavaStart; count = s->noMipCount - first; }
    if (count <= 0) return;
    chunkSetModel(s, SEAM_OVERSCALE_OPAQUE);
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_3D,
                    count, 0, s->noMip + first);
}

void chunkFreeMesh(ChunkMesh* c) {
    for (int si = 0; si < N_SECTIONS; si++) {
        ChunkSection* s = &c->sec[si];
        s->gen++;
        if (s->mesh)   { guDeferFree(s->mesh);   s->mesh = 0; }
        if (s->water)  { guDeferFree(s->water);  s->water = 0; }
        if (s->leaves) { guDeferFree(s->leaves); s->leaves = 0; }
        if (s->noMip)  { guDeferFree(s->noMip);  s->noMip = 0; }
        s->vertexCount = s->waterCount = s->leavesCount = s->noMipCount = 0;
        s->noMipLavaStart = 0;
    }
}
