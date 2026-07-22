
#include "client/renderer/entity/player_model.h"
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "world/item/armor_item.h"

#include "gpu/texture.h"
#include "platform/time.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/entity/local_player.h"
#include "client/player/player_state.h"
#include "client/renderer/item_hand.h"
#include "client/renderer/item_model.h"
#include "client/renderer/entity/entity_renderer.h"
#include "world/inventory/inventory.h"
#include "world/item/item_instance.h"
#include "world/item/item.h"

extern Level   g_level;
extern World   g_world;
extern float   g_attackAnim, g_oAttackAnim;
extern Texture g_terrain;
extern bool    g_haveTerrain;

static const float DEG2RAD = 3.14159265f / 180.0f;
static const float PIF     = 3.14159265f;

struct SkinVertex { float u, v; unsigned int color; float x, y, z; };

struct Part { SkinVertex base[36]; SkinVertex mesh[36]; float px, py, pz; float xRot, yRot, zRot; };
enum { P_HEAD, P_BODY, P_ARM0, P_ARM1, P_LEG0, P_LEG1, P_COUNT };
static Part parts[P_COUNT];
static bool g_built = false;

static Texture g_localSkinTex;
static bool    g_haveLocalSkin = false;

static Texture g_charTex;
static bool    g_haveChar = false;

static void loadLocalPlayerSkinIfNeeded(void) {
    if (g_haveLocalSkin) return;
    if (!textureLoad16("data/images/mob/skin.png", &g_localSkinTex, GU_PSM_5551)) {
        if (!g_haveChar) g_haveChar = textureLoad16("data/images/mob/char.png", &g_charTex, GU_PSM_5551);
        g_localSkinTex = g_charTex;
        g_haveLocalSkin = g_haveChar;
    } else {
        g_haveLocalSkin = true;
    }
}

static void loadCharTextureIfNeeded(void) {
    if (g_haveChar) return;
    g_haveChar = textureLoad16("data/images/mob/char.png", &g_charTex, GU_PSM_5551);
}

static void buildBox(SkinVertex* out,
                     float x0, float y0, float z0, float x1, float y1, float z1,
                     int tx, int ty, int w, int h, int d, bool mirror) {
    const float W = 64.0f, H = 32.0f;
    const unsigned int col = 0xFFFFFFFFu;
    int n = 0;
    auto addPoly = [&](float ax, float ay, float az, float bx, float by, float bz,
                       float cx, float cy, float cz, float dx, float dy, float dz,
                       float u0, float v0, float u1, float v1) {
        if (mirror) { float t = u0; u0 = u1; u1 = t; }
        out[n++] = {u0, v0, col, ax, ay, az};
        out[n++] = {u1, v0, col, bx, by, bz};
        out[n++] = {u1, v1, col, cx, cy, cz};
        out[n++] = {u1, v1, col, cx, cy, cz};
        out[n++] = {u0, v1, col, dx, dy, dz};
        out[n++] = {u0, v0, col, ax, ay, az};
    };

    addPoly(x1,y0,z1, x0,y0,z1, x0,y0,z0, x1,y0,z0,
            (tx+d+w)/W,(ty)/H,     (tx+d)/W,(ty+d)/H);
    addPoly(x0,y0,z0, x0,y0,z1, x0,y1,z1, x0,y1,z0,
            (tx+d)/W,(ty+d)/H,     (tx)/W,(ty+d+h)/H);
    addPoly(x1,y0,z1, x1,y0,z0, x1,y1,z0, x1,y1,z1,
            (tx+2*d+w)/W,(ty+d)/H, (tx+d+w)/W,(ty+d+h)/H);
    addPoly(x1,y1,z0, x0,y1,z0, x0,y1,z1, x1,y1,z1,
            (tx+d+w)/W,(ty+d)/H,   (tx+d+2*w)/W,(ty)/H);
    addPoly(x1,y0,z0, x0,y0,z0, x0,y1,z0, x1,y1,z0,
            (tx+d+w)/W,(ty+d)/H,   (tx+d)/W,(ty+d+h)/H);
    addPoly(x0,y0,z1, x1,y0,z1, x1,y1,z1, x0,y1,z1,
            (tx+2*d+2*w)/W,(ty+d)/H, (tx+2*d+w)/W,(ty+d+h)/H);
}

