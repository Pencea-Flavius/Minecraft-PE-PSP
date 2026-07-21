
#include "client/renderer/entity/pig_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/entity/mob.h"
#include "gpu/texture.h"
#include <math.h>
#include <pspgu.h>

static const float DEG2RAD = 3.14159265f / 180.0f;

enum { P_HEAD, P_SNOUT, P_BODY, P_LEG0, P_LEG1, P_LEG2, P_LEG3, P_COUNT };
static MobPart parts[P_COUNT];
static bool    g_built = false;
static Texture g_tex;
static bool    g_have = false;

static void build() {
    if (g_built) return;
    mobBuildBox(parts[P_HEAD].base, -4,-4,-8,  4,4,0,  0,0,  8,8,8, false, 0);
    parts[P_HEAD].px = 0; parts[P_HEAD].py = 12; parts[P_HEAD].pz = -6;
    mobBuildBox(parts[P_SNOUT].base, -2,0,-9,  2,3,-8, 16,16, 4,3,1, false, 0);
    parts[P_SNOUT].px = 0; parts[P_SNOUT].py = 12; parts[P_SNOUT].pz = -6;
    parts[P_HEAD].head = parts[P_SNOUT].head = true;
    mobBuildBox(parts[P_BODY].base, -5,-10,-7, 5,6,1, 28,8, 10,16,8, false, 0);
    parts[P_BODY].px = 0; parts[P_BODY].py = 11; parts[P_BODY].pz = 2;
    const float lp[4][3] = { {-3,18,7}, {3,18,7}, {-3,18,-5}, {3,18,-5} };
    for (int i = 0; i < 4; i++) {
        mobBuildBox(parts[P_LEG0+i].base, -2,0,-2, 2,6,2, 0,16, 4,6,4, false, 0);
        parts[P_LEG0+i].px = lp[i][0]; parts[P_LEG0+i].py = lp[i][1]; parts[P_LEG0+i].pz = lp[i][2];
    }
    g_built = true;
}

PigRenderer::PigRenderer() { shadowRadius = 0.5f; shadowStrength = 1.0f; }

void PigRenderer::render(Entity* e, float x, float y, float z, float rot, float a) {
    if (!g_have) { g_have = textureLoad16("data/images/mob/pig.png", &g_tex, GU_PSM_5551); if (!g_have) return; }
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

    parts[P_HEAD].xRot = -ipitch * DEG2RAD; parts[P_HEAD].yRot = -dHead * DEG2RAD; parts[P_HEAD].zRot = 0;
    parts[P_SNOUT].xRot = parts[P_HEAD].xRot; parts[P_SNOUT].yRot = parts[P_HEAD].yRot; parts[P_SNOUT].zRot = 0;
    parts[P_BODY].xRot = 90.0f * DEG2RAD; parts[P_BODY].yRot = parts[P_BODY].zRot = 0;
    parts[P_LEG0].xRot =  pend; parts[P_LEG1].xRot = -pend; parts[P_LEG2].xRot = -pend; parts[P_LEG3].xRot = pend;
    for (int i = P_LEG0; i <= P_LEG3; i++) { parts[i].yRot = parts[i].zRot = 0; }

    mobRenderParts(mob, parts, P_COUNT, &g_tex, x, y, z, ibody, a, 0xFFFFFFFFu, 4.0f, 4.0f);
}
