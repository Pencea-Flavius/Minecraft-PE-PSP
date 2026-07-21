
#include "client/renderer/item_model.h"
#include "client/renderer/item_hand.h"
#include "world/level/chunk/chunk.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

extern Texture g_terrain;
extern bool    g_haveTerrain;

static const float DEG2RAD = 3.14159265f / 180.0f;

bool ItemModelRenderer::build(short id, unsigned char data, int bowStage) {
    if (id != m_id || data != m_data || bowStage != m_bowStage) {
        m_id = id; m_data = data; m_bowStage = bowStage;
        m_flat = itemIsFlat2D(id);
        const int cap = (int)(sizeof(m_base) / sizeof(m_base[0]));
        m_count = m_flat ? itemBuildFlatMesh(id, data, m_base, bowStage, cap)
                         : itemBuildBlockMesh(id, data, m_base);

        m_tex = m_flat ? itemFlatTexture(id, data)
                       : (g_haveTerrain ? &g_terrain : (const Texture*)0);
    }
    return m_count > 0;
}

void ItemModelRenderer::draw(unsigned int brCol, bool noMip) {
    if (m_count <= 0) return;

    ChunkVertex* v = (ChunkVertex*)sceGuGetMemory(m_count * sizeof(ChunkVertex));
    for (int i = 0; i < m_count; i++) {
        v[i] = m_base[i];
        v[i].color = mulColor(m_base[i].color, brCol);
    }
    if (m_tex) { noMip ? textureBindNoMip(m_tex) : textureBind(m_tex); }
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    m_count, 0, v);
}

void ItemModelRenderer::drawMesh(ChunkVertex* m, int n, unsigned int brCol,
                                 const Texture* tex, bool noMip) {
    if (n <= 0) return;
    if (brCol != 0xFFFFFFFFu)
        for (int i = 0; i < n; i++) m[i].color = mulColor(m[i].color, brCol);
    if (tex) { noMip ? textureBindNoMip(tex) : textureBind(tex); }

    void* v = guFrameCopy(m, n * sizeof(ChunkVertex));
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    n, 0, v);
}

void ItemModelRenderer::applyFlatPreTransform() {
    ScePspFVector3 t1 = { 0.0f, -0.3f, 0.0f };            sceGumTranslate(&t1);
    ScePspFVector3 sc = { 1.5f, 1.5f, 1.5f };             sceGumScale(&sc);
    sceGumRotateY(50.0f * DEG2RAD);
    sceGumRotateZ(335.0f * DEG2RAD);
    ScePspFVector3 t2 = { -15.0f/16.0f, -1.0f/16.0f, 0.0f }; sceGumTranslate(&t2);
}
