#include "client/renderer/level/frustum.h"
#include <math.h>
#include <pspgu.h>
#include <pspgum.h>

static float g_frustum[6][4];

static void normalizePlane(int side) {
    float* p = g_frustum[side];
    float mag = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
    if (mag > 0.0f) { p[0] /= mag; p[1] /= mag; p[2] /= mag; p[3] /= mag; }
}

void worldSetFrustumCamera(float, float, float, float, float, float,
                           float, float, float, float, float) {
    ScePspFMatrix4 proj, view, clip;
    sceGumMatrixMode(GU_PROJECTION); sceGumStoreMatrix(&proj);
    sceGumMatrixMode(GU_VIEW);       sceGumStoreMatrix(&view);
    sceGumMatrixMode(GU_MODEL);
    gumMultMatrix(&clip, &proj, &view);
    const float* c = (const float*)&clip;

    g_frustum[0][0]=c[3]-c[0];  g_frustum[0][1]=c[7]-c[4];  g_frustum[0][2]=c[11]-c[8];  g_frustum[0][3]=c[15]-c[12]; normalizePlane(0);
    g_frustum[1][0]=c[3]+c[0];  g_frustum[1][1]=c[7]+c[4];  g_frustum[1][2]=c[11]+c[8];  g_frustum[1][3]=c[15]+c[12]; normalizePlane(1);
    g_frustum[2][0]=c[3]+c[1];  g_frustum[2][1]=c[7]+c[5];  g_frustum[2][2]=c[11]+c[9];  g_frustum[2][3]=c[15]+c[13]; normalizePlane(2);
    g_frustum[3][0]=c[3]-c[1];  g_frustum[3][1]=c[7]-c[5];  g_frustum[3][2]=c[11]-c[9];  g_frustum[3][3]=c[15]-c[13]; normalizePlane(3);
    g_frustum[4][0]=c[3]-c[2];  g_frustum[4][1]=c[7]-c[6];  g_frustum[4][2]=c[11]-c[10]; g_frustum[4][3]=c[15]-c[14]; normalizePlane(4);
    g_frustum[5][0]=c[3]+c[2];  g_frustum[5][1]=c[7]+c[6];  g_frustum[5][2]=c[11]+c[10]; g_frustum[5][3]=c[15]+c[14]; normalizePlane(5);
}

static bool cubeInFrustum(float x1, float y1, float z1, float x2, float y2, float z2) {
    for (int i = 0; i < 6; i++) {
        const float* p = g_frustum[i];
        if (p[0]*x1 + p[1]*y1 + p[2]*z1 + p[3] > 0) continue;
        if (p[0]*x2 + p[1]*y1 + p[2]*z1 + p[3] > 0) continue;
        if (p[0]*x1 + p[1]*y2 + p[2]*z1 + p[3] > 0) continue;
        if (p[0]*x2 + p[1]*y2 + p[2]*z1 + p[3] > 0) continue;
        if (p[0]*x1 + p[1]*y1 + p[2]*z2 + p[3] > 0) continue;
        if (p[0]*x2 + p[1]*y1 + p[2]*z2 + p[3] > 0) continue;
        if (p[0]*x1 + p[1]*y2 + p[2]*z2 + p[3] > 0) continue;
        if (p[0]*x2 + p[1]*y2 + p[2]*z2 + p[3] > 0) continue;
        return false;
    }
    return true;
}

bool sectionVisible(const ChunkMesh* c, const ChunkSection* s) {
    return cubeInFrustum((float)c->ox, s->by0, (float)c->oz,
                         (float)(c->ox + CHUNK_SX), s->by1, (float)(c->oz + CHUNK_SZ));
}

bool columnVisible(const ChunkMesh* c) {
    return cubeInFrustum((float)c->ox, 0.0f, (float)c->oz,
                         (float)(c->ox + CHUNK_SX), (float)WORLD_H, (float)(c->oz + CHUNK_SZ));
}

float dist3D_sq(float camX, float camY, float camZ,
                const ChunkMesh* c, float cy0, float cy1) {
    float dx = c->cx - camX;
    float dz = c->cz - camZ;
    float dy = 0.0f;
    if (camY < cy0) dy = cy0 - camY;
    else if (camY > cy1) dy = camY - cy1;
    return dx * dx + dy * dy + dz * dz;
}

extern float g_viewDist;

bool leafOpaqueBand(const ChunkMesh* c, int y0, int y1,
                    float camX, float camY, float camZ, bool fancyGraphics) {
    if (!fancyGraphics) return true;

    if (g_viewDist <= 48.0f) return false;
    float r = 32.0f;
    return dist3D_sq(camX, camY, camZ, c, (float)y0, (float)y1) >= r * r;
}

bool leafCullBand(const ChunkMesh* c, int y0, int y1,
                  float camX, float camY, float camZ, bool fancyGraphics) {
    (void)fancyGraphics;

    if (g_viewDist <= 32.0f) return false;
    float r = (g_viewDist <= 48.0f) ? 32.0f : 16.0f;
    return dist3D_sq(camX, camY, camZ, c, (float)y0, (float)y1) >= r * r;
}
