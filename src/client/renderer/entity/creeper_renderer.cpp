
#include "client/renderer/entity/creeper_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/entity/monster/creeper.h"
#include <math.h>
#include <pspgu.h>

static const float DEG2RAD = 3.14159265f / 180.0f;
static const float PIF     = 3.14159265f;

enum { P_HEAD, P_BODY, P_LEG0, P_LEG1, P_LEG2, P_LEG3, P_COUNT };
static MobPart parts[P_COUNT];
static bool    g_built = false;

static void build() {
    if (g_built) return;
    mobBuildBox(parts[P_HEAD].base, -4,-8,-4, 4,0,4,  0, 0, 8,8,8, false, 0);
    parts[P_HEAD].px = 0; parts[P_HEAD].py = 6; parts[P_HEAD].pz = 0;
    mobBuildBox(parts[P_BODY].base, -4,0,-2, 4,12,2, 16,16, 8,12,4, false, 0);
    parts[P_BODY].px = 0; parts[P_BODY].py = 6; parts[P_BODY].pz = 0;
    const float lp[4][3] = { {-2,18,4}, {2,18,4}, {-2,18,-4}, {2,18,-4} };
    for (int i = 0; i < 4; i++) {
        mobBuildBox(parts[P_LEG0+i].base, -2,0,-2, 2,6,2, 0,16, 4,6,4, false, 0);
        parts[P_LEG0+i].px = lp[i][0]; parts[P_LEG0+i].py = lp[i][1]; parts[P_LEG0+i].pz = lp[i][2];
    }
    parts[P_HEAD].head = true;
    g_built = true;
}

CreeperRenderer::CreeperRenderer() : haveTex(false) {
    shadowRadius = 0.5f; shadowStrength = 1.0f;
}

void CreeperRenderer::render(Entity* e, float x, float y, float z, float rot, float a) {
    if (!haveTex) { haveTex = textureLoad16("data/images/mob/creeper.png", &tex, GU_PSM_5551); if (!haveTex) return; }
    build();
    Creeper* cr = (Creeper*)e;
    Mob* mob = (Mob*)e;

    float dBody = mob->yBodyRot - mob->yBodyRotO;
    while (dBody > 180.0f) dBody -= 360.0f; while (dBody < -180.0f) dBody += 360.0f;
    float ibody = mob->yBodyRotO + dBody * a;
    float dHead = rot - ibody; while (dHead > 180.0f) dHead -= 360.0f; while (dHead < -180.0f) dHead += 360.0f;
    float ipitch = mob->xRotO + (mob->xRot - mob->xRotO) * a;
    float ws = mob->walkAnimSpeedO + (mob->walkAnimSpeed - mob->walkAnimSpeedO) * a; if (ws > 1.0f) ws = 1.0f;
    float wp = mob->walkAnimPos - mob->walkAnimSpeed * (1.0f - a);
    float t = wp * 0.6662f;
    float sA = cosf(t) * 1.4f * ws, sB = cosf(t + PIF) * 1.4f * ws;

    parts[P_HEAD].xRot = -ipitch * DEG2RAD; parts[P_HEAD].yRot = -dHead * DEG2RAD; parts[P_HEAD].zRot = 0;
    parts[P_BODY].xRot = parts[P_BODY].yRot = parts[P_BODY].zRot = 0;
    parts[P_LEG0].xRot = sA; parts[P_LEG1].xRot = sB; parts[P_LEG2].xRot = sB; parts[P_LEG3].xRot = sA;
    for (int i = P_LEG0; i <= P_LEG3; i++) { parts[i].yRot = parts[i].zRot = 0; }

    float gs = cr->getSwelling(a);
    float wobble = 1.0f + sinf(gs * 100.0f) * gs * 0.01f;
    float g = gs; if (g < 0.0f) g = 0.0f; if (g > 1.0f) g = 1.0f;
    g = g * g; g = g * g;
    float scaleXZ = (1.0f + g * 0.4f) * wobble;
    float scaleY  = (1.0f + g * 0.1f) / wobble;
    float flash = 0.0f;
    if (((int)(gs * 10.0f)) & 1) {
        flash = gs * 0.2f; if (flash < 0.0f) flash = 0.0f; if (flash > 1.0f) flash = 1.0f;
    }

    mobRenderParts(mob, parts, P_COUNT, &tex, x, y, z, ibody, a, 0xFFFFFFFFu,
                   8.0f, 4.0f, scaleXZ, flash, -1, scaleY);
}