static void buildParts(void) {
    if (g_built) return;

    buildBox(parts[P_HEAD].base, -4,-8,-4,  4, 0, 4,  0,  0, 8,8,8, false);
    parts[P_HEAD].px = 0;  parts[P_HEAD].py = 0;  parts[P_HEAD].pz = 0;

    buildBox(parts[P_BODY].base, -4, 0,-2,  4,12, 2, 16, 16, 8,12,4, false);
    parts[P_BODY].px = 0;  parts[P_BODY].py = 0;  parts[P_BODY].pz = 0;

    buildBox(parts[P_ARM0].base, -3,-2,-2,  1,10, 2, 40, 16, 4,12,4, false);
    parts[P_ARM0].px = -5; parts[P_ARM0].py = 2;  parts[P_ARM0].pz = 0;

    buildBox(parts[P_ARM1].base, -1,-2,-2,  3,10, 2, 40, 16, 4,12,4, true);
    parts[P_ARM1].px = 5;  parts[P_ARM1].py = 2;  parts[P_ARM1].pz = 0;

    buildBox(parts[P_LEG0].base, -2, 0,-2,  2,12, 2,  0, 16, 4,12,4, false);
    parts[P_LEG0].px = -2; parts[P_LEG0].py = 12; parts[P_LEG0].pz = 0;

    buildBox(parts[P_LEG1].base, -2, 0,-2,  2,12, 2,  0, 16, 4,12,4, true);
    parts[P_LEG1].px = 2;  parts[P_LEG1].py = 12; parts[P_LEG1].pz = 0;
    g_built = true;
}

static SkinVertex g_armor1[P_COUNT][36];
static SkinVertex g_armor05[P_COUNT][36];
static bool       g_armorBuilt = false;
static SkinVertex g_armorMesh[9][36];

static void buildArmorSet(SkinVertex set[][36], float inf) {
    buildBox(set[P_HEAD], -4-inf,-8-inf,-4-inf,  4+inf, 0+inf, 4+inf,  0,  0, 8,8,8, false);
    buildBox(set[P_BODY], -4-inf, 0-inf,-2-inf,  4+inf,12+inf, 2+inf, 16, 16, 8,12,4, false);
    buildBox(set[P_ARM0], -3-inf,-2-inf,-2-inf,  1+inf,10+inf, 2+inf, 40, 16, 4,12,4, false);
    buildBox(set[P_ARM1], -1-inf,-2-inf,-2-inf,  3+inf,10+inf, 2+inf, 40, 16, 4,12,4, true);
    buildBox(set[P_LEG0], -2-inf, 0-inf,-2-inf,  2+inf,12+inf, 2+inf,  0, 16, 4,12,4, false);
    buildBox(set[P_LEG1], -2-inf, 0-inf,-2-inf,  2+inf,12+inf, 2+inf,  0, 16, 4,12,4, true);
}
static void buildArmor() {
    if (g_armorBuilt) return;
    buildArmorSet(g_armor1, 1.0f);
    buildArmorSet(g_armor05, 0.5f);
    g_armorBuilt = true;
}

static Texture g_armorTex[5][2];
static bool    g_armorTried[5][2];
static bool    g_armorOK[5][2];
static Texture* armorTexture(int mat, int file) {
    if (mat < 0 || mat > 4 || file < 0 || file > 1) return 0;
    if (!g_armorTried[mat][file]) {
        static const char* nm[5] = { "cloth", "chain", "iron", "diamond", "gold" };
        char path[80];
        snprintf(path, sizeof path, "data/images/armor/%s_%d.png", nm[mat], file + 1);
        g_armorOK[mat][file] = textureLoad16(path, &g_armorTex[mat][file], GU_PSM_5551);
        g_armorTried[mat][file] = true;
    }
    return g_armorOK[mat][file] ? &g_armorTex[mat][file] : 0;
}

