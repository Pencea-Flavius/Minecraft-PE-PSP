
#include "client/renderer/entity/player_model.h"
#include "client/renderer/entity/mob_model.h"
#include "platform/dcache.h"
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "world/item/armor_item.h"
#include "client/renderer/item_anim_icon.h"

#include "gpu/texture.h"
#include "client/skin/skin_pack.h"
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

struct Part { MobVertex base[36]; float px, py, pz; float xRot, yRot, zRot; };

enum { P_HEAD, P_BODY, P_ARM0, P_ARM1, P_LEG0, P_LEG1, P_HAT, P_COUNT };
static Part parts[P_COUNT];
static bool g_built = false;

static inline void buildBox(MobVertex* out,
                     float x0, float y0, float z0, float x1, float y1, float z1,
                     int tx, int ty, int w, int h, int d, bool mirror, float texH = 32.0f) {
    mobBuildBox(out, x0,y0,z0, x1,y1,z1, tx,ty, w,h,d, mirror, 0.0f, 64.0f, texH, true);
}

static MobVertex g_base64[P_COUNT][36];
static MobVertex g_over64[P_COUNT][36];
static bool      g_built64 = false;

static void buildParts64(void) {
    if (g_built64) return;
    const float H = 64.0f, g = 0.25f;
    buildBox(g_base64[P_HEAD], -4,-8,-4,  4, 0, 4,  0,  0, 8,8,8, false, H);
    buildBox(g_base64[P_BODY], -4, 0,-2,  4,12, 2, 16, 16, 8,12,4, false, H);
    buildBox(g_base64[P_ARM0], -3,-2,-2,  1,10, 2, 40, 16, 4,12,4, false, H);
    buildBox(g_base64[P_ARM1], -1,-2,-2,  3,10, 2, 32, 48, 4,12,4, false, H);
    buildBox(g_base64[P_LEG0], -2, 0,-2,  2,12, 2,  0, 16, 4,12,4, false, H);
    buildBox(g_base64[P_LEG1], -2, 0,-2,  2,12, 2, 16, 48, 4,12,4, false, H);
    buildBox(g_base64[P_HAT],  -4.5f,-8.5f,-4.5f, 4.5f,0.5f,4.5f, 32, 0, 8,8,8, false, H);
    buildBox(g_over64[P_BODY], -4-g, 0-g,-2-g,  4+g,12+g, 2+g, 16, 32, 8,12,4, false, H);
    buildBox(g_over64[P_ARM0], -3-g,-2-g,-2-g,  1+g,10+g, 2+g, 40, 32, 4,12,4, false, H);
    buildBox(g_over64[P_ARM1], -1-g,-2-g,-2-g,  3+g,10+g, 2+g, 48, 48, 4,12,4, false, H);
    buildBox(g_over64[P_LEG0], -2-g, 0-g,-2-g,  2+g,12+g, 2+g,  0, 32, 4,12,4, false, H);
    buildBox(g_over64[P_LEG1], -2-g, 0-g,-2-g,  2+g,12+g, 2+g,  0, 48, 4,12,4, false, H);
    dcacheFlush(g_base64, sizeof(g_base64));
    dcacheFlush(g_over64, sizeof(g_over64));
    g_built64 = true;
}

static inline bool isTall(const Texture* t) { return t && t->realW == 64 && t->realH == 64; }

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

    parts[P_LEG0].px = -1.9f; parts[P_LEG0].py = 12; parts[P_LEG0].pz = 0;

    buildBox(parts[P_LEG1].base, -2, 0,-2,  2,12, 2,  0, 16, 4,12,4, true);
    parts[P_LEG1].px = 1.9f; parts[P_LEG1].py = 12; parts[P_LEG1].pz = 0;

    buildBox(parts[P_HAT].base, -4.5f,-8.5f,-4.5f,  4.5f, 0.5f, 4.5f, 32, 0, 8,8,8, false);
    parts[P_HAT].px = 0;   parts[P_HAT].py = 0;  parts[P_HAT].pz = 0;
    dcacheFlush(parts, sizeof(parts));
    g_built = true;
}

static MobVertex     g_skinBoxMesh[SKIN_MAX_BOXES][36];
static unsigned char g_skinBoxPart[SKIN_MAX_BOXES];
static int           g_skinBoxCount = 0;
static unsigned int  g_skinBoxGen = 0;

