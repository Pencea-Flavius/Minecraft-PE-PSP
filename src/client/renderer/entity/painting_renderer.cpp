
#include "client/renderer/entity/painting_renderer.h"
#include "world/entity/painting.h"
#include "world/entity/motive.h"
#include "world/level/chunk/chunk.h"
#include "client/renderer/entity/mob_model.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "platform/dcache.h"
#include "platform/path.h"
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <malloc.h>

struct PVert { float u, v; float x, y, z; };

static PVert*        s_mesh[MOTIVE_COUNT];
static unsigned short s_meshLen[MOTIVE_COUNT];

static Texture s_art;
static bool s_artLoaded = false;
static bool s_artOk = false;

static void ensureArt() {
    if (s_artLoaded) return;
    s_artLoaded = true;

    s_artOk = textureLoad16(assetPath("data/images/art/kz.png"), &s_art, GU_PSM_5650)
           || textureLoad16("data/images/art/kz.png", &s_art, GU_PSM_5650);
}

static inline int quad(PVert* m, int n,
                       float ax,float ay,float az,float au,float av,
                       float bx,float by,float bz,float bu,float bv,
                       float cx,float cy,float cz,float cu,float cv,
                       float dx,float dy,float dz,float du,float dv) {
    m[n++] = (PVert){au,av,ax,ay,az};
    m[n++] = (PVert){bu,bv,bx,by,bz};
    m[n++] = (PVert){cu,cv,cx,cy,cz};
    m[n++] = (PVert){au,av,ax,ay,az};
    m[n++] = (PVert){cu,cv,cx,cy,cz};
    m[n++] = (PVert){du,dv,dx,dy,dz};
    return n;
}

static PVert* getMesh(int motiveIdx, int* outCount) {
    if (motiveIdx < 0 || motiveIdx >= MOTIVE_COUNT) return 0;
    if (s_mesh[motiveIdx]) { *outCount = s_meshLen[motiveIdx]; return s_mesh[motiveIdx]; }

    const Motive& mo = kMotives[motiveIdx];
    const int w = mo.w, h = mo.h, uo = mo.uo, vo = mo.vo;
    const int verts = (w / 16) * (h / 16) * 36;

    PVert* m = (PVert*)memalign(64, (size_t)verts * sizeof(PVert));
    if (!m) return 0;

    const float edge = 0.5f;
    const float xx0 = -w / 2.0f;
    const float yy0 = -h / 2.0f;

    float bu0 = (12*16) / 256.0f, bu1 = (12*16 + 16) / 256.0f;
    float bv0 = 0.0f,             bv1 = 16 / 256.0f;

    float uu0 = (12*16) / 256.0f, uu1 = (12*16 + 16) / 256.0f;
    float uv0 = 0.5f / 256.0f,    uv1 = 0.5f / 256.0f;

    float su0 = (12*16 + 0.5f) / 256.0f, su1 = (12*16 + 0.5f) / 256.0f;
    float sv0 = 0.0f,                    sv1 = 16 / 256.0f;

    int n = 0;

    for (int xs = 0; xs < w / 16; xs++) {
        for (int ys = 0; ys < h / 16; ys++) {
            float x0 = xx0 + (xs + 1) * 16;
            float x1 = xx0 + (xs) * 16;
            float y0 = yy0 + (ys + 1) * 16;
            float y1 = yy0 + (ys) * 16;

            float fu0 = (uo + w - (xs) * 16) / 256.0f;
            float fu1 = (uo + w - (xs + 1) * 16) / 256.0f;
            float fv0 = (vo + h - (ys) * 16) / 256.0f;
            float fv1 = (vo + h - (ys + 1) * 16) / 256.0f;

            n = quad(m, n,
                x0, y1, -edge, fu1, fv0,
                x1, y1, -edge, fu0, fv0,
                x1, y0, -edge, fu0, fv1,
                x0, y0, -edge, fu1, fv1);

            n = quad(m, n,
                x0, y0,  edge, bu0, bv0,
                x1, y0,  edge, bu1, bv0,
                x1, y1,  edge, bu1, bv1,
                x0, y1,  edge, bu0, bv1);

            n = quad(m, n,
                x0, y0, -edge, uu0, uv0,
                x1, y0, -edge, uu1, uv0,
                x1, y0,  edge, uu1, uv1,
                x0, y0,  edge, uu0, uv1);

            n = quad(m, n,
                x0, y1,  edge, uu0, uv0,
                x1, y1,  edge, uu1, uv0,
                x1, y1, -edge, uu1, uv1,
                x0, y1, -edge, uu0, uv1);

            n = quad(m, n,
                x0, y0,  edge, su1, sv0,
                x0, y1,  edge, su1, sv1,
                x0, y1, -edge, su0, sv1,
                x0, y0, -edge, su0, sv0);

            n = quad(m, n,
                x1, y0, -edge, su1, sv0,
                x1, y1, -edge, su1, sv1,
                x1, y1,  edge, su0, sv1,
                x1, y0,  edge, su0, sv0);
        }
    }

    dcacheFlush(m, (size_t)n * sizeof(PVert));
    s_mesh[motiveIdx]    = m;
    s_meshLen[motiveIdx] = (unsigned short)n;
    *outCount = n;
    return m;
}

void PaintingRenderer::render(Entity* entity, float x, float y, float z, float rot, float a) {
    (void)a;
    ensureArt();
    if (!s_artOk) return;

    Painting* painting = (Painting*)entity;
    int n = 0;
    PVert* mesh = getMesh(painting->motive, &n);
    if (!mesh || n <= 0) return;

    unsigned int col = brightColorFloored(painting->getRawBrightness(), ENTITY_LIGHT_FLOOR);

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    ScePspFVector3 t = { x - g_relBaseX, y - g_relBaseY, z - g_relBaseZ };
    sceGumTranslate(&t);
    sceGumRotateY(rot * 3.14159265f / 180.0f);
    ScePspFVector3 s = { 1.0f / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f };
    sceGumScale(&s);

    ScePspFMatrix4 lm;
    sceGumStoreMatrix(&lm);
    static const float FRONT[3] = { 0.0f, 0.0f, -1.0f };
    col = mobDirLitColor((const float*)&lm, FRONT, col);

    sceGuDisable(GU_CULL_FACE);
    textureBind(&s_art);
    sceGuColor(col);
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    n, 0, mesh);
    sceGuColor(0xFFFFFFFFu);
    sceGuEnable(GU_CULL_FACE);

    sceGumPopMatrix();
}
