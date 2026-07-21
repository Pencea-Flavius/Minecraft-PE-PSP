
#include "client/renderer/entity/humanoid_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/entity/mob.h"
#include <math.h>
#include <pspgu.h>

static const float DEG2RAD = 3.14159265f / 180.0f;
static const float PIF     = 3.14159265f;

enum { P_HEAD, P_BODY, P_ARM0, P_ARM1, P_LEG0, P_LEG1, P_COUNT };
static MobPart partsN[P_COUNT];
static MobPart partsT[P_COUNT];
static bool    g_built = false;

static void buildInto(MobPart* p, bool thin) {

    mobBuildBox(p[P_HEAD].base, -4,-8,-4,  4, 0, 4,  0,  0, 8,8,8, false, 0);
    p[P_HEAD].px = 0;  p[P_HEAD].py = 0;  p[P_HEAD].pz = 0;
    mobBuildBox(p[P_BODY].base, -4, 0,-2,  4,12, 2, 16, 16, 8,12,4, false, 0);
    p[P_BODY].px = 0;  p[P_BODY].py = 0;  p[P_BODY].pz = 0;
    if (!thin) {
        mobBuildBox(p[P_ARM0].base, -3,-2,-2,  1,10, 2, 40, 16, 4,12,4, false, 0);
        mobBuildBox(p[P_ARM1].base, -1,-2,-2,  3,10, 2, 40, 16, 4,12,4, true,  0);
        mobBuildBox(p[P_LEG0].base, -2, 0,-2,  2,12, 2,  0, 16, 4,12,4, false, 0);
        mobBuildBox(p[P_LEG1].base, -2, 0,-2,  2,12, 2,  0, 16, 4,12,4, true,  0);
    } else {

        mobBuildBox(p[P_ARM0].base, -1,-2,-1,  1,10, 1, 40, 16, 2,12,2, false, 0);
        mobBuildBox(p[P_ARM1].base, -1,-2,-1,  1,10, 1, 40, 16, 2,12,2, true,  0);
        mobBuildBox(p[P_LEG0].base, -1, 0,-1,  1,12, 1,  0, 16, 2,12,2, false, 0);
        mobBuildBox(p[P_LEG1].base, -1, 0,-1,  1,12, 1,  0, 16, 2,12,2, true,  0);
    }
    p[P_ARM0].px = -5; p[P_ARM0].py = 2;  p[P_ARM0].pz = 0;
    p[P_ARM1].px = 5;  p[P_ARM1].py = 2;  p[P_ARM1].pz = 0;
    p[P_LEG0].px = -2; p[P_LEG0].py = 12; p[P_LEG0].pz = 0;
    p[P_LEG1].px = 2;  p[P_LEG1].py = 12; p[P_LEG1].pz = 0;
    p[P_HEAD].head = true;
}

static void build() {
    if (g_built) return;
    buildInto(partsN, false);
    buildInto(partsT, true);
    g_built = true;
}

HumanoidRenderer::HumanoidRenderer(const char* texPath, bool thin, bool bow, short holdItem)
:   texPath(texPath), haveTex(false), thin(thin), bow(bow), holdItem(holdItem) {
    shadowRadius = 0.5f; shadowStrength = 1.0f;
}