int playerModelBuildSkinBoxes(const SkinBox* b, int n, MobVertex (*out)[36],
                              unsigned char* part, int max, float texH) {
    int count = 0;
    for (int i = 0; b && i < n && count < max; i++) {
        const SkinBox& s = b[i];

        bool dup = false;
        for (int k = 0; k < i && !dup; k++)
            dup = b[k].part == s.part && b[k].x == s.x && b[k].y == s.y && b[k].z == s.z &&
                  (int)b[k].w == (int)s.w && (int)b[k].h == (int)s.h && (int)b[k].d == (int)s.d &&
                  (int)b[k].u == (int)s.u && (int)b[k].v == (int)s.v;
        if (dup) continue;
        int w = (int)s.w, h = (int)s.h, d = (int)s.d;
        buildBox(out[count], s.x, s.y, s.z, s.x + w, s.y + h, s.z + d,
                 (int)s.u, (int)s.v, w, h, d, false, texH);
        part[count] = s.part;
        count++;
    }
    dcacheFlush(out, sizeof(out[0]) * (count ? count : 1));
    return count;
}

static void buildSkinBoxes(void) {
    unsigned int gen = skinGeneration();
    if (gen == g_skinBoxGen) return;
    g_skinBoxGen = gen;
    const SkinEntry* e = skinCurrent();
    g_skinBoxCount = e ? playerModelBuildSkinBoxes(skinBoxes(), e->boxCount, g_skinBoxMesh,
                                                   g_skinBoxPart, SKIN_MAX_BOXES,
                                                   isTall(skinTexture()) ? 64.0f : 32.0f) : 0;
}

static SkinDraw wornSkin(void) {
    buildSkinBoxes();
    const SkinEntry* e = skinCurrent();
    SkinDraw d = { skinTexture(), g_skinBoxMesh, g_skinBoxPart, g_skinBoxCount,
                   skinAnim(), skinCapeTexture(), e ? e->helmetY : 0.0f };
    return d;
}

static void drawSkinBoxes(const SkinDraw& sd, int part, unsigned int brCol, bool lit,
                          const float* toWorld) {
    for (int k = 0; k < sd.boxCount; k++) {
        if (sd.boxPart[k] != part) continue;
        if (lit) {
            mobDrawPartLit(sd.boxMesh[k], brCol, toWorld);
        } else {
            sceGumDrawArray(GU_TRIANGLES,
                            GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                            36, 0, sd.boxMesh[k]);
        }
    }
}

void playerModelDrawSkinBoxes(int part, unsigned int brCol, const float* toWorld) {
    drawSkinBoxes(wornSkin(), part, brCol, true, toWorld);
}

static MobVertex g_armor1[P_HAT][36];
static MobVertex g_armor05[P_HAT][36];
static bool      g_armorBuilt = false;

static void buildArmorSet(MobVertex set[][36], float inf) {
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
    dcacheFlush(g_armor1,  sizeof(g_armor1));
    dcacheFlush(g_armor05, sizeof(g_armor05));
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

static void drawArmorLayers(unsigned int brCol, bool lit, const float* toWorld, float helmetY) {

    LocalPlayer* p = g_level.player;
    if (!p) return;

    if (skinAnim() & (1u << SKIN_ANIM_DONT_RENDER_ARMOUR)) return;
    sceGuColor(brCol);
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
    for (int li = 0; li < 4; li++) {
        const ArmorLayer& ly = layers[li];
        ItemInstance& ai = p->armor[ly.slot];
        if (ai.isNull()) continue;
        Item* it = ai.getItem();
        if (!it || !it->isArmor()) continue;
        Texture* tex = armorTexture((ai.id - 298) / 4, ly.file);
        if (!tex) continue;
        textureBind(tex);
        MobVertex (*set)[36] = ly.inner ? g_armor05 : g_armor1;
        for (int k = 0; k < ly.n; k++) {
            int i = ly.parts[k];
            sceGumPushMatrix();
            ScePspFVector3 piv = { parts[i].px, parts[i].py, parts[i].pz };
            sceGumTranslate(&piv);
            if (parts[i].zRot != 0.0f) sceGumRotateZ(parts[i].zRot);
            if (parts[i].yRot != 0.0f) sceGumRotateY(parts[i].yRot);
            if (parts[i].xRot != 0.0f) sceGumRotateX(parts[i].xRot);
            if (ly.slot == ArmorItem::SLOT_HEAD && helmetY != 0.0f) {

                ScePspFVector3 off = { 0.0f, helmetY, 0.0f };
                sceGumTranslate(&off);
            }
            if (lit) {
                mobDrawPartLit(set[i], brCol, toWorld);
            } else {
                sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    36, 0, set[i]);
            }
            sceGumPopMatrix();
        }
    }
}

