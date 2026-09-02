
#include "client/renderer/tileentity/tile_entity_renderer.h"
#include "client/renderer/entity/mob_model.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"

extern World g_world;
#include "world/level/tile/entity/sign_tile_entity.h"
#include "world/level/tile/entity/chest_tile_entity.h"
#include "gpu/texture.h"
#include "gpu/gu.h"
#include "gpu/font.h"
#include "platform/path.h"
#include "client/player/player_state.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

struct PVert { float u, v; unsigned int color; float x, y, z; };

static Texture s_signTex; static bool s_signLoaded = false, s_signOk = false;
static Font    s_font;    static bool s_fontLoaded = false, s_fontOk = false;

static void ensureAssets() {
    if (!s_signLoaded) {
        s_signLoaded = true;
        s_signOk = textureLoad(assetPath("data/images/item/sign.png"), &s_signTex)
                || textureLoad("data/images/item/sign.png", &s_signTex);
    }
    if (!s_fontLoaded) {
        s_fontLoaded = true;
        s_fontOk = fontLoad(assetPath("data/images/font/default8.png"), &s_font)
                || fontLoad("data/images/font/default8.png", &s_font);
    }
}

Texture* signBoardTexture() {
    ensureAssets();
    return s_signOk ? &s_signTex : 0;
}

static inline int quad(PVert* m, int n, unsigned int col,
                       float ax,float ay,float az,float au,float av,
                       float bx,float by,float bz,float bu,float bv,
                       float cx,float cy,float cz,float cu,float cv,
                       float dx,float dy,float dz,float du,float dv) {
    m[n++] = (PVert){au,av,col,ax,ay,az};
    m[n++] = (PVert){bu,bv,col,bx,by,bz};
    m[n++] = (PVert){cu,cv,col,cx,cy,cz};
    m[n++] = (PVert){au,av,col,ax,ay,az};
    m[n++] = (PVert){cu,cv,col,cx,cy,cz};
    m[n++] = (PVert){du,dv,col,dx,dy,dz};
    return n;
}

static int box(PVert* m, int n, const unsigned int* col6,
               float x, float y, float z, float w, float h, float d,
               float tu, float tv) {
    const float TW = 64.0f, TH = 32.0f;
    float x0=x, x1=x+w, y0=y, y1=y+h, z0=z, z1=z+d;
    #define U(a) ((tu+(a))/TW)
    #define V(a) ((tv+(a))/TH)

    n = quad(m,n,col6[0], x0,y0,z0,U(d),V(d+h),  x1,y0,z0,U(d+w),V(d+h),  x1,y1,z0,U(d+w),V(d),  x0,y1,z0,U(d),V(d));

    n = quad(m,n,col6[1], x1,y0,z1,U(d+w+d),V(d+h), x0,y0,z1,U(d+w+d+w),V(d+h), x0,y1,z1,U(d+w+d+w),V(d), x1,y1,z1,U(d+w+d),V(d));

    n = quad(m,n,col6[2], x0,y0,z1,U(0),V(d+h), x0,y0,z0,U(d),V(d+h), x0,y1,z0,U(d),V(d), x0,y1,z1,U(0),V(d));

    n = quad(m,n,col6[3], x1,y0,z0,U(d+w),V(d+h), x1,y0,z1,U(d+w+d),V(d+h), x1,y1,z1,U(d+w+d),V(d), x1,y1,z0,U(d+w),V(d));

    n = quad(m,n,col6[4], x0,y0,z1,U(d),V(d), x1,y0,z1,U(d+w),V(d), x1,y0,z0,U(d+w),V(0), x0,y0,z0,U(d),V(0));

    n = quad(m,n,col6[5], x0,y1,z0,U(d+w),V(d), x1,y1,z0,U(d+w+w),V(d), x1,y1,z1,U(d+w+w),V(0), x0,y1,z1,U(d+w),V(0));
    #undef U
    #undef V
    return n;
}

static PVert s_model[144];

static PVert s_text[512];

unsigned int tileEntityLightColor(Level* level, int x, int y, int z) {
    int raw = level ? level->getRawBrightness(x, y, z) : 15;
    if (raw < 0)  raw = 0;
    if (raw > 15) raw = 15;
    float b = g_brightRamp[raw];
    if (b > 1.0f) b = 1.0f;
    else if (b <= 0.4f) b = 0.4f;
    unsigned int c = (unsigned int)(b * 255.0f + 0.5f);
    return 0xFF000000u | (c << 16) | (c << 8) | c;
}

