
#include "client/renderer/tileentity/tile_entity_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/entity/chest_tile_entity.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "platform/path.h"
#include "client/player/player_state.h"
#include <math.h>
#include <pspgu.h>
#include <pspgum.h>

static Texture s_single, s_double;
static bool    s_loaded = false, s_singleOk = false, s_doubleOk = false;

static void ensureAssets() {
    if (s_loaded) return;
    s_loaded = true;
    s_singleOk = textureLoad16(assetPath("data/images/item/chest/normal.png"), &s_single, GU_PSM_5551)
              || textureLoad16("data/images/item/chest/normal.png", &s_single, GU_PSM_5551);
    s_doubleOk = textureLoad16(assetPath("data/images/item/chest/double_normal.png"), &s_double, GU_PSM_5551)
              || textureLoad16("data/images/item/chest/double_normal.png", &s_double, GU_PSM_5551);
}

const Texture* chestModelTexture() {
    ensureAssets();
    return s_singleOk ? &s_single : 0;
}

static MobVertex s_lid[36], s_lock[36], s_body[36];

static const float S = 1.0f / 16.0f;

static void drawPart(const MobVertex* mesh, float px, float py, float pz, float xRot,
                     unsigned int col) {
    sceGumPushMatrix();
    ScePspFVector3 to = { px, py, pz };
    sceGumTranslate(&to);
    if (xRot != 0.0f) sceGumRotateX(xRot);
    void* v = guFrameCopy((void*)mesh, 36 * sizeof(MobVertex));

    if (v) mobDrawPartLit((const MobVertex*)v, col);
    sceGumPopMatrix();
}

int chestBuildHeldMesh(ChunkVertex* out) {
    MobVertex box[108];

    mobBuildBox(box +  0,  1*S, 2*S,  1*S, 15*S,  7*S, 15*S,  0,  0, 14,  5, 14, false, 0.0f, 64.0f, 64.0f);
    mobBuildBox(box + 36,  7*S, 5*S,  0*S,  9*S,  9*S,  1*S,  0,  0,  2,  4,  1, false, 0.0f, 64.0f, 64.0f);
    mobBuildBox(box + 72,  1*S, 6*S,  1*S, 15*S, 16*S, 15*S,  0, 19, 14, 10, 14, false, 0.0f, 64.0f, 64.0f);

    mobBoxToColoured((SkinVertex*)out, box, 108, 0xFFFFFFFFu);
    SkinVertex* v = (SkinVertex*)out;
    for (int i = 0; i < 108; i++) {
        v[i].y = 1.0f - v[i].y;
        v[i].z = 1.0f - v[i].z;

        v[i].y += 150.0f;
    }
    return 108;
}

void renderChestTile(ChestTileEntity* chest, float a) {
    ensureAssets();
    if (!s_singleOk) return;

    const bool dbl = (chest->pair != 0);
    if (dbl && !chest->isMaster()) return;
    if (dbl && !s_doubleOk) return;

    const unsigned int col = tileEntityLightColor(chest->level, chest->x, chest->y, chest->z);

    const float W  = dbl ? 30.0f : 14.0f;
    const int   Wi = dbl ? 30    : 14;
    const float xo = dbl ? -7.0f : 1.0f;
    const float tw = dbl ? 128.0f : 64.0f, th = 64.0f;

    mobBuildBox(s_lid,   0*S, -5*S, -14*S,  W*S,  0*S,   0*S, 0,  0, Wi,  5, 14, false, 0.0f, tw, th);
    mobBuildBox(s_lock, -1*S, -2*S, -15*S,  1*S,  2*S, -14*S, 0,  0,  2,  4,  1, false, 0.0f, tw, th);
    mobBuildBox(s_body,  0*S,  0*S,   0*S,  W*S, 10*S,  14*S, 0, 19, Wi, 10, 14, false, 0.0f, tw, th);

    float open = chest->oOpenness + (chest->openness - chest->oOpenness) * a;
    float t = 1.0f - open;
    float xRot = -(1.0f - t * t * t) * (3.14159265f / 2.0f);

    int data = faceFromMcpe(chest->getData());
    float rot = (data == F_FORWARD) ? 0.0f
              : (data == F_LEFT)    ? 90.0f
              : (data == F_BACK)    ? 180.0f
              : 270.0f;

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();

    ScePspFVector3 tr = { chest->x - g_relBaseX, chest->y + 1.0f - g_relBaseY, chest->z + 1.0f - g_relBaseZ };
    sceGumTranslate(&tr);
    ScePspFVector3 flip = { 1.0f, -1.0f, -1.0f };
    sceGumScale(&flip);
    ScePspFVector3 c1 = { 0.5f, 0.5f, 0.5f };
    sceGumTranslate(&c1);
    sceGumRotateY(rot * 3.14159265f / 180.0f);
    ScePspFVector3 c2 = { -0.5f, -0.5f, -0.5f };
    sceGumTranslate(&c2);

    if (dbl) {

        float sign = (data == F_BACK || data == F_RIGHT) ? -1.0f : 1.0f;
        ScePspFVector3 off = { chest->getModelOffsetX() * sign, 0.0f, 0.0f };
        sceGumTranslate(&off);
    }

    sceGuDisable(GU_CULL_FACE);
    textureBind(dbl ? &s_double : &s_single);

    drawPart(s_body, xo*S, 6*S,  1*S, 0.0f, col);
    drawPart(s_lid,  xo*S, 7*S, 15*S, xRot, col);
    drawPart(s_lock,  8*S, 7*S, 15*S, xRot, col);

    sceGuColor(0xFFFFFFFFu);

    sceGuEnable(GU_CULL_FACE);
    sceGumPopMatrix();
}