static void drawArmorLayers(unsigned int brCol) {
    LocalPlayer* p = g_level.player;
    if (!p) return;
    buildArmor();

    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);
    struct ArmorLayer { int slot; const int* parts; int n; int file; bool inner; };
    static const int legParts[3]   = { P_BODY, P_LEG0, P_LEG1 };
    static const int feetParts[2]  = { P_LEG0, P_LEG1 };
    static const int torsoParts[3] = { P_BODY, P_ARM0, P_ARM1 };
    static const int headParts[1]  = { P_HEAD };
    static const ArmorLayer layers[4] = {
        { ArmorItem::SLOT_LEGS,  legParts,   3, 1, true  },
        { ArmorItem::SLOT_FEET,  feetParts,  2, 0, false },
        { ArmorItem::SLOT_TORSO, torsoParts, 3, 0, false },
        { ArmorItem::SLOT_HEAD,  headParts,  1, 0, false },
    };
    int drawn = 0;
    for (int li = 0; li < 4; li++) {
        const ArmorLayer& ly = layers[li];
        ItemInstance& ai = p->armor[ly.slot];
        if (ai.isNull()) continue;
        Item* it = ai.getItem();
        if (!it || !it->isArmor()) continue;
        Texture* tex = armorTexture((ai.id - 298) / 4, ly.file);
        if (!tex) continue;
        textureBind(tex);
        SkinVertex (*set)[36] = ly.inner ? g_armor05 : g_armor1;
        for (int k = 0; k < ly.n && drawn < 9; k++) {
            int i = ly.parts[k];
            SkinVertex* m = g_armorMesh[drawn++];
            for (int v = 0; v < 36; v++) { m[v] = set[i][v]; m[v].color = brCol; }
            sceKernelDcacheWritebackInvalidateRange(m, sizeof(g_armorMesh[0]));
            sceGumPushMatrix();
            ScePspFVector3 piv = { parts[i].px, parts[i].py, parts[i].pz };
            sceGumTranslate(&piv);
            if (parts[i].zRot != 0.0f) sceGumRotateZ(parts[i].zRot);
            if (parts[i].yRot != 0.0f) sceGumRotateY(parts[i].yRot);
            if (parts[i].xRot != 0.0f) sceGumRotateX(parts[i].xRot);
            sceGumDrawArray(GU_TRIANGLES,
                GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                36, 0, m);
            sceGumPopMatrix();
        }
    }
}

