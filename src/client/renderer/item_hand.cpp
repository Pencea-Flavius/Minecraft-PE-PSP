
#include "item_hand.h"
#include "platform/dcache.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include <pspgu.h>
#include <pspgum.h>
#include <math.h>
#include <pspkernel.h>
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"
#include "client/player/player.h"
#include "client/player/player_state.h"
#include "world/item/item.h"
#include "client/gui/hud.h"
#include "client/gui/screens/menu.h"
#include "client/renderer/tileentity/tile_entity_renderer.h"
#include <string.h>
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "client/renderer/item_model.h"
#include "client/renderer/entity/mob_model.h"
#include "gpu/item_icons.h"
#include "gpu/spawn_egg_colors.h"
#include "client/renderer/item_anim_icon.h"

extern World g_world;
extern bool g_worldBuilt;
extern bool g_haveTerrain;
extern int g_noMipmap;
extern Texture g_terrain;
extern Texture g_guiBlocks;
extern bool g_haveGuiBlocks;

extern int   g_viewBobbing;

static const float PIF     = 3.14159265f;
static const float DEG2RAD = PIF / 180.0f;

bool  g_swinging    = false;
int   g_swingTime   = 0;
float g_attackAnim  = 0.0f;
float g_oAttackAnim = 0.0f;

static float s_equipHeight = 0.0f;
static float s_oEquipHeight = 0.0f;
static short s_equippedId = -1;
static unsigned char s_equippedData = 0xFF;

void playerSwing(void) {

    if (!g_swinging || g_swingTime >= 3 || g_swingTime < 0) {
        g_swingTime = -1;
        g_swinging  = true;
    }
}

void itemHandTick(void) {

    g_oAttackAnim = g_attackAnim;
    if (g_swinging) {
        if (++g_swingTime >= SWING_DURATION) { g_swingTime = 0; g_swinging = false; }
    } else {
        g_swingTime = 0;
    }
    g_attackAnim = (float)g_swingTime / SWING_DURATION;

    s_oEquipHeight = s_equipHeight;

    short id = 0;
    unsigned char data = 0;

    ItemInstance* held = g_level.player->inventory->getSelected();
    if (held) {
        id   = held->id;
        data = (unsigned char)held->data;
    }

    static int s_equippedSlot = -999;
    int slot = g_level.player->inventory->selected;

    Item* itemCls = (id > 0 && id < 4096) ? Item::items[id] : 0;
    const bool dataIsDurability = itemCls && itemCls->maxDamage > 0;
    const bool sameData = dataIsDurability || (data == s_equippedData);

    bool matches = (id == s_equippedId) && sameData && (slot == s_equippedSlot || id == 0);
    float tHeight = matches ? 1.0f : 0.0f;
    float dd = tHeight - s_equipHeight;
    float max_dd = 0.4f;
    if (dd < -max_dd) dd = -max_dd;
    if (dd > max_dd) dd = max_dd;
    s_equipHeight += dd;

    if (matches) {
        s_equippedData = data;
    } else if (s_equipHeight < 0.1f) {
        s_equippedSlot = slot;
        s_equippedId = id;
        s_equippedData = data;
    }
}

static float getAttackAnim(float a) {
    float diff = g_attackAnim - g_oAttackAnim;
    if (diff < 0.0f) diff += 1.0f;
    return g_oAttackAnim + diff * a;
}

static bool isFlat2DItem(short id) {
    if (id >= 256) return true;
    if (id <= 0)   return true;
    return !tileCanRenderAsBlock(Tile::tiles[id]->shape);
}

static Texture g_charTex;
static bool    g_haveChar = false;

void loadCharIfNeeded(void) {
    if (g_haveChar) return;
    if (!textureLoad("data/images/mob/skin.png", &g_charTex)) {
        g_haveChar = textureLoad("data/images/mob/char.png", &g_charTex);
    } else {
        g_haveChar = true;
    }
}