static void setPivots(bool sneaking) {
    parts[P_HEAD].py = sneaking ? 1.0f : 0.0f;
    parts[P_LEG0].py = parts[P_LEG1].py = sneaking ? 9.0f : 12.0f;

    parts[P_LEG0].pz = parts[P_LEG1].pz = sneaking ? 4.0f : 0.1f;
    parts[P_ARM0].px = -5.0f; parts[P_ARM0].pz = 0.0f;
    parts[P_ARM1].px =  5.0f; parts[P_ARM1].pz = 0.0f;
}

static void applySkinAnim(unsigned int anim, float tcos0, float tcos1, bool holding,
                          bool attacking, bool riding) {
    if (!anim) return;
    #define ANIM(b) (anim & (1u << SKIN_ANIM_##b))
    const float HALF_PI = PIF * 0.5f;
    if (ANIM(ARMS_DOWN)) {
        parts[P_ARM0].xRot = parts[P_ARM1].xRot = 0.0f;
        parts[P_ARM0].zRot = parts[P_ARM1].zRot = 0.0f;
    } else if (ANIM(ARMS_OUT_FRONT)) {
        parts[P_ARM0].xRot = parts[P_ARM1].xRot = -HALF_PI;
        parts[P_ARM0].zRot = parts[P_ARM1].zRot = 0.0f;
    } else if (ANIM(SINGLE_ARMS)) {
        parts[P_ARM0].xRot = parts[P_ARM1].xRot = tcos1;
        parts[P_ARM0].zRot = parts[P_ARM1].zRot = 0.0f;
    } else if (ANIM(STATUE_OF_LIBERTY) && !holding && !attacking) {

        parts[P_ARM0].xRot = -PIF;
        parts[P_ARM0].zRot = -0.3f;
        parts[P_ARM1].xRot = tcos0;
        parts[P_ARM1].zRot = 0.0f;
    }

    if (!riding) {
        if (ANIM(NO_LEG_ANIM)) {
            parts[P_LEG0].xRot = parts[P_LEG0].yRot = parts[P_LEG0].zRot = 0.0f;
            parts[P_LEG1].xRot = parts[P_LEG1].yRot = parts[P_LEG1].zRot = 0.0f;
        } else if (ANIM(SINGLE_LEGS)) {
            parts[P_LEG0].xRot = parts[P_LEG1].xRot = tcos0 * 1.4f;
        }
    }
    #undef ANIM
}

