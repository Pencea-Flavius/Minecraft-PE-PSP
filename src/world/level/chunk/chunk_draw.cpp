#include "world/level/chunk/chunk.h"
#include <pspgu.h>
#include <pspgum.h>
#include <malloc.h>
#include <pspkernel.h>
#include <pspgum.h>

DrawVertex* chunkPack(const ChunkVertex* s, int n, int ox, int oy, int oz) {
    DrawVertex* d = (DrawVertex*)memalign(16, (size_t)n * sizeof(DrawVertex));
    if (!d) return 0;
    for (int i = 0; i < n; i++) {
        d[i].u = uvQ(s[i].u); d[i].v = uvQ(s[i].v); d[i].color = s[i].color;
        d[i].x = posQ(s[i].x - ox); d[i].y = posQ(s[i].y - oy); d[i].z = posQ(s[i].z - oz); d[i].w = 0;
    }
    sceKernelDcacheWritebackInvalidateRange(d, (size_t)n * sizeof(DrawVertex));
    return d;
}

extern int g_diagMode;

float g_relBaseX = 0.0f, g_relBaseY = 0.0f, g_relBaseZ = 0.0f;

#define SEAM_OVERSCALE_OPAQUE (32768.0f / 32753.0f)
#define SEAM_OVERSCALE_TRANS  (32768.0f / 32763.0f)

static inline void chunkSetModel(const ChunkSection* s, float scaleMul) {
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    ScePspFVector3 t = { (float)s->ox - g_relBaseX, (float)s->oy - g_relBaseY, (float)s->oz - g_relBaseZ };
    sceGumTranslate(&t);
    float sm = POS_MODEL_SCALE * scaleMul;
    ScePspFVector3 sc = { sm, sm, sm };
    sceGumScale(&sc);
}

void chunkDrawSection(const ChunkSection* s) {
    if (s->vertexCount <= 0 || !s->mesh) return;
    chunkSetModel(s, SEAM_OVERSCALE_OPAQUE);
    const unsigned int fmt = GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_3D;
    if (g_diagMode == 2) {
        int per = (s->vertexCount / 4 / 3) * 3;
        if (per > 0) {
            int off = 0;
            for (int k = 0; k < 3; k++) {
                sceGumDrawArray(GU_TRIANGLES, fmt, per, 0, s->mesh + off);
                off += per;
            }
            sceGumDrawArray(GU_TRIANGLES, fmt, s->vertexCount - off, 0, s->mesh + off);
            return;
        }
    }
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

void chunkDrawNoMipSection(const ChunkSection* s) {
    if (s->noMipCount > 0 && s->noMip) {
        chunkSetModel(s, SEAM_OVERSCALE_OPAQUE);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_3D,
                        s->noMipCount, 0, s->noMip);
    }
}

void chunkFreeMesh(ChunkMesh* c) {
    for (int si = 0; si < N_SECTIONS; si++) {
        ChunkSection* s = &c->sec[si];
        if (s->mesh)   { free(s->mesh);   s->mesh = 0; }
        if (s->water)  { free(s->water);  s->water = 0; }
        if (s->leaves) { free(s->leaves); s->leaves = 0; }
        if (s->noMip)  { free(s->noMip);  s->noMip = 0; }
        s->vertexCount = s->waterCount = s->leavesCount = s->noMipCount = 0;
    }
}