int itemBuildBlockMesh(short id, unsigned char data, ChunkVertex* out) {
    if (id == BLOCK_AIR) return 0;
    if (isCrossShaped(id))
        return emitCross(out, 0, 0, 150, 0, id, data, 0xFFFFFFFFu, false);
    if (isSlab(id))
        return emitSlab(&g_world, 0, 150, 0, id, data, out, 0);
    if (isStairs(id))
        return emitStairs(&g_world, 0, 150, 0, id, 3 , out, 0);
    if (isPane(id))
        return emitPane(&g_world, 0, 150, 0, id, data, out, 0);
    if (isFence(id)) {
        int n = 0;

        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           6.0f/16.0f, 0.0f, 0.0f,  10.0f/16.0f, 1.0f, 4.0f/16.0f, 0, 0, out, n);

        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           6.0f/16.0f, 0.0f, 12.0f/16.0f, 10.0f/16.0f, 1.0f, 1.0f, 0, 0, out, n);

        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           7.0f/16.0f, 13.0f/16.0f, -2.0f/16.0f, 9.0f/16.0f, 15.0f/16.0f, 18.0f/16.0f, 0, 0, out, n);

        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           7.0f/16.0f, 5.0f/16.0f, -2.0f/16.0f, 9.0f/16.0f, 7.0f/16.0f, 18.0f/16.0f, 0, 0, out, n);
        return n;
    }
    if (isWall(id)) {

        const float POST_W = 4.0f/16.0f, WALL_W = 3.0f/16.0f, WALL_H = 13.0f/16.0f;
        const float P0 = 0.5f - POST_W, P1 = 0.5f + POST_W;
        const float A0 = 0.5f - WALL_W, A1 = 0.5f + WALL_W;
        int n = 0;
        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           P0, 0.0f, 0.75f, P1, 1.0f,   1.25f, 0, 0, out, n);
        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           A0, 0.0f, 0.5f,  A1, WALL_H, 0.75f, 0, 0, out, n);
        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           A0, 0.0f, 0.25f, A1, WALL_H, 0.5f,  0, 0, out, n);
        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           P0, 0.0f, -0.25f, P1, 1.0f,  0.25f, 0, 0, out, n);
        return n;
    }
    if (isTrapdoor(id))
        return emitPartialBox(&g_world, 0, 150, 0, id, data,
                              0.0f, (8.0f - 1.5f)/16.0f, 0.0f, 1.0f, (8.0f + 1.5f)/16.0f, 1.0f, 0, 0, out, 0);
    if (isFenceGate(id)) {

        const float w = 1.0f / 16.0f;
        int n = 0;
        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           0.5f - w, 0.3f, 0.0f,         0.5f + w, 1.0f,     w * 2.0f, 0, 0, out, n);
        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           0.5f - w, 0.3f, 1.0f - w * 2, 0.5f + w, 1.0f,     1.0f,     0, 0, out, n);
        n = emitPartialBox(&g_world, 0, 150, 0, id, data,
                           0.5f - w, 0.5f, 0.0f,         0.5f + w, 1.0f - w, 1.0f,     0, 0, out, n);
        return n;
    }

    if (id == BLOCK_CHEST) return chestBuildHeldMesh(out);

    float ix = (id == BLOCK_CACTUS) ? 0.0625f : 0.0f;
    float iz = (id == BLOCK_CACTUS) ? 0.0625f : 0.0f;

    float th = (id == BLOCK_TOPSNOW)  ? 0.125f
             : (isCarpet(id))         ? 0.0625f
             : (id == BLOCK_FARMLAND) ? 0.9375f
             : 1.0f;
    return emitPartialBox(&g_world, 0, 150, 0, id, data,
                          ix, 0.0f, iz, 1.0f - ix, th, 1.0f - iz,
                          0, 0, out, 0);
}