static int posePlayer(LocalPlayer* p, float a, float headYaw, float headPitch) {

    float ws = p->walkAnimSpeedO + (p->walkAnimSpeed - p->walkAnimSpeedO) * a;
    if (ws > 1.0f) ws = 1.0f;
    float wp = p->walkAnimPos - p->walkAnimSpeed * (1.0f - a);
    float t  = wp * 0.6662f;
    float tcos0 = cosf(t) * ws, tcos1 = cosf(t + PIF) * ws;

    parts[P_HEAD].xRot = headPitch; parts[P_HEAD].yRot = headYaw;
    parts[P_BODY].xRot = parts[P_BODY].yRot = parts[P_BODY].zRot = 0.0f;
    parts[P_ARM0].xRot = tcos1; parts[P_ARM0].yRot = parts[P_ARM0].zRot = 0.0f;
    parts[P_ARM1].xRot = tcos0; parts[P_ARM1].yRot = parts[P_ARM1].zRot = 0.0f;
    parts[P_LEG0].xRot = tcos0 * 1.4f; parts[P_LEG1].xRot = tcos1 * 1.4f;
    parts[P_LEG0].yRot = parts[P_LEG1].yRot = 0.0f;

    {
        ItemInstance* sel = p->inventory->getSelected();
        float d = g_attackAnim - g_oAttackAnim; if (d < 0.0f) d += 1.0f;
        float atkNow = g_oAttackAnim + d * a; if (atkNow > 1.0f) atkNow -= 1.0f;
        applySkinAnim(skinAnim(), tcos0, tcos1, sel && !sel->isNull(), atkNow > 0.001f, p->riding);
    }

    if (p->riding) {
        const float HALF_PI = PIF * 0.5f;
        parts[P_ARM0].xRot += -HALF_PI * 0.4f;
        parts[P_ARM1].xRot += -HALF_PI * 0.4f;
        parts[P_LEG0].xRot = -HALF_PI * 0.8f;
        parts[P_LEG1].xRot = -HALF_PI * 0.8f;
        parts[P_LEG0].yRot =  HALF_PI * 0.2f;
        parts[P_LEG1].yRot = -HALF_PI * 0.2f;
    }

    ItemInstance* selHeld = p->inventory->getSelected();
    bool holding = selHeld && !selHeld->isNull();

    Item* useItem = p->isUsingItem() ? p->getUseItem()->getItem() : 0;
    bool aiming = holding && useItem && useItem->getUseAnimation() == 4;

    int  bowStage = aiming ? bowStageIcon((float)p->getTicksUsingItem())
                           : (holding ? itemAnimStage(selHeld->id, p) : -1);

    if (holding) parts[P_ARM0].xRot = parts[P_ARM0].xRot * 0.5f - PIF / 2.0f * 0.2f;

    float ageT = gameSeconds() * 20.0f;

    float bcos = cosf(ageT * 0.09f) * 0.05f + 0.05f;
    float bsin = sinf(ageT * 0.067f) * 0.05f;
    parts[P_ARM0].zRot += bcos; parts[P_ARM1].zRot -= bcos;
    parts[P_ARM0].xRot += bsin; parts[P_ARM1].xRot -= bsin;

    float diff = g_attackAnim - g_oAttackAnim; if (diff < 0.0f) diff += 1.0f;
    float atk = g_oAttackAnim + diff * a; if (atk > 1.0f) atk -= 1.0f;

    setPivots(p->sneaking);
    if (atk > 0.001f) {
        float f = 1.0f - atk; f *= f; f *= f; f = 1.0f - f;
        float s1 = sinf(f * PIF);
        parts[P_BODY].yRot  = sinf(sqrtf(atk) * PIF * 2.0f) * 0.2f;

        parts[P_ARM0].pz =  sinf(parts[P_BODY].yRot) * 5.0f;
        parts[P_ARM0].px = -cosf(parts[P_BODY].yRot) * 5.0f;
        parts[P_ARM1].pz = -sinf(parts[P_BODY].yRot) * 5.0f;
        parts[P_ARM1].px =  cosf(parts[P_BODY].yRot) * 5.0f;
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

    if (p->sneaking) {
        parts[P_BODY].xRot += 0.5f;
        parts[P_ARM0].xRot += 0.4f; parts[P_ARM1].xRot += 0.4f;
    }

    return bowStage;
}

static unsigned int playerLightColor(LocalPlayer* p, float ix, float iy, float iz) {
    int bx = (int)floorf(ix), by = (int)floorf(iy), bz = (int)floorf(iz);
    unsigned int brCol = brightColorFloored(lightRawAt(&g_world, bx, by, bz), ENTITY_LIGHT_FLOOR);

    if (p->hurtTime > 0 || p->deathTime > 0) {
        const unsigned int HURT_GB = 140;
        unsigned int r  =  brCol         & 0xFFu;
        unsigned int g  = (((brCol >> 8)  & 0xFFu) * HURT_GB) / 255;
        unsigned int b  = (((brCol >> 16) & 0xFFu) * HURT_GB) / 255;
        brCol = (brCol & 0xFF000000u) | (b << 16) | (g << 8) | r;
    }
    return brCol;
}

static void drawPlayerBody(const SkinDraw& sd, unsigned int brCol, bool lit, const float* toWorld,
                           bool armour = true) {

    parts[P_HAT].px   = parts[P_HEAD].px;
    parts[P_HAT].py   = parts[P_HEAD].py;
    parts[P_HAT].pz   = parts[P_HEAD].pz;
    parts[P_HAT].xRot = parts[P_HEAD].xRot;
    parts[P_HAT].yRot = parts[P_HEAD].yRot;
    parts[P_HAT].zRot = parts[P_HEAD].zRot;

    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);

    textureBind(sd.tex);

    sceGuTexWrap(GU_REPEAT, GU_REPEAT);
    sceGuDisable(GU_CULL_FACE);
    sceGuColor(brCol);

    static const unsigned char kDisableBit[P_COUNT] = {
        SKIN_ANIM_DISABLE_HEAD, SKIN_ANIM_DISABLE_TORSO, SKIN_ANIM_DISABLE_ARM0,
        SKIN_ANIM_DISABLE_ARM1, SKIN_ANIM_DISABLE_LEG0, SKIN_ANIM_DISABLE_LEG1,
        SKIN_ANIM_DISABLE_HAIR,
    };
    const unsigned int anim = sd.anim;
    const bool tall = isTall(sd.tex);
    if (tall) buildParts64();
    for (int i = 0; i < P_COUNT; i++) {
        sceGumPushMatrix();
        ScePspFVector3 piv = { parts[i].px, parts[i].py, parts[i].pz };
        sceGumTranslate(&piv);
        if (parts[i].zRot != 0.0f) sceGumRotateZ(parts[i].zRot);
        if (parts[i].yRot != 0.0f) sceGumRotateY(parts[i].yRot);
        if (parts[i].xRot != 0.0f) sceGumRotateX(parts[i].xRot);
        const MobVertex* base = tall ? g_base64[i] : parts[i].base;

        const MobVertex* over = (tall && i >= P_BODY && i <= P_LEG1) ? g_over64[i] : 0;
        if (anim & (1u << kDisableBit[i])) {

        } else if (lit) {
            mobDrawPartLit(base, brCol, toWorld);
            if (over) mobDrawPartLit(over, brCol, toWorld);
        } else {
            sceGumDrawArray(GU_TRIANGLES,
                            GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                            36, 0, base);
            if (over)
                sceGumDrawArray(GU_TRIANGLES,
                                GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                                36, 0, over);
        }
        drawSkinBoxes(sd, i, brCol, lit, toWorld);
        sceGumPopMatrix();
    }
    sceGuColor(brCol);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    if (armour) drawArmorLayers(brCol, lit, toWorld, sd.helmetY);
}

