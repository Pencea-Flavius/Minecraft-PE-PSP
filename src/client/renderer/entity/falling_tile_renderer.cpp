
#include "client/renderer/entity/falling_tile_renderer.h"
#include "world/entity/falling_tile.h"
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

static ChunkVertex s_mesh[36];
static int s_meshCount = 0;
static int s_lastTile = -1;
static int s_lastData = -1;

void FallingTileRenderer::render(Entity* entity, float x, float y, float z, float , float ) {
    if (!g_haveTerrain) return;
    FallingTile* ft = (FallingTile*)entity;
    if (ft->tile == 0) return;

    if (ft->tile != s_lastTile || ft->data != s_lastData) {
        s_lastTile = ft->tile; s_lastData = ft->data;

        s_meshCount = emitPartialBox(&g_world, 0, 150, 0, (unsigned char)ft->tile, (unsigned char)ft->data,
                                     0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                                     0, 0, s_mesh, 0);
    }
    if (s_meshCount <= 0) return;

    int br = lightRawAt(&g_world, Mth::floor(x), Mth::floor(y), Mth::floor(z));
    unsigned int brCol = g_brightColor[br];

    ChunkVertex* mesh = (ChunkVertex*)sceGuGetMemory(s_meshCount * sizeof(ChunkVertex));
    for (int i = 0; i < s_meshCount; i++) {
        mesh[i] = s_mesh[i];
        mesh[i].color = mulColor(s_mesh[i].color, brCol);
    }

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();

    ScePspFVector3 tr = { x - 0.5f - g_relBaseX, y - 150.5f - g_relBaseY, z - 0.5f - g_relBaseZ };
    sceGumTranslate(&tr);

    textureBind(&g_terrain);
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    s_meshCount, 0, mesh);

    sceGumPopMatrix();
}