static bool isTransparent(const Texture* tex, int tx, int ty) {
    if (!tex || !tex->data) return true;
    if (tx < 0 || tx >= tex->texW || ty < 0 || ty >= tex->texH) return true;

    if (tex->psm != GU_PSM_8888) {
        unsigned short px = ((const unsigned short*)tex->data)[ty * tex->texW + tx];
        if (tex->psm == GU_PSM_4444) return (px >> 12) < 8;
        if (tex->psm == GU_PSM_5551) return (px & 0x8000u) == 0;
        return false;
    }

    const unsigned int* pixels = (const unsigned int*)tex->data;
    if (tex->swizzled) {
        int wBlocks = tex->texW / 4;
        int blockIdx = (ty / 8) * wBlocks + (tx / 4);
        return (pixels[blockIdx * 32 + (ty % 8) * 4 + (tx % 4)] >> 24) < 128;
    }
    return (pixels[ty * tex->texW + tx] >> 24) < 128;
}

int bowStageIcon(float ticks) {
    const float MAX = 20.0f;
    if (ticks >= MAX - 2.0f)          return II_BOW_PULL_2;
    if (ticks > (2.0f * MAX) / 3.0f)  return II_BOW_PULL_1;
    if (ticks > 0.0f)                 return II_BOW_PULL_0;
    return -1;
}

int itemBuildFlatMesh(short id, unsigned char data, ChunkVertex* out, int bowStage, int cap) {
    if (id == BLOCK_AIR) return 0;

    float tex_w, tex_h;
    float sx, sy;
    unsigned int tileTint = 0xFFFFFFFFu;

    int animX, animY;
    const Texture* animTex = itemAnimIcon(id, bowStage >= 0 ? bowStage : 0, &animX, &animY);
    if (animTex) {
        sx = (float)animX; sy = (float)animY;
        tex_w = (float)animTex->texW; tex_h = (float)animTex->texH;
    } else if (id >= 256) {

        int icon = itemFlatIcon(id, data);
        if (bowStage >= 0) icon = bowStage;
        if (icon < 0) return 0;
        sx = (icon & 31) * 16.0f;
        sy = (27 + (icon >> 5)) * 16.0f;
        tex_w = 512.0f; tex_h = 512.0f;
    } else {
        int i = getGuiBlockIcon(id, data);
        if (i >= 128) {
            int iconIdx = i - 128;
            sx = (iconIdx & 31) * 16.0f;
            sy = (27 + (iconIdx >> 5)) * 16.0f;
            tex_w = 512.0f; tex_h = 512.0f;
        } else {
            int col, row; unsigned int t;
            tileForBlock(id, data, 0, &col, &row, &t);
            sx = col * 16.0f;
            sy = row * 16.0f;
            tex_w = 256.0f; tex_h = 256.0f;

            tileTint = t;
        }
    }

    const float z0 = 0.0f, z1 = -1.0f/16.0f;
    const float T  = 1.0f/16.0f;

    const unsigned int colFB = eggMul(0xFFFFFFFFu, tileTint);
    const unsigned int colLR = eggMul(0xFFCCCCCCu, tileTint);
    const unsigned int colTB = eggMul(0xFF999999u, tileTint);

    const Texture* texPtr = animTex ? animTex : itemFlatTexture(id, data);
    const int basex = (int)sx, basey = (int)sy;

    const bool egg = (id == ITEM_SPAWN_EGG);
    unsigned int eggBase = 0xFFFFFFFFu, eggSpot = 0xFFFFFFFFu;
    int ovx0 = 0, ovy0 = 0;
    if (egg) {
        spawnEggColors(data, &eggBase, &eggSpot);
        ovx0 = (II_SPAWN_EGG_OVERLAY & 31) * 16;
        ovy0 = (27 + (II_SPAWN_EGG_OVERLAY >> 5)) * 16;
    }

    int n = 0;
    #define FACE(c, ax,ay,az, bx,by,bz, cx,cy,cz, dx,dy,dz)   \
        do { if (n + 6 > cap) return n;                        \
             out[n++] = {uu, vv, c, ax, ay, az};               \
             out[n++] = {uu, vv, c, bx, by, bz};               \
             out[n++] = {uu, vv, c, cx, cy, cz};               \
             out[n++] = {uu, vv, c, cx, cy, cz};               \
             out[n++] = {uu, vv, c, dx, dy, dz};               \
             out[n++] = {uu, vv, c, ax, ay, az}; } while (0)

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {

            const int tx = basex + 15 - x;
            const int ty = basey + 15 - y;
            if (isTransparent(texPtr, tx, ty)) continue;

            const float uu = (tx + 0.5f) / tex_w;
            const float vv = (ty + 0.5f) / tex_h;

            const float x0 = x * T,       x1 = x0 + T;
            const float y0 = y * T,       y1 = y0 + T;

            unsigned int cFB = colFB, cLR = colLR, cTB = colTB;
            if (egg) {
                unsigned int ec =
                    !isTransparent(texPtr, ovx0 + (tx - basex), ovy0 + (ty - basey))
                        ? eggSpot : eggBase;
                cFB = eggMul(colFB, ec); cLR = eggMul(colLR, ec); cTB = eggMul(colTB, ec);
            }

            FACE(cFB, x0,y0,z0,  x1,y0,z0,  x1,y1,z0,  x0,y1,z0);
            FACE(cFB, x0,y1,z1,  x1,y1,z1,  x1,y0,z1,  x0,y0,z1);

            if (isTransparent(texPtr, tx + 1, ty))
                FACE(cLR, x0,y0,z1,  x0,y0,z0,  x0,y1,z0,  x0,y1,z1);
            if (isTransparent(texPtr, tx - 1, ty))
                FACE(cLR, x1,y1,z1,  x1,y1,z0,  x1,y0,z0,  x1,y0,z1);
            if (isTransparent(texPtr, tx, ty + 1))
                FACE(cTB, x1,y0,z0,  x0,y0,z0,  x0,y0,z1,  x1,y0,z1);
            if (isTransparent(texPtr, tx, ty - 1))
                FACE(cTB, x0,y1,z0,  x1,y1,z0,  x1,y1,z1,  x0,y1,z1);
        }
    }
    #undef FACE

    return n;
}

