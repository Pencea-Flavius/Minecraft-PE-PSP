
#include "client/renderer/entity/spider_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/entity/mob.h"
#include <math.h>
#include <pspgu.h>

static const float DEG2RAD = 3.14159265f / 180.0f;
static const float PIF     = 3.14159265f;

enum { P_HEAD, P_BODY0, P_BODY1, P_LEG0, P_LEG1, P_LEG2, P_LEG3, P_LEG4, P_LEG5, P_LEG6, P_LEG7, P_COUNT };
static MobPart parts[P_COUNT];
static bool    g_built = false;

static void build() {
    if (g_built) return;
    const float yo = 15.0f;
    mobBuildBox(parts[P_HEAD].base,  -4,-4,-8, 4,4,0,  32,4, 8,8,8, false, 0);
    parts[P_HEAD].px = 0; parts[P_HEAD].py = yo; parts[P_HEAD].pz = -3;
    mobBuildBox(parts[P_BODY0].base, -3,-3,-3, 3,3,3,  0,0, 6,6,6, false, 0);
    parts[P_BODY0].px = 0; parts[P_BODY0].py = yo; parts[P_BODY0].pz = 0;
    mobBuildBox(parts[P_BODY1].base, -5,-4,-6, 5,4,6,  0,12, 10,8,12, false, 0);
    parts[P_BODY1].px = 0; parts[P_BODY1].py = yo; parts[P_BODY1].pz = 9;

    const float lz[8] = { 2,2, 1,1, 0,0, -1,-1 };
    for (int i = 0; i < 8; i++) {
        bool right = (i & 1);
        if (right) mobBuildBox(parts[P_LEG0+i].base, -1,-1,-1, 15,1,1, 18,0, 16,2,2, false, 0);
        else       mobBuildBox(parts[P_LEG0+i].base, -15,-1,-1, 1,1,1, 18,0, 16,2,2, false, 0);
        parts[P_LEG0+i].px = right ? 4.0f : -4.0f;
        parts[P_LEG0+i].py = yo;
        parts[P_LEG0+i].pz = lz[i];
    }
    parts[P_HEAD].head = true;
    g_built = true;
}

SpiderRenderer::SpiderRenderer() : haveTex(false) {
    shadowRadius = 0.7f; shadowStrength = 1.0f;
}

void SpiderRenderer::render(Entity* e, float x, float y, float z, float rot, float a) {
    if (!haveTex) { haveTex = textureLoad16("data/images/mob/spider.png", &tex, GU_PSM_5551); if (!haveTex) return; }
    build();
    Mob* mob = (Mob*)e;

    float dBody = mob->yBodyRot - mob->yBodyRotO;
    while (dBody > 180.0f) dBody -= 360.0f; while (dBody < -180.0f) dBody += 360.0f;
    float ibody = mob->yBodyRotO + dBody * a;
    float dHead = rot - ibody; while (dHead > 180.0f) dHead -= 360.0f; while (dHead < -180.0f) dHead += 360.0f;
    float ipitch = mob->xRotO + (mob->xRot - mob->xRotO) * a;
    float r = mob->walkAnimSpeedO + (mob->walkAnimSpeed - mob->walkAnimSpeedO) * a; if (r > 1.0f) r = 1.0f;
    float time = mob->walkAnimPos - mob->walkAnimSpeed * (1.0f - a);

    parts[P_HEAD].xRot = -ipitch * DEG2RAD; parts[P_HEAD].yRot = -dHead * DEG2RAD; parts[P_HEAD].zRot = 0;
    parts[P_BODY0].xRot = parts[P_BODY0].yRot = parts[P_BODY0].zRot = 0;
    parts[P_BODY1].xRot = parts[P_BODY1].yRot = parts[P_BODY1].zRot = 0;

    const float sr = PIF / 4.0f, ur = PIF / 8.0f, ur2 = sr;

    parts[P_LEG0].zRot = -sr;        parts[P_LEG1].zRot = sr;
    parts[P_LEG2].zRot = -sr*0.74f;  parts[P_LEG3].zRot = sr*0.74f;
    parts[P_LEG4].zRot = -sr*0.74f;  parts[P_LEG5].zRot = sr*0.74f;
    parts[P_LEG6].zRot = -sr;        parts[P_LEG7].zRot = sr;
    parts[P_LEG0].yRot =  ur2; parts[P_LEG1].yRot = -ur2;
    parts[P_LEG2].yRot =  ur;  parts[P_LEG3].yRot = -ur;
    parts[P_LEG4].yRot = -ur;  parts[P_LEG5].yRot =  ur;
    parts[P_LEG6].yRot = -ur2; parts[P_LEG7].yRot =  ur2;

    float c0 = -(cosf(time*0.6662f*2 + PIF*2*0/4.0f) * 0.4f) * r;
    float c1 = -(cosf(time*0.6662f*2 + PIF*2*2/4.0f) * 0.4f) * r;
    float c2 = -(cosf(time*0.6662f*2 + PIF*2*1/4.0f) * 0.4f) * r;
    float c3 = -(cosf(time*0.6662f*2 + PIF*2*3/4.0f) * 0.4f) * r;
    float s0 = fabsf(sinf(time*0.6662f + PIF*2*0/4.0f) * 0.4f) * r;
    float s1 = fabsf(sinf(time*0.6662f + PIF*2*2/4.0f) * 0.4f) * r;
    float s2 = fabsf(sinf(time*0.6662f + PIF*2*1/4.0f) * 0.4f) * r;
    float s3 = fabsf(sinf(time*0.6662f + PIF*2*3/4.0f) * 0.4f) * r;

    parts[P_LEG0].yRot += c0; parts[P_LEG1].yRot -= c0;
    parts[P_LEG2].yRot += c1; parts[P_LEG3].yRot -= c1;
    parts[P_LEG4].yRot += c2; parts[P_LEG5].yRot -= c2;
    parts[P_LEG6].yRot += c3; parts[P_LEG7].yRot -= c3;
    parts[P_LEG0].zRot += s0; parts[P_LEG1].zRot -= s0;
    parts[P_LEG2].zRot += s1; parts[P_LEG3].zRot -= s1;
    parts[P_LEG4].zRot += s2; parts[P_LEG5].zRot -= s2;
    parts[P_LEG6].zRot += s3; parts[P_LEG7].zRot -= s3;
    for (int i = P_LEG0; i <= P_LEG7; i++) parts[i].xRot = 0;

    mobRenderParts(mob, parts, P_COUNT, &tex, x, y, z, ibody, a, 0xFFFFFFFFu, 8.0f, 4.0f);
}
