
#include "client/renderer/entity/chicken_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/entity/mob.h"
#include "world/entity/animal/chicken.h"
#include "gpu/texture.h"
#include <math.h>
#include <pspgu.h>

static const float DEG2RAD = 3.14159265f / 180.0f;

enum { P_HEAD, P_BEAK, P_WATTLE, P_BODY, P_LEG0, P_LEG1, P_WING0, P_WING1, P_COUNT };
static MobPart parts[P_COUNT];
static bool    g_built = false;
static Texture g_tex;
static bool    g_have  = false;

static void build() {
    if (g_built) return;
    const float yo = 16.0f;
    mobBuildBox(parts[P_HEAD].base,   -2,-6,-2, 2,0,1,  0,0,  4,6,3, false, 0);
    parts[P_HEAD].px = 0; parts[P_HEAD].py = -1 + yo; parts[P_HEAD].pz = -4;
    mobBuildBox(parts[P_BEAK].base,   -2,-4,-4, 2,-2,-2, 14,0, 4,2,2, false, 0);
    parts[P_BEAK].px = 0; parts[P_BEAK].py = -1 + yo; parts[P_BEAK].pz = -4;
    mobBuildBox(parts[P_WATTLE].base, -1,-2,-3, 1,0,-1, 14,4, 2,2,2, false, 0);
    parts[P_WATTLE].px = 0; parts[P_WATTLE].py = -1 + yo; parts[P_WATTLE].pz = -4;
    mobBuildBox(parts[P_BODY].base,   -3,-4,-3, 3,4,3,  0,9,  6,8,6, false, 0);
    parts[P_BODY].px = 0; parts[P_BODY].py = yo; parts[P_BODY].pz = 0;
    mobBuildBox(parts[P_LEG0].base,   -1,0,-3, 2,5,0, 26,0, 3,5,3, false, 0);
    parts[P_LEG0].px = -2; parts[P_LEG0].py = 3 + yo; parts[P_LEG0].pz = 1;
    mobBuildBox(parts[P_LEG1].base,   -1,0,-3, 2,5,0, 26,0, 3,5,3, false, 0);
    parts[P_LEG1].px = 1; parts[P_LEG1].py = 3 + yo; parts[P_LEG1].pz = 1;
    mobBuildBox(parts[P_WING0].base,   0,0,-3, 1,4,3, 24,13, 1,4,6, false, 0);
    parts[P_WING0].px = -4; parts[P_WING0].py = -3 + yo; parts[P_WING0].pz = 0;
    mobBuildBox(parts[P_WING1].base,  -1,0,-3, 0,4,3, 24,13, 1,4,6, false, 0);
    parts[P_WING1].px = 4; parts[P_WING1].py = -3 + yo; parts[P_WING1].pz = 0;
    parts[P_HEAD].head = parts[P_BEAK].head = parts[P_WATTLE].head = true;
    g_built = true;
}

ChickenRenderer::ChickenRenderer() { shadowRadius = 0.3f; shadowStrength = 1.0f; }

void ChickenRenderer::render(Entity* e, float x, float y, float z, float rot, float a) {
    if (!g_have) { g_have = textureLoad16("data/images/mob/chicken.png", &g_tex, GU_PSM_5551); if (!g_have) return; }
    build();
    Mob*     mob = (Mob*)e;
    Chicken* ch  = (Chicken*)e;

    float dBody = mob->yBodyRot - mob->yBodyRotO;
    while (dBody > 180.0f) dBody -= 360.0f; while (dBody < -180.0f) dBody += 360.0f;
    float ibody = mob->yBodyRotO + dBody * a;
    float dHead = rot - ibody; while (dHead > 180.0f) dHead -= 360.0f; while (dHead < -180.0f) dHead += 360.0f;
    float ipitch = mob->xRotO + (mob->xRot - mob->xRotO) * a;
    float ws = mob->walkAnimSpeedO + (mob->walkAnimSpeed - mob->walkAnimSpeedO) * a; if (ws > 1.0f) ws = 1.0f;
    float wp = mob->walkAnimPos - mob->walkAnimSpeed * (1.0f - a);
    if (mob->isBaby()) wp *= 3.0f;

    float hx = -(ipitch * DEG2RAD);
    float hy  = -(dHead  * DEG2RAD);
    parts[P_HEAD].xRot   = hx; parts[P_HEAD].yRot   = hy; parts[P_HEAD].zRot   = 0;
    parts[P_BEAK].xRot   = hx; parts[P_BEAK].yRot   = hy; parts[P_BEAK].zRot   = 0;
    parts[P_WATTLE].xRot = hx; parts[P_WATTLE].yRot = hy; parts[P_WATTLE].zRot = 0;

    parts[P_BODY].xRot = 90.0f * DEG2RAD; parts[P_BODY].yRot = 0; parts[P_BODY].zRot = 0;

    float pend = cosf(wp * 0.6662f) * 1.4f * ws;
    parts[P_LEG0].xRot =  pend; parts[P_LEG0].yRot = 0; parts[P_LEG0].zRot = 0;
    parts[P_LEG1].xRot = -pend; parts[P_LEG1].yRot = 0; parts[P_LEG1].zRot = 0;

    float flap      = ch->oFlap      + (ch->flap      - ch->oFlap)      * a;
    float flapSpeed = ch->oFlapSpeed + (ch->flapSpeed - ch->oFlapSpeed) * a;
    float bob = (sinf(flap) + 1.0f) * flapSpeed;
    parts[P_WING0].zRot =  bob; parts[P_WING0].xRot = 0; parts[P_WING0].yRot = 0;
    parts[P_WING1].zRot = -bob; parts[P_WING1].xRot = 0; parts[P_WING1].yRot = 0;

    float yFixed = y + 2.0f / 16.0f;

    mobRenderParts(mob, parts, P_COUNT, &g_tex, x, yFixed, z, ibody, a, 0xFFFFFFFFu, 5.0f, 2.0f);
}