static float interpBodyYaw(LocalPlayer* p, float a);

static void drawCapeAt(Texture* tex, float xRot, float lean2, unsigned int brCol, bool lit,
                       const float* toWorld) {
    static MobVertex cloak[36];
    static bool built = false;
    if (!built) {
        buildBox(cloak, -5, 0, -1, 5, 16, 0, 0, 0, 10, 16, 1, false);
        dcacheFlush(cloak, sizeof(cloak));
        built = true;
    }
    textureBind(tex);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);
    sceGumPushMatrix();
    ScePspFVector3 back = { 0.0f, 0.0f, 2.0f };
    sceGumTranslate(&back);
    sceGumRotateX(xRot * DEG2RAD);
    sceGumRotateZ(lean2 / 2.0f * DEG2RAD);
    sceGumRotateY(-lean2 / 2.0f * DEG2RAD);
    sceGumRotateY(PIF);
    if (lit) mobDrawPartLit(cloak, brCol, toWorld);
    else sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                         36, 0, cloak);
    sceGumPopMatrix();
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuColor(brCol);
}

static void drawCape(LocalPlayer* p, float a, unsigned int brCol, const float* toWorld) {
    Texture* tex = skinCapeTexture();
    if (!tex) return;
    float xd = (p->xCloakO + (p->xCloak - p->xCloakO) * a) - (p->xo + (p->x - p->xo) * a);
    float yd = (p->yCloakO + (p->yCloak - p->yCloakO) * a) - (p->yo + (p->y - p->yo) * a);
    float zd = (p->zCloakO + (p->zCloak - p->zCloakO) * a) - (p->zo + (p->z - p->zo) * a);
    float yr = interpBodyYaw(p, a);
    float xa = sinf(yr * DEG2RAD), za = -cosf(yr * DEG2RAD);
    float flap = yd * 10.0f;
    if (flap < -6.0f) flap = -6.0f;
    if (flap > 32.0f) flap = 32.0f;
    float lean  = (xd * xa + zd * za) * 100.0f;
    float lean2 = (xd * za - zd * xa) * 100.0f;
    if (lean < 0.0f) lean = 0.0f;
    float bobv = p->oBob + (p->bob - p->oBob) * a;
    flap += sinf((p->walkDistO + (p->walkDist - p->walkDistO) * a) * 6.0f) * 32.0f * bobv;
    if (p->sneaking) flap += 25.0f;
    float xRot = 6.0f + lean / 2.0f + flap;
    if (xRot > 64.0f) xRot = 64.0f;
    drawCapeAt(tex, xRot, lean2, brCol, true, toWorld);
}

