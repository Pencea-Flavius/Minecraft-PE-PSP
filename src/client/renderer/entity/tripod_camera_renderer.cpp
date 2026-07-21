
#include "client/renderer/entity/tripod_camera_renderer.h"
#include "world/entity/tripod_camera.h"
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/chunk/chunk.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "platform/path.h"
#include "util/mth.h"
#include <pspgu.h>
#include <pspgum.h>

extern World g_world;

static Texture s_tex;
static bool s_have = false, s_tried = false;

static const float DEG2RAD = 3.14159265f / 180.0f;

static int quad(ChunkVertex* v, int n, unsigned int c,
                float x0, float y0, float z0, float x1, float y1, float z1,
                float x2, float y2, float z2, float x3, float y3, float z3,
                float u0, float v0, float u1, float v1) {
    u0 /= 64.0f; u1 /= 64.0f; v0 /= 32.0f; v1 /= 32.0f;
    ChunkVertex a = { u0, v1, c, x0, y0, z0 };
    ChunkVertex b = { u1, v1, c, x1, y1, z1 };
    ChunkVertex d = { u1, v0, c, x2, y2, z2 };
    ChunkVertex e = { u0, v0, c, x3, y3, z3 };
    v[n++] = a; v[n++] = b; v[n++] = d;
    v[n++] = a; v[n++] = d; v[n++] = e;
    return n;
}

void TripodCameraRenderer::render(Entity* entity, float x, float y, float z,
                                  float , float ) {
    TripodCamera* cam = (TripodCamera*)entity;
    if (!s_tried) {
        s_have = textureLoad16(assetPath("data/images/item/camera.png"), &s_tex, GU_PSM_5551)
              || textureLoad16("data/images/item/camera.png", &s_tex, GU_PSM_5551);
        s_tried = true;
    }
    int br = lightRawAt(&g_world, Mth::floor(x), Mth::floor(y), Mth::floor(z));
    unsigned int c = g_brightColor[br];
    float fy = y - cam->heightOffset;

    if (s_have) textureBindNoMip(&s_tex);
    else        sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDisable(GU_CULL_FACE);

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    ScePspFVector3 tr = { x - g_relBaseX, fy - g_relBaseY, z - g_relBaseZ };
    sceGumTranslate(&tr);

    {
        ChunkVertex* v = (ChunkVertex*)sceGuGetMemory(12 * sizeof(ChunkVertex));
        int n = 0;
        const float r = 0.45f;
        n = quad(v, n, c, -r,0,-r,  r,0,r,  r,1,r,  -r,1,-r,  48,16, 64,32);
        n = quad(v, n, c, -r,0,r,   r,0,-r, r,1,-r, -r,1,r,   48,16, 64,32);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                        n, 0, v);
    }

    {

        ScePspFVector3 piv = { 0.0f, 18.0f / 16.0f, 0.0f };
        sceGumTranslate(&piv);
    }
    sceGumRotateY((180.0f + cam->yRot) * DEG2RAD);
    sceGumRotateX(cam->xRot * 0.5f * DEG2RAD);
    {
        const float s = 1.0f / 16.0f;
        float x0 = -4*s, x1 = 4*s;
        float y0 = -4*s, y1 = 4*s;
        float z0 = -6*s, z1 = 4*s;
        ChunkVertex* v = (ChunkVertex*)sceGuGetMemory(36 * sizeof(ChunkVertex));
        int n = 0;

        n = quad(v, n, c, x0,y0,z0, x1,y0,z0, x1,y1,z0, x0,y1,z0, 10,10, 18,18);
        n = quad(v, n, c, x1,y0,z1, x0,y0,z1, x0,y1,z1, x1,y1,z1, 28,10, 36,18);
        n = quad(v, n, c, x0,y0,z1, x0,y0,z0, x0,y1,z0, x0,y1,z1,  0,10, 10,18);
        n = quad(v, n, c, x1,y0,z0, x1,y0,z1, x1,y1,z1, x1,y1,z0, 18,10, 28,18);
        n = quad(v, n, c, x0,y1,z0, x1,y1,z0, x1,y1,z1, x0,y1,z1, 10,0,  18,10);
        n = quad(v, n, c, x0,y0,z1, x1,y0,z1, x1,y0,z0, x0,y0,z0, 18,0,  26,10);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                        n, 0, v);

        if (cam->activated && cam->life < 8 && cam->life > 0) {
            unsigned int fa = (unsigned int)(cam->life * 255 / 8);
            unsigned int fc = (fa << 24) | 0x00FFFFFFu;
            float fz = z0 - 0.04f;
            ChunkVertex* fv = (ChunkVertex*)sceGuGetMemory(6 * sizeof(ChunkVertex));
            const float fr = 0.55f;
            int fn = quad(fv, 0, fc, -fr,-fr,fz, fr,-fr,fz, fr,fr,fz, -fr,fr,fz, 48,0, 64,16);
            sceGumDrawArray(GU_TRIANGLES,
                            GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                            fn, 0, fv);
        }
    }

    sceGumPopMatrix();
    if (!s_have) sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_CULL_FACE);
}
