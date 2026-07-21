
#include "client/renderer/entity/cow_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/entity/mob.h"
#include "gpu/texture.h"
#include <math.h>
#include <pspgu.h>

static const float DEG2RAD = 3.14159265f / 180.0f;

enum { P_HEAD, P_HORN0, P_HORN1, P_BODY, P_UDDER, P_LEG0, P_LEG1, P_LEG2, P_LEG3, P_COUNT };
static MobPart parts[P_COUNT];
static bool    g_built = false;
static Texture g_tex;
static bool    g_have = false;

static void build() {
    if (g_built) return;

    mobBuildBox(parts[P_HEAD].base, -4,-4,-6, 4,4,0, 0,0, 8,8,6, false, 0);
    parts[P_HEAD].px = 0; parts[P_HEAD].py = 4; parts[P_HEAD].pz = -8;

    mobBuildBox(parts[P_HORN0].base, -5,-5,-4, -4,-2,-3, 22,0, 1,3,1, false, 0);
    parts[P_HORN0].px = 0; parts[P_HORN0].py = 4; parts[P_HORN0].pz = -8;
    mobBuildBox(parts[P_HORN1].base,  4,-5,-4,  5,-2,-3, 22,0, 1,3,1, false, 0);
    parts[P_HORN1].px = 0; parts[P_HORN1].py = 4; parts[P_HORN1].pz = -8;
    parts[P_HEAD].head = parts[P_HORN0].head = parts[P_HORN1].head = true;

    mobBuildBox(parts[P_BODY].base, -6,-10,-7, 6,8,3, 18,4, 12,18,10, false, 0);
    parts[P_BODY].px = 0; parts[P_BODY].py = 5; parts[P_BODY].pz = 2;

    mobBuildBox(parts[P_UDDER].base, -2,2,-8, 2,8,-7, 52,0, 4,6,1, false, 0);
    parts[P_UDDER].px = 0; parts[P_UDDER].py = 5; parts[P_UDDER].pz = 2;

    const float lp[4][3] = { {-4,12,7}, {4,12,7}, {-4,12,-6}, {4,12,-6} };
    for (int i = 0; i < 4; i++) {
        mobBuildBox(parts[P_LEG0+i].base, -2,0,-2, 2,12,2, 0,16, 4,12,4, false, 0);
        parts[P_LEG0+i].px = lp[i][0]; parts[P_LEG0+i].py = lp[i][1]; parts[P_LEG0+i].pz = lp[i][2];
    }
    g_built = true;
}

CowRenderer::CowRenderer() { shadowRadius = 0.7f; shadowStrength = 1.0f; }

void CowRenderer::render(Entity* e, float x, float y, float z, float rot, float a) {
    if (!g_have) { g_have = textureLoad16("data/images/mob/cow.png", &g_tex, GU_PSM_5551); if (!g_have) return; }
    build();
    Mob* mob = (Mob*)e;

    float dBody = mob->yBodyRot - mob->yBodyRotO;
    while (dBody > 180.0f) dBody -= 360.0f; while (dBody < -180.0f) dBody += 360.0f;
    float ibody = mob->yBodyRotO + dBody * a;
    float dHead = rot - ibody; while (dHead > 180.0f) dHead -= 360.0f; while (dHead < -180.0f) dHead += 360.0f;
    float ipitch = mob->xRotO + (mob->xRot - mob->xRotO) * a;
    float ws = mob->walkAnimSpeedO + (mob->walkAnimSpeed - mob->walkAnimSpeedO) * a; if (ws > 1.0f) ws = 1.0f;
    float wp = mob->walkAnimPos - mob->walkAnimSpeed * (1.0f - a);
    if (mob->isBaby()) wp *= 3.0f;
    float pend = cosf(wp * 0.6662f) * 1.4f * ws;

    float hx = -ipitch * DEG2RAD, hy = -dHead * DEG2RAD;
    parts[P_HEAD].xRot = hx; parts[P_HEAD].yRot = hy; parts[P_HEAD].zRot = 0;
    parts[P_HORN0].xRot = hx; parts[P_HORN0].yRot = hy; parts[P_HORN0].zRot = 0;
    parts[P_HORN1].xRot = hx; parts[P_HORN1].yRot = hy; parts[P_HORN1].zRot = 0;
    parts[P_BODY].xRot = 90.0f * DEG2RAD; parts[P_BODY].yRot = parts[P_BODY].zRot = 0;
    parts[P_UDDER].xRot = 90.0f * DEG2RAD; parts[P_UDDER].yRot = parts[P_UDDER].zRot = 0;
    parts[P_LEG0].xRot =  pend; parts[P_LEG1].xRot = -pend; parts[P_LEG2].xRot = -pend; parts[P_LEG3].xRot = pend;
    for (int i = P_LEG0; i <= P_LEG3; i++) { parts[i].yRot = parts[i].zRot = 0; }

    mobRenderParts(mob, parts, P_COUNT, &g_tex, x, y, z, ibody, a, 0xFFFFFFFFu, 8.0f, 4.0f);
}
