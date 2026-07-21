
#include "client/renderer/entity/arrow_renderer.h"
#include "world/entity/arrow.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "platform/path.h"
#include "util/mth.h"
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <cmath>

struct AVert { float u, v; unsigned int color; float x, y, z; };

static Texture s_tex;
static bool s_loaded = false;
static bool s_ok = false;

static void ensureTex() {
    if (s_loaded) return;
    s_loaded = true;
    s_ok = textureLoad16(assetPath("data/images/item/arrows.png"), &s_tex, GU_PSM_5551)
        || textureLoad16("data/images/item/arrows.png", &s_tex, GU_PSM_5551);
}

static inline int quad(AVert* m, int n, unsigned int col,
                       float ax,float ay,float az,float au,float av,
                       float bx,float by,float bz,float bu,float bv,
                       float cx,float cy,float cz,float cu,float cv,
                       float dx,float dy,float dz,float du,float dv) {
    m[n++] = (AVert){au,av,col,ax,ay,az};
    m[n++] = (AVert){bu,bv,col,bx,by,bz};
    m[n++] = (AVert){cu,cv,col,cx,cy,cz};
    m[n++] = (AVert){au,av,col,ax,ay,az};
    m[n++] = (AVert){cu,cv,col,cx,cy,cz};
    m[n++] = (AVert){du,dv,col,dx,dy,dz};
    return n;
}

static void drawMesh(AVert* m, int n) {

    void* v = guFrameCopy(m, n * sizeof(AVert));
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    n, 0, v);
}

void ArrowRenderer::render(Entity* entity, float x, float y, float z, float , float a) {
    ensureTex();
    if (!s_ok) return;

    Arrow* arrow = (Arrow*)entity;

    float br = arrow->getBrightness(a);
    if (br < 0.35f) br = 0.35f;
    if (br > 1) br = 1;
    unsigned int c = (unsigned char)(br * 255);
    unsigned int col = 0xFF000000u | (c << 16) | (c << 8) | c;

    const float DEG = Mth::PI / 180.0f;
    float yawI   = arrow->yRotO + (arrow->yRot - arrow->yRotO) * a;
    float pitchI = arrow->xRotO + (arrow->xRot - arrow->xRotO) * a;

    float u0 = 0.0f, u1 = 16 / 32.0f, v0 = 0.0f, v1 = 5 / 32.0f;
    float u02 = 0.0f, u12 = 5 / 32.0f, v02 = 5 / 32.0f, v12 = 10 / 32.0f;
    const float ss = 0.9f / 16.0f;

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    ScePspFVector3 tr = { x - g_relBaseX, y - g_relBaseY, z - g_relBaseZ };
    sceGumTranslate(&tr);
    sceGumRotateY((yawI - 90.0f) * DEG);
    sceGumRotateZ(pitchI * DEG);

    float shake = arrow->shakeTime - a;
    if (shake > 0) {
        float pw = -sinf(shake * 3.0f) * shake;
        sceGumRotateZ(pw * DEG);
    }

    sceGumRotateX(45.0f * DEG);
    ScePspFVector3 sc = { ss, ss, ss };
    sceGumScale(&sc);
    ScePspFVector3 t2 = { -4, 0, 0 };
    sceGumTranslate(&t2);

    sceGuDisable(GU_CULL_FACE);
    textureBind(&s_tex);

    static AVert fletch[6];
    int fn = quad(fletch, 0, col,
        -7,-2,-2, u02,v02,
        -7,-2,+2, u12,v02,
        -7,+2,+2, u12,v12,
        -7,+2,-2, u02,v12);
    drawMesh(fletch, fn);

    static AVert shaft[6];
    int sn = quad(shaft, 0, col,
        -8,-2,0, u0,v0,
        +8,-2,0, u1,v0,
        +8,+2,0, u1,v1,
        -8,+2,0, u0,v1);
    for (int i = 0; i < 4; i++) {
        sceGumRotateX(90.0f * DEG);
        drawMesh(shaft, sn);
    }

    sceGuEnable(GU_CULL_FACE);
    sceGumPopMatrix();
}