static void renderSign(SignTileEntity* sign, float a) {
    ensureAssets();
    if (!s_signOk) return;

    int data = sign->getData();
    bool standing = (sign->getTile() == BLOCK_SIGN);

    unsigned int col = tileEntityLightColor(sign->level, sign->x, sign->y, sign->z);

    int n = 0;
    const float size = 16.0f / 24.0f;
    float rot;
    if (standing) {
        rot = data * 360.0f / 16.0f;
    } else {

        rot = (data == 2) ? 180.0f : (data == 3) ? 0.0f : (data == 4) ? 90.0f : -90.0f;
    }

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    ScePspFVector3 t = { sign->x + 0.5f - g_relBaseX, sign->y + 0.75f * size - g_relBaseY, sign->z + 0.5f - g_relBaseZ };
    sceGumTranslate(&t);
    sceGumRotateY(-rot * 3.14159265f / 180.0f);
    if (!standing) {
        ScePspFVector3 off = { 0.0f, -5.0f/16.0f, -7.0f/16.0f };
        sceGumTranslate(&off);
    }

    sceGumPushMatrix();
    ScePspFVector3 ms = { size/16.0f, -size/16.0f, -size/16.0f };
    sceGumScale(&ms);
    sceGuDisable(GU_CULL_FACE);
    textureBind(&s_signTex);

    ScePspFMatrix4 lm;
    sceGumStoreMatrix(&lm);
    static const float FN[6][3] = {
        { 0,0,-1 }, { 0,0,1 }, { -1,0,0 }, { 1,0,0 }, { 0,-1,0 }, { 0,1,0 },
    };
    unsigned int col6[6];
    for (int f = 0; f < 6; f++) col6[f] = mobDirLitColor((const float*)&lm, FN[f], col);
    n = box(s_model, 0, col6, -12, -14, -1, 24, 12, 2, 0, 0);
    if (standing) n = box(s_model, n, col6, -1, -2, -1, 2, 14, 2, 0, 14);

    void* mv = guFrameCopy(s_model, n * sizeof(PVert));
    if (mv) sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    n, 0, mv);
    sceGumPopMatrix();

    if (s_fontOk) {
        const float s = (1.0f / 60.0f) * size;
        ScePspFVector3 tt = { 0.0f, 0.5f * size, 0.07f * size };
        sceGumTranslate(&tt);
        ScePspFVector3 ts = { s, -s, s };
        sceGumScale(&ts);

        int tn = 0;
        const int CELL = 8;
        const float FW = (float)s_font.tex.texW, FH = (float)s_font.tex.texH;
        float yy = (float)(SignTileEntity::NUM_LINES * -5);
        for (int li = 0; li < SignTileEntity::NUM_LINES; li++) {
            char buf[40];
            const std::string& msg = sign->messages[li];
            if (li == sign->selectedLine) snprintf(buf, sizeof(buf), "> %s <", msg.c_str());
            else { strncpy(buf, msg.c_str(), sizeof(buf)-1); buf[sizeof(buf)-1] = 0; }

            float cursorX = -fontTextWidth(&s_font, buf) / 2.0f;
            for (const unsigned char* p = (const unsigned char*)buf; *p; p++) {
                unsigned char ch = *p;
                int cx = (ch % 16) * CELL, cy = (ch / 16) * CELL;
                float u0 = cx / FW, v0 = cy / FH, u1 = (cx + CELL) / FW, v1 = (cy + CELL) / FH;
                float gx0 = cursorX, gx1 = cursorX + CELL, gy0 = yy, gy1 = yy + CELL;
                if (tn + 6 <= (int)(sizeof(s_text)/sizeof(s_text[0]))) {
                    tn = quad(s_text, tn, 0xFF000000u,
                              gx0,gy0,0, u0,v0,  gx1,gy0,0, u1,v0,
                              gx1,gy1,0, u1,v1,  gx0,gy1,0, u0,v1);
                }
                cursorX += s_font.charWidth[ch];
            }
            yy += 10.0f;
        }

        if (tn > 0) {
            sceGuDepthMask(GU_TRUE);
            textureBind(&s_font.tex);
            void* tv = guFrameCopy(s_text, tn * sizeof(PVert));
            if (tv) sceGumDrawArray(GU_TRIANGLES,
                            GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                            tn, 0, tv);
            sceGuDepthMask(GU_FALSE);
        }
    }

    sceGuEnable(GU_CULL_FACE);
    sceGumPopMatrix();
}

void renderAllTileEntities(Level* level, float a) {
    if (!level) return;

    const float CULL2 = 64.0f * 64.0f;
    std::vector<TileEntity*>& list = level->tileEntities;
    for (size_t i = 0; i < list.size(); i++) {
        TileEntity* te = list[i];
        if (!te || te->removed) continue;

        if (!level->isLoadedAt((float)te->x, (float)te->z)) continue;

        int tile = level->getTile(te->x, te->y, te->z);
        switch (te->rendererId) {
            case TR_SIGN_RENDERER:
                if (!isSign(tile)) { te->removed = true; continue; }
                break;
            case TR_CHEST_RENDERER:
                if (tile != BLOCK_CHEST) { te->removed = true; continue; }
                break;
            default: continue;
        }
        if (!worldColumnDrawn(&g_world, (float)te->x + 0.5f, (float)te->z + 0.5f)) continue;
        if (!g_level.player) continue;
        float dx = (te->x + 0.5f) - g_level.player->x, dy = (te->y + 0.5f) - g_level.player->y, dz = (te->z + 0.5f) - g_level.player->z;
        if (dx*dx + dy*dy + dz*dz > CULL2) continue;
        if (te->rendererId == TR_SIGN_RENDERER) renderSign((SignTileEntity*)te, a);
        else                                    renderChestTile((ChestTileEntity*)te, a);
    }

}