static void drawHeldItem(LocalPlayer* p, int bowStage, unsigned int brCol) {
    sceGuColor(0xFFFFFFFFu);

    sceGuDisable(GU_BLEND);
    if (g_haveTerrain) {
        ItemInstance* held = p->inventory->getSelected();
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

                    if (!isCrossShaped(id)) { sceGuEnable(GU_CULL_FACE); sceGuFrontFace(GU_CW); }
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

    sceGuEnable(GU_BLEND);
}

static void rotateLikeMobRenderer(LocalPlayer* p, float ibody, float a) {
    sceGumRotateY((180.0f - ibody) * DEG2RAD);
    if (p->deathTime > 0) {
        float fall = sqrtf(((p->deathTime + a - 1.0f) / 20.0f) * 1.6f);
        if (fall > 1.0f) fall = 1.0f;
        sceGumRotateZ(fall * 90.0f * DEG2RAD);
    }
}

static void drawPlayerModel(LocalPlayer* p, float a, float headYaw, float headPitch,
                            unsigned int brCol, const float* toWorld) {
    ScePspFVector3 sc = { -1.0f/16.0f, -1.0f/16.0f, 1.0f/16.0f };  sceGumScale(&sc);

    float gndY = -24.0f + (p->sneaking ? 3.0f : 0.0f);
    ScePspFVector3 gnd = { 0.0f, gndY, 0.0f };       sceGumTranslate(&gnd);

    int bowStage = posePlayer(p, a, headYaw, headPitch);
    drawPlayerBody(wornSkin(), brCol, true, toWorld);
    drawCape(p, a, brCol, toWorld);
    drawHeldItem(p, bowStage, brCol);
}

static float interpBodyYaw(LocalPlayer* p, float a) {
    float dBody = p->yBodyRot - p->yBodyRotO; while (dBody > 180.0f) dBody -= 360.0f; while (dBody < -180.0f) dBody += 360.0f;
    return p->yBodyRotO + dBody * a;
}

static void beginGuiModel(int x0, int y0, int w, int h) {
    sceGumMatrixMode(GU_PROJECTION); sceGumPushMatrix(); sceGumLoadIdentity();
    sceGumOrtho(0.0f, 480.0f, 272.0f, 0.0f, -200.0f, 200.0f);
    sceGumMatrixMode(GU_VIEW); sceGumPushMatrix(); sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL); sceGumPushMatrix(); sceGumLoadIdentity();
    sceGuScissor(x0 < 0 ? 0 : x0, y0 < 0 ? 0 : y0, w, h);
    sceGuClearDepth(0);
    sceGuClear(GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_DEPTH_TEST);
}
static void endGuiModel(bool alphaTest) {
    if (alphaTest) sceGuEnable(GU_ALPHA_TEST); else sceGuDisable(GU_ALPHA_TEST);
    sceGuEnable(GU_CULL_FACE);
    sceGuFrontFace(GU_CW);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuScissor(0, 0, 480, 272);
    sceGumMatrixMode(GU_PROJECTION); sceGumPopMatrix();
    sceGumMatrixMode(GU_VIEW); sceGumPopMatrix();
    sceGumMatrixMode(GU_MODEL); sceGumPopMatrix();
}