const Texture* itemFlatTexture(short id, unsigned char data) {
    int ax, ay;
    const Texture* anim = itemAnimIcon(id, 0, &ax, &ay);
    if (anim) return anim;
    if (id >= 256) return g_haveGuiBlocks ? &g_guiBlocks : 0;
    int i = getGuiBlockIcon(id, data);
    if (i >= 128) return g_haveGuiBlocks ? &g_guiBlocks : 0;
    return g_haveTerrain ? &g_terrain : 0;
}

const Texture* itemFlatIconUV(short id, unsigned char data,
                            float* u0, float* v0, float* u1, float* v1,
                            unsigned int* tint) {
    if (tint) *tint = 0xFFFFFFFFu;

    {
        int ax, ay;
        const Texture* anim = itemAnimIcon(id, 0, &ax, &ay);
        if (anim) {
            *u0 = (ax + 0.5f)  / (float)anim->texW;
            *v0 = (ay + 0.5f)  / (float)anim->texH;
            *u1 = (ax + 15.5f) / (float)anim->texW;
            *v1 = (ay + 15.5f) / (float)anim->texH;
            return anim;
        }
    }
    if (id >= 256) {
        if (!g_haveGuiBlocks) return 0;
        int icon = itemFlatIcon(id, data);
        if (icon < 0) return 0;
        float sx = (icon & 31) * 16.0f, sy = (27 + (icon >> 5)) * 16.0f;

        *u0 = (sx + 0.5f) / 512.0f; *v0 = (sy + 0.5f) / 512.0f;
        *u1 = (sx + 15.5f) / 512.0f; *v1 = (sy + 15.5f) / 512.0f;
        return &g_guiBlocks;
    }
    int i = getGuiBlockIcon(id, data);
    if (i >= 128) {
        if (!g_haveGuiBlocks) return 0;
        int iconIdx = i - 128;
        float sx = (iconIdx & 31) * 16.0f, sy = (27 + (iconIdx >> 5)) * 16.0f;
        *u0 = (sx + 0.5f) / 512.0f; *v0 = (sy + 0.5f) / 512.0f;
        *u1 = (sx + 15.5f) / 512.0f; *v1 = (sy + 15.5f) / 512.0f;
        return &g_guiBlocks;
    }
    if (!g_haveTerrain) return 0;

    int col, row; unsigned int t;
    tileForBlock(id, data, 0, &col, &row, &t);
    if (tint) *tint = t;
    *u0 = col * TILE_UV; *v0 = row * TILE_UV;
    *u1 = (col * 16.0f + 15.5f) / 256.0f; *v1 = (row * 16.0f + 15.5f) / 256.0f;
    return &g_terrain;
}

