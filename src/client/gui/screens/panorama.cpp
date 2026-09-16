
#include "client/gui/screens/panorama.h"
#include "client/gui/hud.h"
#include "gpu/gu.h"
#include "gpu/texture.h"
#include "platform/path.h"
#include "platform/time.h"

#include <pspgu.h>
#include <pspgum.h>
#include <cmath>
#include <cstdio>

namespace {

const int FACES = 6;
Texture s_face[FACES];
bool    s_loaded = false;

bool    s_failed = false;
int     s_faceCount = 0;
float   s_spin   = 0.0f;
float   s_lastT  = 0.0f;

const float DEG2RAD = 3.14159265f / 180.0f;

const float PANORAMA_FOV    = 120.0f;
const float PANORAMA_ASPECT = 1.0f;

const int   PANORAMA_SUBDIV = 8;

bool loadFace(Texture* out, const char* rel) {
    return textureLoad16(assetPath(rel), out, GU_PSM_5650) ||
           textureLoad16(rel, out, GU_PSM_5650);
}

}

void panoramaSetLoaded(bool want) {
    if (want) {
        if (s_loaded || s_failed) return;

        if (s_faceCount == 0) textureForgetFailures();
        char rel[64];
        std::snprintf(rel, sizeof(rel), "data/images/gui/background/panorama_%d.png", s_faceCount);
        if (!loadFace(&s_face[s_faceCount], rel)) {

            for (int j = 0; j < s_faceCount; j++) textureFree(&s_face[j]);
            s_faceCount = 0;
            s_failed = true;
            return;
        }
        if (++s_faceCount == FACES) {
            s_loaded = true;
            s_lastT  = nowSeconds();
        }
    } else {

        const int held = s_loaded ? FACES : s_faceCount;
        for (int i = 0; i < held; i++) textureFree(&s_face[i]);
        s_faceCount = 0;
        s_loaded = false;

        s_failed = false;
    }
}

void panoramaLoadAll(void) {

    for (int i = 0; i < FACES && !s_loaded && !s_failed; i++) panoramaSetLoaded(true);
}

bool panoramaRender() {
    if (!s_loaded) return false;

    float now = nowSeconds();
    float dt  = now - s_lastT;
    s_lastT = now;
    if (dt < 0.0f) dt = 0.0f; else if (dt > 0.1f) dt = 0.1f;
    s_spin += dt * 30.0f;

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_FOG);

    sceGumMatrixMode(GU_PROJECTION);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    sceGumPerspective(PANORAMA_FOV, PANORAMA_ASPECT, 0.05f, 10.0f);
    sceGumMatrixMode(GU_VIEW);
    sceGumPushMatrix();
    sceGumLoadIdentity();

    const float pitch = sinf(s_spin / 400.0f) + 20.0f;
    const float yaw   = -s_spin * 0.1f;

    struct PanoVertex { float u, v; float x, y, z; };

    bool ok = true;
    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    for (int i = 0; i < FACES; i++) {

        sceGumLoadIdentity();
        sceGumRotateX(180.0f * DEG2RAD);
        sceGumRotateX(pitch * DEG2RAD);
        sceGumRotateY(yaw * DEG2RAD);
        switch (i) {
            case 1: sceGumRotateY( 90.0f * DEG2RAD); break;
            case 2: sceGumRotateY(180.0f * DEG2RAD); break;
            case 3: sceGumRotateY(-90.0f * DEG2RAD); break;
            case 4: sceGumRotateX( 90.0f * DEG2RAD); break;
            case 5: sceGumRotateX(-90.0f * DEG2RAD); break;
            default: break;
        }

        textureBind(&s_face[i]);

        sceGuTexFilter(GU_LINEAR, GU_LINEAR);

        sceGuColor(0xFFFFFFFFu);
        const int N = PANORAMA_SUBDIV;
        PanoVertex* q = (PanoVertex*)guFrameAlloc(N * N * 6 * sizeof(PanoVertex));

        if (!q) { ok = false; break; }
        int n = 0;
        for (int cy = 0; cy < N; cy++) {
            for (int cx = 0; cx < N; cx++) {
                const float u0 = (float)cx / N,       u1 = (float)(cx + 1) / N;
                const float v0 = (float)cy / N,       v1 = (float)(cy + 1) / N;
                const float x0 = u0 * 2.0f - 1.0f,    x1 = u1 * 2.0f - 1.0f;
                const float y0 = v0 * 2.0f - 1.0f,    y1 = v1 * 2.0f - 1.0f;

                q[n].u = u0; q[n].v = v0; q[n].x = x0; q[n].y = y0; q[n].z = 1.0f; n++;
                q[n].u = u1; q[n].v = v0; q[n].x = x1; q[n].y = y0; q[n].z = 1.0f; n++;
                q[n].u = u1; q[n].v = v1; q[n].x = x1; q[n].y = y1; q[n].z = 1.0f; n++;
                q[n].u = u0; q[n].v = v0; q[n].x = x0; q[n].y = y0; q[n].z = 1.0f; n++;
                q[n].u = u1; q[n].v = v1; q[n].x = x1; q[n].y = y1; q[n].z = 1.0f; n++;
                q[n].u = u0; q[n].v = v1; q[n].x = x0; q[n].y = y1; q[n].z = 1.0f; n++;
            }
        }
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, n, 0, q);
    }
    sceGumPopMatrix();
    sceGumMatrixMode(GU_VIEW);
    sceGumPopMatrix();
    sceGumMatrixMode(GU_PROJECTION);
    sceGumPopMatrix();
    sceGumMatrixMode(GU_MODEL);

    guOrtho();
    sceGuEnable(GU_CULL_FACE);
    sceGuEnable(GU_DEPTH_TEST);

    sceGuDisable(GU_DEPTH_TEST);
    guiFillGradient(0.0f, 0.0f, 480.0f, 272.0f, 0x59FFFFFFu, 0x59000000u);
    sceGuEnable(GU_DEPTH_TEST);
    return ok;
}