void playerModelRender(float a) {
    LocalPlayer* p = g_level.player;
    if (!p) return;

    if (p->health <= 0 && p->deathTime >= 20) return;
    if (!skinTexture()) return;
    buildParts();

    float ix = p->xo + (p->x - p->xo) * a;
    float iy = p->yo + (p->y - p->yo) * a;
    float iz = p->zo + (p->z - p->zo) * a;
    float iyaw   = p->yRotO + (p->yRot - p->yRotO) * a;
    float ipitch = p->xRotO + (p->xRot - p->xRotO) * a;
    float feet   = iy - PLAYER_EYE;

    float ibody = interpBodyYaw(p, a);
    float dHead = iyaw - ibody; while (dHead > 180.0f) dHead -= 360.0f; while (dHead < -180.0f) dHead += 360.0f;

    unsigned int brCol = playerLightColor(p, ix, iy, iz);

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
        rotateLikeMobRenderer(p, ibody, a);
    }

    drawPlayerModel(p, a, dHead * DEG2RAD, -ipitch * DEG2RAD, brCol, 0);

    sceGumPopMatrix();
    sceGuEnable(GU_CULL_FACE);

    renderEntityShadow(ix, feet, iz, 0.0f, 0.5f, 1.0f);

    if (p->isOnFire())
        renderEntityFlame(ix, feet, iz, feet, p->bbWidth, p->bbHeight);
}

int g_animatedCharacter = 0;

static bool dollSprintSignal(LocalPlayer*) { return false; }

void playerModelRenderPaperDoll(float a, bool displayGui) {
    LocalPlayer* p = g_level.player;
    if (!p) return;

    static int characterDisplayTimer = 0;
    if (!displayGui)                characterDisplayTimer = 0;
    else if (p->sneaking)           characterDisplayTimer = 30;
    else if (dollSprintSignal(p))   characterDisplayTimer = 30;
    else if (p->flying)             characterDisplayTimer = 5;
    else if (characterDisplayTimer > 0) --characterDisplayTimer;

    const bool display = p->sneaking || dollSprintSignal(p) || p->flying || characterDisplayTimer > 0;
    if (!displayGui || !display) return;
    if (p->health <= 0 && p->deathTime >= 20) return;
    if (!skinTexture()) return;
    buildParts();

    const float DOLL_SCALE = 20.0f;
    const float DOLL_TOP   = 26.0f;
    const float DOLL_X = 20.0f, DOLL_Y = DOLL_TOP + 2.0f * DOLL_SCALE;
    const float xd = -40.0f, yd = 10.0f;

    const int rx = (int)(DOLL_X - 1.5f * DOLL_SCALE), ry = (int)(DOLL_Y - 2.6f * DOLL_SCALE);
    beginGuiModel(rx, ry, (int)(3.0f * DOLL_SCALE), (int)(3.2f * DOLL_SCALE));

    ScePspFVector3 pos = { DOLL_X, DOLL_Y, 50.0f };               sceGumTranslate(&pos);
    ScePspFVector3 ss  = { -DOLL_SCALE, DOLL_SCALE, DOLL_SCALE };  sceGumScale(&ss);
    sceGumRotateZ(PIF);

    static const float C = -0.70710678f;
    static const float toWorld[16] = {
         C,    0.0f, -C,   0.0f,
         0.0f, -1.0f, 0.0f, 0.0f,
         C,    0.0f,  C,   0.0f,
         0.0f, 0.0f,  0.0f, 1.0f,
    };
    sceGumRotateX(-atanf(yd / 40.0f) * 20.0f * DEG2RAD);
    float ibody = interpBodyYaw(p, a);

    sceGumRotateY((ibody - atanf(xd / 40.0f) * 20.0f) * DEG2RAD);

    rotateLikeMobRenderer(p, ibody, a);
    float ix = p->xo + (p->x - p->xo) * a;
    float iy = p->yo + (p->y - p->yo) * a;
    float iz = p->zo + (p->z - p->zo) * a;

    drawPlayerModel(p, a, 0.0f, -atanf(yd / 40.0f) * 20.0f * DEG2RAD,
                    playerLightColor(p, ix, iy, iz), toWorld);

    endGuiModel(true);
}

