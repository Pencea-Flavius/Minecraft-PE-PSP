
#include "client/renderer/item_model.h"
#include "client/renderer/item_hand.h"
#include "world/level/chunk/chunk.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "client/renderer/tileentity/tile_entity_renderer.h"
#include "client/renderer/entity/mob_model.h"
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
              : (id == BLOCK_CHEST) ? chestModelTexture()
              : (g_haveTerrain ? &g_terrain : (const Texture*)0);
    }
    return m_count > 0;
}

static void itemLightMatrix(ScePspFMatrix4* m, const float* toWorld) {
    sceGumStoreMatrix(m);
    if (!toWorld) return;
    const float* a = toWorld;
    const float* b = (const float*)m;
    float c[16];
    for (int i = 0; i < 16; i++) c[i] = b[i];
    for (int col = 0; col < 3; col++)
        for (int row = 0; row < 3; row++)
            c[col * 4 + row] = a[row] * b[col * 4]
                             + a[4 + row] * b[col * 4 + 1]
                             + a[8 + row] * b[col * 4 + 2];
    for (int i = 0; i < 16; i++) ((float*)m)[i] = c[i];
}

static void itemShadeCopy(ChunkVertex* out, const ChunkVertex* base, int n,
                          unsigned int brCol, const float* lmf) {
    static const float kIdentity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    if (!lmf) lmf = kIdentity;
    int i = 0;
    for (; i + 2 < n; i += 3) {
        const ChunkVertex& a = base[i];
        const ChunkVertex& b = base[i + 1];
        const ChunkVertex& c = base[i + 2];
        const float e1x = b.x - a.x, e1y = b.y - a.y, e1z = b.z - a.z;
        const float e2x = c.x - a.x, e2y = c.y - a.y, e2z = c.z - a.z;
        const float nrm[3] = { e1y * e2z - e1z * e2y,
                               e1z * e2x - e1x * e2z,
                               e1x * e2y - e1y * e2x };

        const unsigned int lit = mobDirLitColor(lmf, nrm, brCol);
        for (int k = 0; k < 3; k++) {
            out[i + k] = base[i + k];
            out[i + k].color = mulColor(base[i + k].color, lit);
        }
    }
    for (; i < n; i++) {
        out[i] = base[i];
        out[i].color = mulColor(base[i].color, brCol);
    }
}

void ItemModelRenderer::draw(unsigned int brCol, bool noMip, bool priority,
                             const float* toWorld) {
    if (m_count <= 0) return;

    ChunkVertex* v = (ChunkVertex*)(priority ? guFrameAllocPriority(m_count * sizeof(ChunkVertex))
                                             : guFrameAlloc(m_count * sizeof(ChunkVertex)));
    if (!v) return;
    ScePspFMatrix4 lm;
    itemLightMatrix(&lm, toWorld);
    itemShadeCopy(v, m_base, m_count, brCol, (const float*)&lm);
    if (m_tex) { noMip ? textureBindNoMip(m_tex) : textureBind(m_tex); }

    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    m_count, 0, v);
}

namespace {
struct SharedItem {
    short          id;
    unsigned char  data;
    unsigned int   col;
    int            count;
    void*          verts;
    const Texture* tex;
    bool           flat;
};
const int SHARED_SLOTS = 8;
SharedItem   s_shared[SHARED_SLOTS];
int          s_sharedN  = 0;
unsigned int s_sharedFrame = 0;
}

bool ItemModelRenderer::buildShared(short id, unsigned char data, unsigned int brCol) {
    if (s_sharedFrame != guFrameId()) { s_sharedFrame = guFrameId(); s_sharedN = 0; }

    m_sharedSlot = -1;
    int loose = -1;
    for (int i = 0; i < s_sharedN; i++) {
        if (s_shared[i].id != id || s_shared[i].data != data) continue;
        if (s_shared[i].col == brCol) {
            m_sharedSlot = i;
            return s_shared[i].count > 0;
        }
        if (loose < 0) loose = i;
    }

    if (loose >= 0 && s_sharedN >= SHARED_SLOTS / 2) {
        m_sharedSlot = loose;
        return s_shared[loose].count > 0;
    }

    if (s_sharedN >= SHARED_SLOTS) {
        m_sharedSlot = -2;
        m_fallbackCol = brCol;
        return build(id, data, -1);
    }

    if (!build(id, data, -1)) {

        s_shared[s_sharedN].id = id; s_shared[s_sharedN].data = data;
        s_shared[s_sharedN].col = brCol;
        s_shared[s_sharedN].count = 0; s_shared[s_sharedN].verts = 0;
        s_shared[s_sharedN].tex = 0;  s_shared[s_sharedN].flat = false;
        s_sharedN++;
        return false;
    }

    ChunkVertex* v = (ChunkVertex*)guFrameAlloc(m_count * sizeof(ChunkVertex));
    if (!v) return false;

    itemShadeCopy(v, m_base, m_count, brCol, 0);
    s_shared[s_sharedN].id = id; s_shared[s_sharedN].data = data;
    s_shared[s_sharedN].col = brCol;
    s_shared[s_sharedN].count = m_count; s_shared[s_sharedN].verts = v;
    s_shared[s_sharedN].tex = m_tex;     s_shared[s_sharedN].flat = m_flat;
    m_sharedSlot = s_sharedN++;
    return true;
}

void ItemModelRenderer::drawShared(bool noMip) {
    if (m_sharedSlot == -2) { draw(m_fallbackCol, noMip); return; }
    if (m_sharedSlot < 0) return;
    const SharedItem& s = s_shared[m_sharedSlot];
    if (s.count <= 0 || !s.verts) return;
    if (s.tex) { noMip ? textureBindNoMip(s.tex) : textureBind(s.tex); }
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    s.count, 0, s.verts);
}

void ItemModelRenderer::drawMesh(ChunkVertex* m, int n, unsigned int brCol,
                                 const Texture* tex, bool noMip) {
    if (n <= 0) return;
    if (brCol != 0xFFFFFFFFu)
        for (int i = 0; i < n; i++) m[i].color = mulColor(m[i].color, brCol);
    if (tex) { noMip ? textureBindNoMip(tex) : textureBind(tex); }

    void* v = guFrameCopy(m, n * sizeof(ChunkVertex));
    if (!v) return;
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