void playerModelRender(float a) {
    LocalPlayer* p = g_level.player;
    if (!p) return;

    if (p->health <= 0 && p->deathTime >= 20) return;
    loadLocalPlayerSkinIfNeeded();
    if (!g_haveLocalSkin) return;
    buildParts();

    float ix = p->xo + (p->x - p->xo) * a;
    float iy = p->yo + (p->y - p->yo) * a;
    float iz = p->zo + (p->z - p->zo) * a;
    float iyaw   = p->yRotO + (p->yRot - p->yRotO) * a;
    float ipitch = p->xRotO + (p->xRot - p->xRotO) * a;
    float feet   = iy - PLAYER_EYE;

    float dBody = p->yBodyRot - p->yBodyRotO; while (dBody > 180.0f) dBody -= 360.0f; while (dBody < -180.0f) dBody += 360.0f;
    float ibody = p->yBodyRotO + dBody * a;
    float dHead = iyaw - ibody; while (dHead > 180.0f) dHead -= 360.0f; while (dHead < -180.0f) dHead += 360.0f;

    float ws = p->walkAnimSpeedO + (p->walkAnimSpeed - p->walkAnimSpeedO) * a;
    if (ws > 1.0f) ws = 1.0f;
    float wp = p->walkAnimPos - p->walkAnimSpeed * (1.0f - a);
    float t  = wp * 0.6662f;
    float tcos0 = cosf(t) * ws, tcos1 = cosf(t + PIF) * ws;

    parts[P_HEAD].xRot = -ipitch * DEG2RAD; parts[P_HEAD].yRot = -dHead * DEG2RAD;
    parts[P_BODY].xRot = parts[P_BODY].yRot = parts[P_BODY].zRot = 0.0f;
    parts[P_ARM0].xRot = tcos1; parts[P_ARM0].yRot = parts[P_ARM0].zRot = 0.0f;
    parts[P_ARM1].xRot = tcos0; parts[P_ARM1].yRot = parts[P_ARM1].zRot = 0.0f;
    parts[P_LEG0].xRot = tcos0 * 1.4f; parts[P_LEG1].xRot = tcos1 * 1.4f;

    ItemInstance* selHeld = g_level.player->inventory->getSelected();
    bool holding = selHeld && !selHeld->isNull();
    bool aiming = holding && selHeld->id == ITEM_BOW && p->bowPull > 0.0f;
    int  bowStage = aiming ? bowStageIcon(p->bowTimeHeld) : -1;

    if (holding) parts[P_ARM0].xRot = parts[P_ARM0].xRot * 0.5f - PIF / 2.0f * 0.2f;

    float ageT = nowSeconds() * 20.0f;

    float bcos = cosf(ageT * 0.09f) * 0.05f + 0.05f;
    float bsin = sinf(ageT * 0.067f) * 0.05f;
    parts[P_ARM0].zRot += bcos; parts[P_ARM1].zRot -= bcos;
    parts[P_ARM0].xRot += bsin; parts[P_ARM1].xRot -= bsin;

    float diff = g_attackAnim - g_oAttackAnim; if (diff < 0.0f) diff += 1.0f;
    float atk = g_oAttackAnim + diff * a; if (atk > 1.0f) atk -= 1.0f;
    if (atk > 0.001f) {
        float f = 1.0f - atk; f *= f; f *= f; f = 1.0f - f;
        float s1 = sinf(f * PIF);
        parts[P_BODY].yRot  = sinf(sqrtf(atk) * PIF * 2.0f) * 0.2f;
        parts[P_ARM0].yRot += parts[P_BODY].yRot;
        parts[P_ARM1].yRot += parts[P_BODY].yRot;
        parts[P_ARM1].xRot += parts[P_BODY].yRot;
        parts[P_ARM0].xRot -= s1 * 1.2f + sinf(atk * PIF) * (0.7f - parts[P_HEAD].xRot) * 0.75f;
        parts[P_ARM0].yRot += parts[P_BODY].yRot * 2.0f;
        parts[P_ARM0].zRot += sinf(atk * PIF) * -0.4f;
    }

    if (aiming) {
        float hx = parts[P_HEAD].xRot, hy = parts[P_HEAD].yRot;
        parts[P_ARM0].zRot = 0.0f;            parts[P_ARM1].zRot = 0.0f;
        parts[P_ARM0].yRot = -0.1f + hy;      parts[P_ARM1].yRot = 0.1f + hy + 0.4f;
        parts[P_ARM0].xRot = -PIF / 2.0f + hx; parts[P_ARM1].xRot = -PIF / 2.0f + hx;
        parts[P_ARM0].zRot += bcos; parts[P_ARM1].zRot -= bcos;
        parts[P_ARM0].xRot += bsin; parts[P_ARM1].xRot -= bsin;
    }

    int bx = (int)floorf(ix), by = (int)floorf(iy), bz = (int)floorf(iz);
    unsigned int brCol = g_brightColor[lightRawAt(&g_world, bx, by, bz)];

    if (p->hurtTime > 0 || p->deathTime > 0) {
        const unsigned int HURT_GB = 140;
        unsigned int r  =  brCol         & 0xFFu;
        unsigned int g  = (((brCol >> 8)  & 0xFFu) * HURT_GB) / 255;
        unsigned int b  = (((brCol >> 16) & 0xFFu) * HURT_GB) / 255;
        brCol = (brCol & 0xFF000000u) | (b << 16) | (g << 8) | r;
    }
    for (int i = 0; i < P_COUNT; i++) {
        for (int k = 0; k < 36; k++) { parts[i].mesh[k] = parts[i].base[k]; parts[i].mesh[k].color = brCol; }
        sceKernelDcacheWritebackInvalidateRange(parts[i].mesh, sizeof(parts[i].mesh));
    }

    textureBind(&g_localSkinTex);
    sceGuDisable(GU_CULL_FACE);

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();

    int sdir = 0;
    float ax = ix, az = iz, af = feet;
    if (p->isSleeping()) {
        sdir = worldData(&g_world, p->bedX, p->bedY, p->bedZ) & 3;
        static const float BOX[4] = {  0.0f, 1.8f,  0.0f, -1.8f };
        static const float BOZ[4] = { -1.8f, 0.0f,  1.8f,  0.0f };
        ax += BOX[sdir]; az += BOZ[sdir];
    }
    ScePspFVector3 tpos = { ax - g_relBaseX, af - g_relBaseY, az - g_relBaseZ }; sceGumTranslate(&tpos);
    if (p->isSleeping()) {
        static const float SLEEP_ROT[4] = { 90.0f, 0.0f, 270.0f, 180.0f };

        sceGumRotateY((180.0f - SLEEP_ROT[sdir]) * DEG2RAD);
        sceGumRotateZ(-90.0f * DEG2RAD);
        sceGumRotateY( 90.0f * DEG2RAD);
    } else {

        sceGumRotateY((ibody + 180.0f) * DEG2RAD);

        if (p->deathTime > 0) {
            float fall = sqrtf(((p->deathTime + a - 1.0f) / 20.0f) * 1.6f);
            if (fall > 1.0f) fall = 1.0f;
            sceGumRotateZ(fall * 90.0f * DEG2RAD);
        }
    }
    ScePspFVector3 sc = { -1.0f/16.0f, -1.0f/16.0f, 1.0f/16.0f };  sceGumScale(&sc);
    ScePspFVector3 gnd = { 0.0f, -24.0f, 0.0f };       sceGumTranslate(&gnd);

    for (int i = 0; i < P_COUNT; i++) {
        sceGumPushMatrix();
        ScePspFVector3 piv = { parts[i].px, parts[i].py, parts[i].pz };
        sceGumTranslate(&piv);
        if (parts[i].zRot != 0.0f) sceGumRotateZ(parts[i].zRot);
        if (parts[i].yRot != 0.0f) sceGumRotateY(parts[i].yRot);
        if (parts[i].xRot != 0.0f) sceGumRotateX(parts[i].xRot);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                        36, 0, parts[i].mesh);
        sceGumPopMatrix();
    }

    drawArmorLayers(brCol);

    if (g_haveTerrain) {
        ItemInstance* held = g_level.player->inventory->getSelected();
        if (held && !held->isNull()) {
            short id = held->id; unsigned char data = held->data;
            static ItemModelRenderer model;
            if (model.build(id, data, bowStage)) {
                sceGumPushMatrix();
                ScePspFVector3 ap = { parts[P_ARM0].px, parts[P_ARM0].py, parts[P_ARM0].pz };
                sceGumTranslate(&ap);
                if (parts[P_ARM0].zRot != 0.0f) sceGumRotateZ(parts[P_ARM0].zRot);
                if (parts[P_ARM0].yRot != 0.0f) sceGumRotateY(parts[P_ARM0].yRot);
                if (parts[P_ARM0].xRot != 0.0f) sceGumRotateX(parts[P_ARM0].xRot);
                ScePspFVector3 fist = { -1.0f, 7.0f, 1.0f }; sceGumTranslate(&fist);
                ScePspFVector3 s16  = { 16.0f, 16.0f, 16.0f }; sceGumScale(&s16);

                const bool isFlatItem  = model.isFlat();
                const bool mirroredPose = isFlatItem &&
                    (id == ITEM_BOW || (Item::items[id] && Item::items[id]->isHandEquipped()));

                if (!model.isFlat()) {
                    ScePspFVector3 bo = { 0.0f, 3.0f/16.0f, -5.0f/16.0f }; sceGumTranslate(&bo);

                    sceGumRotateX(20.0f * DEG2RAD); sceGumRotateY(225.0f * DEG2RAD);

                    ScePspFVector3 bs = { 0.375f, -0.375f, 0.375f }; sceGumScale(&bs);
                    ScePspFVector3 un = { -0.5f, -150.5f, -0.5f }; sceGumTranslate(&un);

                    if (!isPlant(id)) { sceGuEnable(GU_CULL_FACE); sceGuFrontFace(GU_CW); }
                } else if (id == ITEM_BOW) {

                    ScePspFVector3 bt = { 0.0f, 2.0f/16.0f, 5.0f/16.0f }; sceGumTranslate(&bt);
                    sceGumRotateY(-20.0f * DEG2RAD);
                    ScePspFVector3 bsc = { 10.0f/16.0f, -10.0f/16.0f, 10.0f/16.0f }; sceGumScale(&bsc);
                    sceGumRotateX(-100.0f * DEG2RAD); sceGumRotateY(45.0f * DEG2RAD);
                    ItemModelRenderer::applyFlatPreTransform();
                } else if (Item::items[id] && Item::items[id]->isHandEquipped()) {

                    ScePspFVector3 ht = { 0.0f, 3.0f/16.0f, 0.0f }; sceGumTranslate(&ht);
                    ScePspFVector3 hs = { 10.0f/16.0f, -10.0f/16.0f, 10.0f/16.0f }; sceGumScale(&hs);
                    sceGumRotateX(-100.0f * DEG2RAD); sceGumRotateY(45.0f * DEG2RAD);
                    ItemModelRenderer::applyFlatPreTransform();
                } else {

                    ScePspFVector3 fo = { 4.0f/16.0f, 3.0f/16.0f, -3.0f/16.0f }; sceGumTranslate(&fo);
                    ScePspFVector3 fs = { 6.0f/16.0f, 6.0f/16.0f, 6.0f/16.0f }; sceGumScale(&fs);
                    sceGumRotateZ(60.0f * DEG2RAD); sceGumRotateX(-90.0f * DEG2RAD); sceGumRotateZ(20.0f * DEG2RAD);
                    ItemModelRenderer::applyFlatPreTransform();
                }

                if (isFlatItem) {
                    sceGuEnable(GU_CULL_FACE);
                    sceGuFrontFace(mirroredPose ? GU_CW : GU_CCW);
                }
                model.draw(brCol, true);

                sceGuFrontFace(GU_CCW); sceGuDisable(GU_CULL_FACE);
                sceGumPopMatrix();
            }
        }
    }

    sceGumPopMatrix();
    sceGuEnable(GU_CULL_FACE);

    renderEntityShadow(ix, feet, iz, 0.0f, 0.5f, 1.0f);
}