void playerModelRenderPreview(float sx, float sy, float scale) {
    if (!skinTexture()) return;
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
    applySkinAnim(skinAnim(), tcos0, tcos1, false, false, false);

    float bcos = cosf(t * 0.09f) * 0.05f + 0.05f;
    float bsin = sinf(t * 0.067f) * 0.05f;
    parts[P_ARM0].zRot += bcos; parts[P_ARM1].zRot -= bcos;
    parts[P_ARM0].xRot += bsin; parts[P_ARM1].xRot -= bsin;
    setPivots(false);

    beginGuiModel((int)(sx - 10.0f * scale), (int)(sy - 10.0f * scale),
                  (int)(20.0f * scale), (int)(36.0f * scale));

    ScePspFVector3 pos = { sx, sy, 0.0f }; sceGumTranslate(&pos);
    ScePspFVector3 sc  = { -scale, scale, scale }; sceGumScale(&sc);
    sceGumRotateY(PIF);
    drawPlayerBody(wornSkin(), 0xFFFFFFFFu, false, 0);
    endGuiModel(false);
}

void playerModelRenderSkinPreview(const SkinDraw& d, float x, float y, float w, float h,
                                  float yRotDeg, float walkPos, float walkSpeed,
                                  int pose, float swing) {
    if (!d.tex) return;
    buildParts();

    const bool sneaking  = (pose == SKIN_POSE_SNEAK);
    const bool attacking = (pose == SKIN_POSE_ATTACK);

    float ws = walkSpeed > 1.0f ? 1.0f : walkSpeed;
    float tcos0 = cosf(walkPos * 0.6662f) * ws, tcos1 = cosf(walkPos * 0.6662f + PIF) * ws;
    for (int i = 0; i < P_COUNT; i++) parts[i].xRot = parts[i].yRot = parts[i].zRot = 0.0f;
    parts[P_ARM0].xRot = tcos1;        parts[P_ARM1].xRot = tcos0;
    parts[P_LEG0].xRot = tcos0 * 1.4f; parts[P_LEG1].xRot = tcos1 * 1.4f;

    applySkinAnim(d.anim, tcos0, tcos1, attacking, attacking, false);

    parts[P_ARM0].zRot += 0.1f; parts[P_ARM1].zRot -= 0.1f;
    setPivots(sneaking);

    if (attacking && swing > 0.001f) {
        float f = 1.0f - swing; f *= f; f *= f; f = 1.0f - f;
        float s1 = sinf(f * PIF);
        parts[P_BODY].yRot  = sinf(sqrtf(swing) * PIF * 2.0f) * 0.2f;
        parts[P_ARM0].pz =  sinf(parts[P_BODY].yRot) * 5.0f;
        parts[P_ARM0].px = -cosf(parts[P_BODY].yRot) * 5.0f;
        parts[P_ARM1].pz = -sinf(parts[P_BODY].yRot) * 5.0f;
        parts[P_ARM1].px =  cosf(parts[P_BODY].yRot) * 5.0f;
        parts[P_ARM0].yRot += parts[P_BODY].yRot;
        parts[P_ARM1].yRot += parts[P_BODY].yRot;
        parts[P_ARM1].xRot += parts[P_BODY].yRot;
        parts[P_ARM0].xRot -= s1 * 1.2f + sinf(swing * PIF) * 0.7f * 0.75f;
        parts[P_ARM0].yRot += parts[P_BODY].yRot * 2.0f;
        parts[P_ARM0].zRot += sinf(swing * PIF) * -0.4f;
    }

    if (sneaking) {
        parts[P_BODY].xRot += 0.5f;
        parts[P_ARM0].xRot += 0.4f; parts[P_ARM1].xRot += 0.4f;
    }

    float scale = h / 32.0f;
    float s     = scale * 15.0f / 16.0f;
    beginGuiModel((int)x, (int)y, (int)w, (int)h);
    ScePspFVector3 pos = { x + w * 0.5f, y + h - 24.0f * s, 0.0f }; sceGumTranslate(&pos);
    ScePspFVector3 sc  = { -s, s, s };                               sceGumScale(&sc);
    sceGumRotateY(PIF);
    sceGumRotateY(yRotDeg * DEG2RAD);
    drawPlayerBody(d, 0xFFFFFFFFu, false, 0, false);
    if (d.cape) drawCapeAt(d.cape, 7.0f, 0.0f, 0xFFFFFFFFu, false, 0);
    endGuiModel(false);
}

void playerModelRenderWornPreview(float x, float y, float w, float h, float yRotDeg) {
    playerModelRenderSkinPreview(wornSkin(), x, y, w, h, yRotDeg, 0.0f, 0.0f);
}