void HumanoidRenderer::render(Entity* e, float x, float y, float z, float rot, float a) {
    if (!haveTex) { haveTex = textureLoad16(texPath, &tex, GU_PSM_5551); if (!haveTex) return; }
    build();
    MobPart* parts = thin ? partsT : partsN;
    Mob* mob = (Mob*)e;

    float dBody = mob->yBodyRot - mob->yBodyRotO;
    while (dBody > 180.0f) dBody -= 360.0f; while (dBody < -180.0f) dBody += 360.0f;
    float ibody = mob->yBodyRotO + dBody * a;
    float dHead = rot - ibody; while (dHead > 180.0f) dHead -= 360.0f; while (dHead < -180.0f) dHead += 360.0f;
    float ipitch = mob->xRotO + (mob->xRot - mob->xRotO) * a;
    float ws = mob->walkAnimSpeedO + (mob->walkAnimSpeed - mob->walkAnimSpeedO) * a; if (ws > 1.0f) ws = 1.0f;
    float wp = mob->walkAnimPos - mob->walkAnimSpeed * (1.0f - a);
    if (mob->isBaby()) wp *= 3.0f;
    float t = wp * 0.6662f;
    float tcos0 = cosf(t) * ws, tcos1 = cosf(t + PIF) * ws;

    parts[P_HEAD].xRot = -ipitch * DEG2RAD; parts[P_HEAD].yRot = -dHead * DEG2RAD; parts[P_HEAD].zRot = 0;
    parts[P_BODY].xRot = parts[P_BODY].yRot = parts[P_BODY].zRot = 0;
    parts[P_LEG0].xRot = tcos0 * 1.4f; parts[P_LEG0].yRot = parts[P_LEG0].zRot = 0;
    parts[P_LEG1].xRot = tcos1 * 1.4f; parts[P_LEG1].yRot = parts[P_LEG1].zRot = 0;

    if (bow) {

        float hy = parts[P_HEAD].yRot, hx = parts[P_HEAD].xRot;
        parts[P_ARM0].zRot = 0;                parts[P_ARM1].zRot = 0;
        parts[P_ARM0].yRot = -0.1f + hy;       parts[P_ARM1].yRot = 0.1f + hy + 0.4f;
        parts[P_ARM0].xRot = -PIF / 2.0f + hx; parts[P_ARM1].xRot = -PIF / 2.0f + hx;
    } else if (holdItem) {

        parts[P_ARM0].xRot = tcos1; parts[P_ARM0].yRot = 0; parts[P_ARM0].zRot = 0;
        parts[P_ARM1].xRot = tcos0; parts[P_ARM1].yRot = 0; parts[P_ARM1].zRot = 0;
        parts[P_ARM0].xRot = parts[P_ARM0].xRot * 0.5f - PIF / 2.0f * 0.2f;
        float aa = mob->getAttackAnim(a);
        if (aa > 0.001f) {

            float bodyYaw = sinf(sqrtf(aa) * PIF * 2.0f) * 0.2f;
            parts[P_BODY].yRot = bodyYaw;
            float f6 = 1.0f - aa; f6 *= f6; f6 *= f6; f6 = 1.0f - f6;
            float f7 = sinf(f6 * PIF);
            float f8 = sinf(aa * PIF) * -(parts[P_HEAD].xRot - 0.7f) * 0.75f;
            parts[P_ARM0].xRot -= f7 * 1.2f + f8;
            parts[P_ARM0].yRot += bodyYaw * 2.0f;
            parts[P_ARM0].zRot  = sinf(aa * PIF) * -0.4f;
            parts[P_ARM1].yRot += bodyYaw;
        }
    } else {

        float aa = mob->getAttackAnim(a);
        float attack2 = sinf(aa * PIF);
        float attack  = sinf((1.0f - (1.0f - aa) * (1.0f - aa)) * PIF);
        float armX = -PIF / 2.0f - (attack2 * 1.2f - attack * 0.4f);
        float armYaw = 0.1f - attack2 * 0.6f;
        parts[P_ARM0].xRot = armX; parts[P_ARM0].yRot = -armYaw; parts[P_ARM0].zRot = 0;
        parts[P_ARM1].xRot = armX; parts[P_ARM1].yRot =  armYaw; parts[P_ARM1].zRot = 0;
    }

    float bob = (float)mob->tickCount + a;
    float zBob = cosf(bob * 0.09f) * 0.05f + 0.05f;
    float xBob = sinf(bob * 0.067f) * 0.05f;
    parts[P_ARM0].zRot += zBob; parts[P_ARM1].zRot -= zBob;
    parts[P_ARM0].xRot += xBob; parts[P_ARM1].xRot -= xBob;

    int heldPart = (bow || holdItem) ? P_ARM0 : -1;
    mobRenderParts(mob, parts, P_COUNT, &tex, x, y, z, ibody, a,
                   0xFFFFFFFFu, 8.0f, 4.0f, 1.0f, 0.0f, heldPart, -1.0f, holdItem);
}