bool itemIsFlat2D(short id) { return isFlat2DItem(id); }

static MobVertex s_armMeshBase[36];
static bool      s_armBuilt = false;

static void buildArm(void) {
    if (s_armBuilt) return;
    const float W = 64.0f, H = 32.0f;

    int n = 0;
    auto addPoly = [&](float x0, float y0, float z0,
                       float x1, float y1, float z1,
                       float x2, float y2, float z2,
                       float x3, float y3, float z3,
                       float u0, float v0, float u1, float v1) {
        s_armMeshBase[n++] = {u0, v0, x0, y0, z0};
        s_armMeshBase[n++] = {u1, v0, x1, y1, z1};
        s_armMeshBase[n++] = {u1, v1, x2, y2, z2};
        s_armMeshBase[n++] = {u1, v1, x2, y2, z2};
        s_armMeshBase[n++] = {u0, v1, x3, y3, z3};
        s_armMeshBase[n++] = {u0, v0, x0, y0, z0};
    };

    float vx0 = -3.0f/16.0f, vx1 = 1.0f/16.0f;
    float vy0 = -2.0f/16.0f, vy1 = 10.0f/16.0f;
    float vz0 = -2.0f/16.0f, vz1 =  2.0f/16.0f;

    addPoly(vx1, vy0, vz1,  vx0, vy0, vz1,  vx0, vy0, vz0,  vx1, vy0, vz0,  48/W, 16/H, 44/W, 20/H);

    addPoly(vx0, vy0, vz0,  vx0, vy0, vz1,  vx0, vy1, vz1,  vx0, vy1, vz0,  44/W, 20/H, 40/W, 32/H);

    addPoly(vx1, vy0, vz1,  vx1, vy0, vz0,  vx1, vy1, vz0,  vx1, vy1, vz1,  52/W, 20/H, 48/W, 32/H);

    addPoly(vx1, vy1, vz0,  vx0, vy1, vz0,  vx0, vy1, vz1,  vx1, vy1, vz1,  48/W, 20/H, 52/W, 16/H);

    addPoly(vx1, vy0, vz0,  vx0, vy0, vz0,  vx0, vy1, vz0,  vx1, vy1, vz0,  48/W, 20/H, 44/W, 32/H);

    addPoly(vx0, vy0, vz1,  vx1, vy0, vz1,  vx1, vy1, vz1,  vx0, vy1, vz1,  56/W, 20/H, 52/W, 32/H);

    dcacheFlush(s_armMeshBase, sizeof(s_armMeshBase));
    s_armBuilt = true;
}