void playerModelRenderPreview(float sx, float sy, float scale) {
    loadLocalPlayerSkinIfNeeded();
    if (!g_haveLocalSkin) return;
    buildParts();

    float t = nowSeconds() * 20.0f;
    float xd = 10.0f * sinf(t * 0.05f);
    float yd = 10.0f * cosf(t * 0.05f);
    float headYaw   = atanf(xd / 40.0f) * 20.0f;
    float headPitch = atanf(yd / 40.0f) * -20.0f;

    float ws = 0.25f;
    float phase = t * 0.25f * 0.6662f;
    float tcos0 = cosf(phase) * ws, tcos1 = cosf(phase + PIF) * ws;

    parts[P_HEAD].xRot = headPitch * DEG2RAD; parts[P_HEAD].yRot = headYaw * DEG2RAD; parts[P_HEAD].zRot = 0.0f;
    parts[P_BODY].xRot = parts[P_BODY].yRot = parts[P_BODY].zRot = 0.0f;
    parts[P_ARM0].xRot = tcos1; parts[P_ARM0].yRot = parts[P_ARM0].zRot = 0.0f;
    parts[P_ARM1].xRot = tcos0; parts[P_ARM1].yRot = parts[P_ARM1].zRot = 0.0f;
    parts[P_LEG0].xRot = tcos0 * 1.4f; parts[P_LEG0].yRot = parts[P_LEG0].zRot = 0.0f;
    parts[P_LEG1].xRot = tcos1 * 1.4f; parts[P_LEG1].yRot = parts[P_LEG1].zRot = 0.0f;

    float bcos = cosf(t * 0.09f) * 0.05f + 0.05f;
    float bsin = sinf(t * 0.067f) * 0.05f;
    parts[P_ARM0].zRot += bcos; parts[P_ARM1].zRot -= bcos;
    parts[P_ARM0].xRot += bsin; parts[P_ARM1].xRot -= bsin;

    for (int i = 0; i < P_COUNT; i++) {
        for (int k = 0; k < 36; k++) { parts[i].mesh[k] = parts[i].base[k]; parts[i].mesh[k].color = 0xFFFFFFFFu; }
        sceKernelDcacheWritebackInvalidateRange(parts[i].mesh, sizeof(parts[i].mesh));
    }

    sceGumMatrixMode(GU_PROJECTION); sceGumPushMatrix(); sceGumLoadIdentity();
    sceGumOrtho(0.0f, 480.0f, 272.0f, 0.0f, -200.0f, 200.0f);
    sceGumMatrixMode(GU_VIEW); sceGumPushMatrix(); sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL); sceGumPushMatrix(); sceGumLoadIdentity();

    ScePspFVector3 pos = { sx, sy, 0.0f }; sceGumTranslate(&pos);
    ScePspFVector3 sc  = { -scale, scale, scale }; sceGumScale(&sc);
    sceGumRotateY(PIF);

    {
        int x0 = (int)(sx - 10.0f * scale), y0 = (int)(sy - 10.0f * scale);
        int w  = (int)(20.0f * scale),      h  = (int)(36.0f * scale);
        sceGuScissor(x0 < 0 ? 0 : x0, y0 < 0 ? 0 : y0, w, h);
        sceGuClearDepth(0);
        sceGuClear(GU_DEPTH_BUFFER_BIT);
    }
    sceGuEnable(GU_DEPTH_TEST);

    textureBind(&g_localSkinTex);
    sceGuDisable(GU_CULL_FACE);
    for (int i = 0; i < P_COUNT; i++) {
        sceGumPushMatrix();
        ScePspFVector3 piv = { parts[i].px, parts[i].py, parts[i].pz };
        sceGumTranslate(&piv);
        if (parts[i].zRot != 0.0f) sceGumRotateZ(parts[i].zRot);
        if (parts[i].yRot != 0.0f) sceGumRotateY(parts[i].yRot);
        if (parts[i].xRot != 0.0f) sceGumRotateX(parts[i].xRot);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                        36, 0, parts[i].mesh);
        sceGumPopMatrix();
    }
    drawArmorLayers(0xFFFFFFFFu);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuEnable(GU_CULL_FACE);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuScissor(0, 0, 480, 272);

    sceGumMatrixMode(GU_PROJECTION); sceGumPopMatrix();
    sceGumMatrixMode(GU_VIEW); sceGumPopMatrix();
    sceGumMatrixMode(GU_MODEL); sceGumPopMatrix();
}
