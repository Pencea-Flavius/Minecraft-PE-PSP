
#include "client/renderer/entity/sheep_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/entity/animal/sheep.h"
#include "gpu/texture.h"
#include <math.h>
#include <pspgu.h>

static const float DEG2RAD = 3.14159265f / 180.0f;

static const unsigned int WOOL_TINT[16] = {
    0xFFFFFFFFu, 0xFF33B3F2u, 0xFFD980E6u, 0xFFF2B399u, 0xFF33E6E6u, 0xFF1ACC80u,
    0xFFCCB3F2u, 0xFF4D4D4Du, 0xFF999999u, 0xFFB3994Du, 0xFFE666B3u, 0xFFCC6633u,
    0xFF4D6680u, 0xFF338066u, 0xFF4D4DCCu, 0xFF1A1A1Au,
};

enum { P_HEAD, P_BODY, P_LEG0, P_LEG1, P_LEG2, P_LEG3, P_COUNT };
static MobPart base[P_COUNT], fur[P_COUNT];
static bool    g_built = false;
static Texture g_base, g_fur;
static bool    g_haveBase = false, g_haveFur = false;

static void setPiv(MobPart& p, float x, float y, float z) { p.px = x; p.py = y; p.pz = z; }

static void build() {
    if (g_built) return;

    mobBuildBox(base[P_HEAD].base, -3,-4,-6, 3,2,2, 0,0, 6,6,8, false, 0);  setPiv(base[P_HEAD], 0,6,-8);
    mobBuildBox(base[P_BODY].base, -4,-10,-7, 4,6,-1, 28,8, 8,16,6, false, 0); setPiv(base[P_BODY], 0,5,2);
    const float lp[4][3] = { {-3,12,7}, {3,12,7}, {-3,12,-5}, {3,12,-5} };
    for (int i = 0; i < 4; i++) { mobBuildBox(base[P_LEG0+i].base, -2,0,-2, 2,12,2, 0,16, 4,12,4, false, 0);
        setPiv(base[P_LEG0+i], lp[i][0], lp[i][1], lp[i][2]); }

    mobBuildBox(fur[P_HEAD].base, -3,-4,-4, 3,2,2, 0,0, 6,6,6, false, 0.6f);  setPiv(fur[P_HEAD], 0,6,-8);
    mobBuildBox(fur[P_BODY].base, -4,-10,-7, 4,6,-1, 28,8, 8,16,6, false, 1.75f); setPiv(fur[P_BODY], 0,5,2);
    for (int i = 0; i < 4; i++) { mobBuildBox(fur[P_LEG0+i].base, -2,0,-2, 2,6,2, 0,16, 4,6,4, false, 0.5f);
        setPiv(fur[P_LEG0+i], lp[i][0], lp[i][1], lp[i][2]); }
    base[P_HEAD].head = fur[P_HEAD].head = true;
    g_built = true;
}

SheepRenderer::SheepRenderer() { shadowRadius = 0.7f; shadowStrength = 1.0f; }

static void setAnim(MobPart* p, float hx, float hy, float pend) {
    p[P_HEAD].xRot = hx; p[P_HEAD].yRot = hy; p[P_HEAD].zRot = 0;
    p[P_BODY].xRot = 90.0f * DEG2RAD; p[P_BODY].yRot = p[P_BODY].zRot = 0;
    p[P_LEG0].xRot = pend; p[P_LEG1].xRot = -pend; p[P_LEG2].xRot = -pend; p[P_LEG3].xRot = pend;
    for (int i = P_LEG0; i <= P_LEG3; i++) { p[i].yRot = p[i].zRot = 0; }
}

void SheepRenderer::render(Entity* e, float x, float y, float z, float rot, float a) {
    if (!g_haveBase) { g_haveBase = textureLoad16("data/images/mob/sheep.png", &g_base, GU_PSM_5551); if (!g_haveBase) return; }
    if (!g_haveFur)  { g_haveFur  = textureLoad16("data/images/mob/sheep_fur.png", &g_fur, GU_PSM_5551); }
    build();
    Sheep* sheep = (Sheep*)e;
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

    setAnim(base, hx, hy, pend);
    setAnim(fur,  hx, hy, pend);

    if (sheep->getEatAnimationTick() > 0) {
        float py  = 6.0f + sheep->getHeadEatPositionScale(a) * 9.0f;
        float exr = sheep->getHeadEatAngleScale(a);
        base[P_HEAD].py = py; base[P_HEAD].xRot = exr;
        fur[P_HEAD].py  = py; fur[P_HEAD].xRot  = exr;
    } else {
        base[P_HEAD].py = fur[P_HEAD].py = 6.0f;
    }

    mobRenderParts(mob, base, P_COUNT, &g_base, x, y, z, ibody, a, 0xFFFFFFFFu, 8.0f, 4.0f);

    if (!sheep->isSheared() && g_haveFur) {
        mobRenderParts(mob, fur, P_COUNT, &g_fur, x, y, z, ibody, a, WOOL_TINT[sheep->getColor() & 0x0f], 8.0f, 4.0f);
    }
}
