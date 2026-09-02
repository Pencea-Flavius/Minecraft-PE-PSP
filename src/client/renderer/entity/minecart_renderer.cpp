
#include "client/renderer/entity/minecart_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/entity/minecart.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "platform/path.h"
#include "platform/dcache.h"
#include "util/mth.h"
#include <pspgu.h>
#include <pspgum.h>
#include <cmath>

extern World g_world;

static Texture s_tex;
static bool s_have = false, s_tried = false;

static const float DEG2RAD = 3.14159265f / 180.0f;
static const float PIF = 3.14159265f;

#define MC_PARTS 5
static MobVertex s_part[MC_PARTS][36];
static bool s_built = false;

struct PartDef { float px, py, pz; float xRot, yRot; };
static PartDef s_def[MC_PARTS];

static void buildModel() {
    if (s_built) return;

    mobBuildBox(s_part[0], -10.0f, -8.0f, -1.0f, 10.0f, 8.0f, 1.0f, 0, 10, 20, 16, 2, false, 0.0f);
    s_def[0] = { 0.0f, 4.0f, 0.0f, PIF * 0.5f, 0.0f };

    for (int i = 1; i <= 4; i++)
        mobBuildBox(s_part[i], -8.0f, -9.0f, -1.0f, 8.0f, -1.0f, 1.0f, 0, 0, 16, 8, 2, false, 0.0f);
    s_def[1] = { -9.0f, 4.0f, 0.0f,  0.0f, PIF * 0.5f * 3.0f };
    s_def[2] = {  9.0f, 4.0f, 0.0f,  0.0f, PIF * 0.5f };
    s_def[3] = {  0.0f, 4.0f, -7.0f, 0.0f, PIF };
    s_def[4] = {  0.0f, 4.0f,  7.0f, 0.0f, 0.0f };
    dcacheFlush(s_part, sizeof(s_part));
    s_built = true;
}

void MinecartRenderer::render(Entity* entity, float x, float y, float z, float rot, float a) {
    Minecart* cart = (Minecart*)entity;
    if (!s_tried) {
        s_have = textureLoad16(assetPath("data/images/item/minecart.png"), &s_tex, GU_PSM_5551)
              || textureLoad16("data/images/item/minecart.png", &s_tex, GU_PSM_5551);
        s_tried = true;
    }
    buildModel();

    float yaw = rot;
    float pitch = cart->xRot;

    float here[3];
    if (cart->railPos(x, y, z, here)) {
        float ahead[3], behind[3];
        if (!cart->railPosOffs(x, y, z,  0.3f, ahead))  { ahead[0] = here[0];  ahead[1] = here[1];  ahead[2] = here[2]; }
        if (!cart->railPosOffs(x, y, z, -0.3f, behind)) { behind[0] = here[0]; behind[1] = here[1]; behind[2] = here[2]; }

        x += here[0] - x;
        y += (ahead[1] + behind[1]) * 0.5f - y;
        z += here[2] - z;

        float dx = behind[0] - ahead[0];
        float dy = behind[1] - ahead[1];
        float dz = behind[2] - ahead[2];
        float len = sqrtf(dx * dx + dy * dy + dz * dz);
        if (len >= 0.0001f) {
            dx /= len; dy /= len; dz /= len;
            yaw   = atan2f(dz, dx) * 180.0f / PIF;

            pitch = atanf(dy) * 73.0f;
        }
    }

    int br = lightRawAt(&g_world, Mth::floor(x), Mth::floor(y), Mth::floor(z));
    unsigned int col = brightColorFloored(br, ENTITY_LIGHT_FLOOR);

    if (s_have) textureBindNoMip(&s_tex);
    else        sceGuDisable(GU_TEXTURE_2D);

    sceGuDisable(GU_BLEND);

    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);

    sceGuDisable(GU_CULL_FACE);
    sceGuColor(col);

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    ScePspFVector3 tr = { x - g_relBaseX, y - g_relBaseY, z - g_relBaseZ };
    sceGumTranslate(&tr);
    sceGumRotateY((180.0f - yaw) * DEG2RAD);
    sceGumRotateZ(-pitch * DEG2RAD);

    float ht = (float)cart->hurtTime - a;
    float dmg = cart->damage - a;
    if (dmg < 0.0f) dmg = 0.0f;
    if (ht > 0.0f)
        sceGumRotateX(sinf(ht) * ht * dmg / 10.0f * (float)cart->hurtDir * DEG2RAD);

    ScePspFVector3 flip = { -1.0f / 16.0f, -1.0f / 16.0f, 1.0f / 16.0f };
    sceGumScale(&flip);

    for (int i = 0; i < MC_PARTS; i++) {
        sceGumPushMatrix();
        ScePspFVector3 p = { s_def[i].px, s_def[i].py, s_def[i].pz };
        sceGumTranslate(&p);
        if (s_def[i].yRot != 0.0f) sceGumRotateY(s_def[i].yRot);
        if (s_def[i].xRot != 0.0f) sceGumRotateX(s_def[i].xRot);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                        36, 0, s_part[i]);
        sceGumPopMatrix();
    }

    sceGumPopMatrix();

    sceGuColor(0xFFFFFFFFu);
    sceGuEnable(GU_CULL_FACE);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    if (!s_have) sceGuEnable(GU_TEXTURE_2D);
}
