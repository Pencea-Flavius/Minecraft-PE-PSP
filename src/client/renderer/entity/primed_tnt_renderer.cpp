
#include "client/renderer/entity/primed_tnt_renderer.h"
#include "world/entity/primed_tnt.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "util/mth.h"
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

extern World g_world;
extern bool g_haveTerrain;
extern Texture g_terrain;

struct PosVertex { float x, y, z; };
static ChunkVertex s_mesh[36];
static PosVertex   s_overlay[36];
static int s_meshCount = 0;

void PrimedTntRenderer::render(Entity* entity, float x, float y, float z, float , float a) {
    if (!g_haveTerrain) return;
    PrimedTnt* tnt = (PrimedTnt*)entity;

    if (s_meshCount == 0) {

        s_meshCount = emitPartialBox(&g_world, 0, 150, 0, (unsigned char)BLOCK_TNT, 0,
                                     0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                                     0, 0, s_mesh, 0);
        for (int i = 0; i < s_meshCount && i < 36; i++) {
            s_overlay[i].x = s_mesh[i].x; s_overlay[i].y = s_mesh[i].y; s_overlay[i].z = s_mesh[i].z;
        }
    }
    if (s_meshCount <= 0) return;

    int br = lightRawAt(&g_world, Mth::floor(x), Mth::floor(y), Mth::floor(z));
    unsigned int brCol = g_brightColor[br];

    ChunkVertex* mesh = (ChunkVertex*)sceGuGetMemory(s_meshCount * sizeof(ChunkVertex));
    for (int i = 0; i < s_meshCount; i++) {
        mesh[i] = s_mesh[i];
        mesh[i].color = mulColor(s_mesh[i].color, brCol);
    }

    float lifeF = tnt->life - a + 1.0f;

    float s = 1.0f;
    if (lifeF < 10.0f) {
        float g = 1.0f - lifeF / 10.0f;
        if (g < 0) g = 0; if (g > 1) g = 1;
        g = g * g; g = g * g;
        s = 1.0f + g * 0.3f;
    }

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();

    ScePspFVector3 tr = { x - g_relBaseX, y - g_relBaseY, z - g_relBaseZ };
    sceGumTranslate(&tr);
    ScePspFVector3 sc = { s, s, s };
    sceGumScale(&sc);
    ScePspFVector3 c = { -0.5f, -150.5f, -0.5f };
    sceGumTranslate(&c);

    textureBind(&g_terrain);
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    s_meshCount, 0, mesh);

    if (((tnt->life / 5) & 1) == 0) {
        float br = (1.0f - lifeF / 100.0f) * 0.8f;
        if (br < 0) br = 0; if (br > 0.8f) br = 0.8f;
        unsigned int col = ((unsigned int)(br * 255.0f) << 24) | 0x00FFFFFFu;
        sceGuDisable(GU_TEXTURE_2D);
        sceGuColor(col);
        void* ov = guFrameCopy(s_overlay, s_meshCount * sizeof(PosVertex));
        sceGumDrawArray(GU_TRIANGLES, GU_VERTEX_32BITF | GU_TRANSFORM_3D, s_meshCount, 0, ov);
        sceGuColor(0xFFFFFFFFu);
        sceGuEnable(GU_TEXTURE_2D);
    }

    sceGumPopMatrix();
}