void itemHandDraw(float a, float bs, float bc) {

    int id = s_equippedId;
    int data = s_equippedData;

    bool hasItem = (id != BLOCK_AIR);
    bool isFlat  = hasItem && isFlat2DItem(id);

    int px = (int)g_level.player->x, py = (int)g_level.player->y, pz = (int)g_level.player->z;
    if (g_level.player->x < 0 && px != g_level.player->x) px--;
    if (g_level.player->y < 0 && py != g_level.player->y) py--;
    if (g_level.player->z < 0 && pz != g_level.player->z) pz--;
    int rawBr = lightRawAt(&g_world, px, py, pz);
    unsigned int brCol = g_brightColor[rawBr];

    static ItemModelRenderer s_model;
    if (hasItem) {
        int bowStage = (id == ITEM_BOW && g_level.player->bowPull > 0.0f)
                       ? bowStageIcon(g_level.player->bowTimeHeld)
                       : itemAnimStage(id, g_level.player);

        if (!s_model.build((short)id, (unsigned char)data, bowStage)) {
            hasItem = false;
            isFlat  = false;
        }
    }
    if (!hasItem) {
        loadCharIfNeeded();
        buildArm();
    }

    guPerspective(70.0f, 0.02f, 4.0f);
    sceGuClearDepth(0);
    sceGuClear(GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_DEPTH_TEST);

    sceGuDisable(GU_BLEND);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);

    if (hasItem && !isFlat && !isCrossShaped(id)) {
        sceGuEnable(GU_CULL_FACE);
        sceGuFrontFace(GU_CCW);
    } else {
        sceGuDisable(GU_CULL_FACE);
    }

    float swing     = getAttackAnim(a);
    if (hasItem && id == ITEM_BOW && g_level.player->bowPull > 0.0f) {
        swing = 0.0f;
    }

    const bool eating = hasItem && g_level.player->eatAnim > 0.0f &&
                        Item::items[id] && Item::items[id]->isFood();
    if (eating) swing = 0.0f;
    const float sqrtSwing = sqrtf(swing);
    const float swing1    = sinf(swing * PIF);
    const float swing2    = sinf(sqrtSwing * PIF);
    const float swing3    = sinf(swing * swing * PIF);

    const float d = 0.8f;
    const float h = s_oEquipHeight + (s_equipHeight - s_oEquipHeight) * a;

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    if (hasItem && id == ITEM_BOW) {
        ScePspFVector3 back = { 0.0f, 0.0f, -0.1f };
        sceGumTranslate(&back);
    }

    if (g_viewBobbing) {
        float wda = g_level.player->walkDist - g_level.player->walkDistO;
        float b = -(g_level.player->walkDist + wda * a);
        float bobv  = g_level.player->oBob  + (g_level.player->bob  - g_level.player->oBob)  * a;
        float tiltv = g_level.player->oTilt + (g_level.player->tilt - g_level.player->oTilt) * a;
        const float sinB = sinf(b * PIF);
        ScePspFVector3 bob = { sinB * bobv * 0.5f, -fabsf(cosf(b * PIF) * bobv), 0.0f };
        sceGumTranslate(&bob);
        sceGumRotateZ(sinB * bobv * 3.0f * DEG2RAD);
        sceGumRotateX(fabsf(cosf(b * PIF - 0.2f) * bobv) * 5.0f * DEG2RAD);

        sceGumRotateX(tiltv * DEG2RAD);
    }

    {
        const Player* p = g_level.player;
        float xr  = p->xRotO + (p->xRot - p->xRotO) * a;
        float yr  = p->yRotO + (p->yRot - p->yRotO) * a;
        float xrr = p->xBobO + (p->xBob - p->xBobO) * a;
        float yrr = p->yBobO + (p->yBob - p->yBobO) * a;
        sceGumRotateX((xr - xrr) * 0.1f * DEG2RAD);
        sceGumRotateY((yr - yrr) * 0.1f * DEG2RAD);
    }

    if (hasItem) {

        if (eating) {

            float progress = g_level.player->eatAnim;
            float t   = (1.0f - progress) * 32.0f;
            float is  = 1.0f - progress; is = is*is*is; is = is*is*is; is = is*is*is;
            float iss = 1.0f - is;
            ScePspFVector3 e0 = { 0.0f, fabsf(cosf(t / 4.0f * PIF) * 0.1f) * (progress > 0.2f ? 1.0f : 0.0f), 0.0f };
            sceGumTranslate(&e0);
            ScePspFVector3 e1 = { iss * 0.6f, -iss * 0.5f, 0.0f };
            sceGumTranslate(&e1);
            sceGumRotateY(iss * 90.0f * DEG2RAD);
            sceGumRotateX(iss * 10.0f * DEG2RAD);
            sceGumRotateZ(iss * 30.0f * DEG2RAD);
        } else {

            ScePspFVector3 sw = { -swing2 * 0.4f,
                                   sinf(sqrtSwing * PIF * 2.0f) * 0.2f,
                                  -swing1 * 0.2f };
            sceGumTranslate(&sw);
        }

        ScePspFVector3 base = { 0.7f * d, -0.65f * d - (1.0f - h) * 0.6f, -0.9f * d };
        sceGumTranslate(&base);

        sceGumRotateY(45.0f * DEG2RAD);
        sceGumRotateY(-swing3 * 20.0f * DEG2RAD);
        sceGumRotateZ(-swing2 * 20.0f * DEG2RAD);
        sceGumRotateX(-swing2 * 80.0f * DEG2RAD);

        ScePspFVector3 sc = { 0.4f, 0.4f, 0.4f };
        sceGumScale(&sc);

        if (id == ITEM_BOW && g_level.player->bowPull > 0.0f) {
            float pow = g_level.player->bowPull;
            float timeHeld = g_level.player->bowTimeHeld;
            sceGumRotateZ(-18.0f * DEG2RAD);
            sceGumRotateY(-12.0f * DEG2RAD);
            sceGumRotateX( -8.0f * DEG2RAD);
            ScePspFVector3 b1 = { -0.9f, 0.2f, 0.0f };
            sceGumTranslate(&b1);
            if (pow > 0.1f) {
                ScePspFVector3 jit = { 0.0f, sinf((timeHeld - 0.1f) * 1.3f) * 0.01f * (pow - 0.1f), 0.0f };
                sceGumTranslate(&jit);
            }
            ScePspFVector3 b2 = { 0.0f, 0.0f, pow * 0.1f };
            sceGumTranslate(&b2);
            sceGumRotateZ(-335.0f * DEG2RAD);
            sceGumRotateY( -50.0f * DEG2RAD);
            ScePspFVector3 b3 = { 0.0f, 0.5f, 0.0f };
            sceGumTranslate(&b3);
            ScePspFVector3 ys = { 1.0f, 1.0f, 1.0f + pow * 0.2f };
            sceGumScale(&ys);
            ScePspFVector3 b4 = { 0.0f, -0.5f, 0.0f };
            sceGumTranslate(&b4);
            sceGumRotateY(  50.0f * DEG2RAD);
            sceGumRotateZ( 335.0f * DEG2RAD);
        }

        if (!isFlat) {

            ScePspFVector3 center = { -0.5f, -150.5f, -0.5f };
            sceGumTranslate(&center);

            s_model.draw(brCol, true, true);
        } else {

            ItemModelRenderer::applyFlatPreTransform();

            sceGuEnable(GU_CULL_FACE);
            sceGuFrontFace(GU_CCW);
            s_model.draw(brCol, true, true);
        }
    } else {

        ScePspFVector3 sw = { -swing2 * 0.3f,
                               sinf(sqrtSwing * PIF * 2.0f) * 0.4f,
                              -swing1 * 0.4f };
        sceGumTranslate(&sw);

        ScePspFVector3 base = { 0.8f * d, -0.75f * d - (1.0f - h) * 0.6f, -0.9f * d };
        sceGumTranslate(&base);

        sceGumRotateY(45.0f * DEG2RAD);
        sceGumRotateY(swing2 * 70.0f * DEG2RAD);
        sceGumRotateZ(-swing3 * 20.0f * DEG2RAD);

        ScePspFVector3 fistPos = { -1.0f, 3.6f, 3.5f };
        sceGumTranslate(&fistPos);
        sceGumRotateZ(120.0f * DEG2RAD);
        sceGumRotateX(200.0f * DEG2RAD);
        sceGumRotateY(-135.0f * DEG2RAD);
        float armScale = 1.5f / 24.0f * 16.0f;
        ScePspFVector3 scArm = { armScale, armScale, armScale };
        sceGumScale(&scArm);
        ScePspFVector3 armOff = { 5.6f, 0.0f, 0.0f };
        sceGumTranslate(&armOff);

        ScePspFVector3 armPivot = { -5.0f/16.0f, 2.0f/16.0f, 0.0f };
        sceGumTranslate(&armPivot);

        if (g_haveChar) {
            textureBind(&g_charTex);
            sceGuColor(brCol);
            sceGumDrawArray(GU_TRIANGLES,
                        GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                        36, 0, s_armMeshBase);
            sceGuColor(0xFFFFFFFFu);
        }

    }

    sceGuEnable(GU_CULL_FACE);
}
