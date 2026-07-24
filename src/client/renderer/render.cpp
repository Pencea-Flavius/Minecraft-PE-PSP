#include "client/renderer/render.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "client/player/player.h"
#include "client/gamemode/gamemode.h"
#include "client/renderer/level/mesh_worker.h"

#include "gpu/gu.h"
#include "gpu/texture.h"
#include "gpu/font.h"
#include "gpu/sprite.h"
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/level/mob_spawner.h"
#include "client/renderer/entity/entity_render_dispatcher.h"
#include "client/renderer/tileentity/tile_entity_renderer.h"
#include "world/level/chunk/chunk.h"
#include "platform/path.h"
#include "client/renderer/item_hand.h"
#include "client/renderer/entity/player_model.h"
#include "client/renderer/particle.h"
#include "world/level/storage/level_storage.h"
#include "world/entity/tripod_camera.h"
#include <cstring>

#include <stdio.h>
#include <pspctrl.h>
#include <cmath>
#include <cstdlib>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <psputils.h>

extern World g_world;
extern Level g_level;
extern bool  g_worldBuilt;
extern bool  g_saveShowProgress;

extern float g_timerAlpha;
extern int   g_viewBobbing;
#define PLAYER_EYE 1.62f

int   g_diagMode = 0;
int   g_noMipmap = 0;
int   g_showFps  = 0;
int   g_showCoords = 0;
float g_viewDist = WORLD_VIEW_DIST;

static const float DEG2RAD = 3.14159265f / 180.0f;

#include "client/renderer/water_anim.h"
#include "client/gui/gen_screen.h"
#include "client/gui/hud.h"
#include "client/gui/inventory_ui.h"

Texture g_terrain;
bool    g_haveTerrain = false;

static volatile bool g_loadedFromDisk = false;

static volatile int g_genPhase = 0;
Texture g_guiBlocks;
bool    g_haveGuiBlocks = false;

Texture g_clouds;
bool    g_haveClouds = false;
Texture g_particles;
bool    g_haveParticles = false;

extern int g_fancyGraphics;
extern int g_cloudTicks;

static bool loadTexMip(Texture* out, int level, const char* rel) {
    return textureLoadMipLevel(out, level, assetPath(rel)) || textureLoadMipLevel(out, level, rel);
}

static bool loadTex(Texture* out, const char* rel) {
    return textureLoad(assetPath(rel), out) || textureLoad(rel, out);
}

static bool loadTex4444(Texture* out, const char* rel) {
    return textureLoad4444(assetPath(rel), out) || textureLoad4444(rel, out);
}

static bool loadTex16(Texture* out, const char* rel, int psm) {
    return textureLoad16(assetPath(rel), out, psm) || textureLoad16(rel, out, psm);
}

float g_camX = 0.0f, g_camY = 0.0f, g_camZ = 0.0f;

enum WorldGenStage { GS_IDLE, GS_SHOW, GS_TERRAIN, GS_TERRAIN_WAIT, GS_MESHING };
static WorldGenStage g_genStage = GS_IDLE;
#define MESH_CHUNKS_PER_FRAME 8

namespace {
struct CloudVertex {
    float u, v;
    unsigned int color;
    float x, y, z;
};
struct ColorVertex {
    unsigned int color;
    float x, y, z;
};
}

#define SKY_DOME_COLOR 0xFFBF5424u
#define SKY_DOME_OFFSET 48.0f

unsigned int g_skyColorNow = SKY_COLOR;
static unsigned int g_skyDomeColorNow = SKY_DOME_COLOR;
static unsigned int g_cloudColorNow = 0xCCFFFFFFu;

static unsigned int scaleABGR(unsigned int c, float fr, float fg, float fb) {
    unsigned int r = (unsigned int)(( c        & 0xFF) * fr);
    unsigned int g = (unsigned int)(((c >> 8)  & 0xFF) * fg);
    unsigned int b = (unsigned int)(((c >> 16) & 0xFF) * fb);
    return (c & 0xFF000000u) | (b << 16) | (g << 8) | r;
}

static void updateDayColors(float alpha) {
    float td = worldTimeOfDay(g_world.dayTime, alpha);
    float c = cosf(td * 2.0f * 3.14159265f);

    float sb = c * 2.0f + 0.5f;
    if (sb < 0.0f) sb = 0.0f; if (sb > 0.75f) sb = 0.75f;
    sb /= 0.75f;

    float fb = c * 2.0f + 0.5f;
    if (fb < 0.0f) fb = 0.0f; if (fb > 1.0f) fb = 1.0f;
    g_skyDomeColorNow = scaleABGR(SKY_DOME_COLOR, sb, sb, sb);
    g_skyColorNow     = scaleABGR(SKY_COLOR, fb*0.94f+0.06f, fb*0.94f+0.06f, fb*0.91f+0.09f);
    g_cloudColorNow   = scaleABGR(0xCCFFFFFFu, fb*0.90f+0.10f, fb*0.90f+0.10f, fb*0.85f+0.15f);
}

static void renderSky(float px, float py, float pz) {
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_CULL_FACE);

    sceGuShadeModel(GU_SMOOTH);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDepthMask(GU_TRUE);

    const int s = 32, d = 6;
    ScePspFVector3 t = { px, py + SKY_DOME_OFFSET, pz };
    sceGumTranslate(&t);

    int cells = (2 * d) * (2 * d);
    ColorVertex* v = (ColorVertex*)sceGuGetMemory(cells * 6 * sizeof(ColorVertex));
    unsigned int dc = g_skyDomeColorNow;
    int n = 0;
    for (int xx = -s * d; xx < s * d; xx += s) {
        for (int zz = -s * d; zz < s * d; zz += s) {
            float wx0 = (float)xx, wx1 = (float)(xx + s);
            float wz0 = (float)zz, wz1 = (float)(zz + s);
            v[n].color=dc; v[n].x=wx0; v[n].y=0; v[n].z=wz1; n++;
            v[n].color=dc; v[n].x=wx1; v[n].y=0; v[n].z=wz1; n++;
            v[n].color=dc; v[n].x=wx1; v[n].y=0; v[n].z=wz0; n++;
            v[n].color=dc; v[n].x=wx0; v[n].y=0; v[n].z=wz1; n++;
            v[n].color=dc; v[n].x=wx1; v[n].y=0; v[n].z=wz0; n++;
            v[n].color=dc; v[n].x=wx0; v[n].y=0; v[n].z=wz0; n++;
        }
    }

    sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, n, 0, v);

    sceGuDepthMask(GU_FALSE);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuShadeModel(GU_FLAT);
    sceGuEnable(GU_TEXTURE_2D);
}

static void renderClouds(float alpha, float px, float py, float pz) {
    if (!g_haveClouds) return;

    textureBind(&g_clouds);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDepthMask(GU_TRUE);
    sceGuDisable(GU_CULL_FACE);
    sceGuShadeModel(GU_SMOOTH);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);

    const int s = 32;
    const int d = 256 / s;
    const float scale = 1.0f / 2048.0f;

    float time = (float)g_cloudTicks + alpha;
    float xo = px + time * 0.03f;
    float zo = pz;
    ScePspFVector3 t = { px, 128.0f + 0.33f, pz };
    sceGumTranslate(&t);

    unsigned int color = g_cloudColorNow;

    int cells = (2 * d) * (2 * d);
    CloudVertex* v = (CloudVertex*)sceGuGetMemory(cells * 6 * sizeof(CloudVertex));
    int n = 0;
    for (int xx = -s * d; xx < s * d; xx += s) {
        for (int zz = -s * d; zz < s * d; zz += s) {
            float u0 = (xx + xo) * scale,     v0 = (zz + zo) * scale;
            float u1 = (xx + s + xo) * scale, v1 = (zz + s + zo) * scale;
            float wx0 = (float)xx, wx1 = (float)(xx + s);
            float wz0 = (float)zz, wz1 = (float)(zz + s);

            v[n].u=u0; v[n].v=v1; v[n].color=color; v[n].x=wx0; v[n].y=0; v[n].z=wz1; n++;
            v[n].u=u1; v[n].v=v1; v[n].color=color; v[n].x=wx1; v[n].y=0; v[n].z=wz1; n++;
            v[n].u=u1; v[n].v=v0; v[n].color=color; v[n].x=wx1; v[n].y=0; v[n].z=wz0; n++;

            v[n].u=u0; v[n].v=v1; v[n].color=color; v[n].x=wx0; v[n].y=0; v[n].z=wz1; n++;
            v[n].u=u1; v[n].v=v0; v[n].color=color; v[n].x=wx1; v[n].y=0; v[n].z=wz0; n++;
            v[n].u=u0; v[n].v=v0; v[n].color=color; v[n].x=wx0; v[n].y=0; v[n].z=wz0; n++;
        }
    }

    sceGumDrawArray(GU_TRIANGLES,
                   GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                   n, 0, v);

    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuEnable(GU_CULL_FACE);
    sceGuDepthMask(GU_FALSE);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuShadeModel(GU_FLAT);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuDisable(GU_BLEND);
}

static void drawInWallOverlay(float ix, float iy, float iz) {
    if (!g_haveTerrain) return;
    int hx = (int)ix, hy = (int)iy, hz = (int)iz;
    unsigned char id = worldBlock(&g_world, hx, hy, hz);
    if (!isOpaque(id)) return;
    int col, row; unsigned int tint;
    tileForBlock(id, worldData(&g_world, hx, hy, hz), F_LEFT, &col, &row, &tint);

    const unsigned int color = 0xFF1A1A1Au;
    textureBind(&g_terrain);
    spriteDraw(&g_terrain, 0, 0, 480, 272, col * 16.0f, row * 16.0f, 16.0f, 16.0f, color);
}

static void fireScreenEffect() {
    if (!g_level.player || !g_level.player->isOnFire()) return;

    guPerspective(70.0f, 0.02f, 4.0f);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_FOG);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    textureBindNoMip(&g_terrain);
    struct V { float u, v; unsigned int c; float x, y, z; };
    const unsigned int col = 0xE6FFFFFFu;
    const float size = 1.0f, z0 = -0.5f;
    const float x0 = -size / 2, x1 = x0 + size, y0 = -size / 2, y1 = y0 + size;

    const float TILE = 1.0f / 16.0f, HT = TILE / 32.0f;
    for (int i = 0; i < 2; i++) {
        const float u0 = 15.0f / 16.0f + HT, u1 = 16.0f / 16.0f - HT;
        const float v0 = (1 + i) / 16.0f + HT, v1 = (2 + i) / 16.0f - HT;
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        ScePspFVector3 tr = { -(i * 2 - 1) * 0.24f, -0.3f, 0.0f };
        sceGumTranslate(&tr);
        sceGumRotateY((i * 2 - 1) * 10.0f * DEG2RAD);
        V* q = (V*)sceGuGetMemory(4 * sizeof(V));
        q[0] = { u1, v1, col, x0, y0, z0 };
        q[1] = { u0, v1, col, x1, y0, z0 };
        q[2] = { u0, v0, col, x1, y1, z0 };
        q[3] = { u1, v0, col, x0, y1, z0 };
        sceGumDrawArray(GU_TRIANGLE_FAN,
                        GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                        4, 0, q);
    }
}

static void loadWorldView(float ex, float ey, float ez,
                          float cx, float cy, float cz, float roll,
                          float bx, float by, float bz) {
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    if (roll != 0.0f) sceGumRotateZ(roll * DEG2RAD);
    ScePspFVector3 eye = { ex - bx, ey - by, ez - bz };
    ScePspFVector3 ctr = { cx - bx, cy - by, cz - bz };
    ScePspFVector3 up  = { 0.0f, 1.0f, 0.0f };
    sceGumLookAt(&eye, &ctr, &up);
}

static void renderMiningCrack(float ex, float ey, float ez) {
    if (!g_mining.active || g_mining.progress <= 0.0f || !g_haveTerrain) return;
    unsigned char id   = worldBlock(&g_world, g_mining.x, g_mining.y, g_mining.z);
    unsigned char data = worldData(&g_world, g_mining.x, g_mining.y, g_mining.z);
    if (id == BLOCK_AIR) return;

    int stage = (int)(g_mining.progress * 10.0f);
    if (stage < 0) stage = 0; else if (stage > 9) stage = 9;

    float boxes[3][6];
    int nb = worldSelectionBoxes(&g_world, g_mining.x, g_mining.y, g_mining.z, boxes);
    if (nb <= 0) return;

    const float EPS = 0.01f;
    const float T   = 1.0f / 16.0f;
    const float HT  = T / 32.0f;

    const float cu0 = stage * T + HT, cu1 = (stage + 1) * T - HT;
    const float cv0 = 15 * T + HT,    cv1 = 16 * T - HT;

    static const unsigned char FQ[6][4][3] = {
        {{0,1,0},{1,1,0},{1,1,1},{0,1,1}},
        {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
        {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},
        {{0,0,0},{1,0,0},{1,1,0},{0,1,0}},
        {{1,0,0},{1,0,1},{1,1,1},{1,1,0}},
        {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
    };
    static const int   TRI[6]    = {0,1,2,2,3,0};

    static const signed char FAXIS[6] = { 1, 1, 2, 2, 0, 0 };
    static const signed char FSIGN[6] = { 1,-1, 1,-1, 1,-1 };

    ChunkVertex* mesh = (ChunkVertex*)sceGuGetMemory(nb * 36 * sizeof(ChunkVertex));
    int total = 0;
    for (int b = 0; b < nb; b++) {
        float lo[3] = { boxes[b][0] - EPS, boxes[b][1] - EPS, boxes[b][2] - EPS };
        float hi[3] = { boxes[b][3] + EPS, boxes[b][4] + EPS, boxes[b][5] + EPS };
        const float eye[3] = { ex, ey, ez };
        for (int f = 0; f < 6; f++) {
            int ax = FAXIS[f];
            bool facing = FSIGN[f] > 0 ? (eye[ax] > hi[ax]) : (eye[ax] < lo[ax]);
            if (!facing) continue;
            for (int t = 0; t < 6; t++) {
                const unsigned char* c = FQ[f][TRI[t]];
                ChunkVertex& v = mesh[total++];
                v.color = 0xFFFFFFFFu;
                v.x = c[0] ? hi[0] : lo[0];
                v.y = c[1] ? hi[1] : lo[1];
                v.z = c[2] ? hi[2] : lo[2];

                float fx = v.x - g_mining.x, fy = v.y - g_mining.y, fz = v.z - g_mining.z;
                if (fx < 0) fx = 0; else if (fx > 1) fx = 1;
                if (fy < 0) fy = 0; else if (fy > 1) fy = 1;
                if (fz < 0) fz = 0; else if (fz > 1) fz = 1;
                float tu, tv;
                if (ax == 1)      { tu = fx; tv = fz; }
                else if (ax == 2) { tu = fx; tv = fy; }
                else              { tu = fz; tv = fy; }
                v.u = cu0 + tu * (cu1 - cu0);
                v.v = cv0 + tv * (cv1 - cv0);
            }
        }
    }
    if (total <= 0) return;

    textureBind(&g_terrain);

    if (g_terrain.mipCount && !g_noMipmap) sceGuTexLevelMode(GU_TEXTURE_CONST, 0.0f);
    sceGuEnable(GU_BLEND);

    sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_FIX, 0, 0);
    sceGuDisable(GU_CULL_FACE);
    sceGuDepthMask(GU_TRUE);

    sceGuDepthOffset(80);
    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    extern float g_relBaseX, g_relBaseY, g_relBaseZ;
    ScePspFVector3 tr = { -g_relBaseX, -g_relBaseY, -g_relBaseZ };
    sceGumTranslate(&tr);
    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    total, 0, mesh);
    sceGumPopMatrix();
    if (g_terrain.mipCount && !g_noMipmap) sceGuTexLevelMode(GU_TEXTURE_AUTO, 0.0f);
    sceGuDepthOffset(0);

    sceGuDepthMask(GU_FALSE);
    sceGuEnable(GU_CULL_FACE);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
}

int g_blockOutline = 1;
static void renderSelectionOutline(float ex, float ey, float ez) {
    (void)ex; (void)ey; (void)ez;
    if (!g_blockOutline) return;
    if (!g_worldBuilt || !g_level.player) return;

    BlockHit hit = worldPick(&g_world, g_level.player->x, g_level.player->y, g_level.player->z,
                             g_level.player->yRot, g_level.player->xRot, 5.0f);
    unsigned char id = worldBlock(&g_world, hit.x, hit.y, hit.z);
    if (id == BLOCK_INVISIBLE_BEDROCK) return;

    float boxes[3][6];
    int nb = worldSelectionBoxes(&g_world, hit.x, hit.y, hit.z, boxes);
    if (nb <= 0) return;

    (void)id;
    extern float g_relBaseX, g_relBaseY, g_relBaseZ;
    ScePspFVector3 tr = { -g_relBaseX, -g_relBaseY, -g_relBaseZ };

    {
        const float GROW = 0.002f;
        const unsigned int COL = 0xFF000000u;

        static const unsigned char EDGE[12][2] = {
            {0,1},{1,3},{3,2},{2,0},
            {4,5},{5,7},{7,6},{6,4},
            {0,4},{1,5},{2,6},{3,7},
        };

        struct FillVtx { unsigned int color; float x, y, z; };

        FillVtx* v = (FillVtx*)sceGuGetMemory((nb * 24 > 108 ? nb * 24 : 108) * sizeof(FillVtx));
        int n = 0;

        const float EPS = 1.0f / 512.0f;
        const float org[3] = { (float)hit.x, (float)hit.y, (float)hit.z };
        unsigned char occ = 0;
        bool snapped = true;

        for (int b = 0; b < nb && snapped; b++) {
            int h[6];
            for (int c = 0; c < 6; c++) {
                float local = (boxes[b][c] - org[c % 3]) * 2.0f;
                h[c] = (int)(local < 0.0f ? local - 0.5f : local + 0.5f);
                if (h[c] < 0 || h[c] > 2 || local - h[c] > EPS || h[c] - local > EPS) snapped = false;
            }
            if (!snapped) break;
            for (int oz = 0; oz < 2; oz++)
            for (int oy = 0; oy < 2; oy++)
            for (int ox = 0; ox < 2; ox++) {
                const int o[3] = { ox, oy, oz };
                bool inside = true;
                for (int a = 0; a < 3; a++)
                    if (o[a] < h[a] || o[a] + 1 > h[a + 3]) inside = false;
                if (inside) occ |= (unsigned char)(1 << (ox + 2 * oy + 4 * oz));
            }
        }

        if (snapped && occ) {

            static const bool kDrawEdge[16] = {
                false, true,  true,  false, true,  false, true,  true,
                true,  true,  false, true,  false, true,  true,  false
            };
            static const int PERP[3][2] = { {1,2}, {0,2}, {0,1} };

            for (int a = 0; a < 3; a++) {
                int p = PERP[a][0], q = PERP[a][1];
                for (int i = 0; i <= 2; i++)
                for (int j = 0; j <= 2; j++)
                for (int k = 0; k < 2; k++) {
                    int mask = 0;
                    for (int dv = 0; dv < 2; dv++)
                    for (int du = 0; du < 2; du++) {
                        int o[3];
                        o[a] = k; o[p] = i - 1 + du; o[q] = j - 1 + dv;
                        if (o[p] < 0 || o[p] > 1 || o[q] < 0 || o[q] > 1) continue;
                        if (occ & (1 << (o[0] + 2 * o[1] + 4 * o[2]))) mask |= 1 << (du + 2 * dv);
                    }
                    if (!kDrawEdge[mask]) continue;

                    for (int e = 0; e < 2; e++) {
                        int c[3];
                        c[a] = k + e; c[p] = i; c[q] = j;
                        FillVtx& fv = v[n++];
                        fv.color = COL;
                        float w3[3];
                        for (int ax = 0; ax < 3; ax++)
                            w3[ax] = org[ax] + (c[ax] == 0 ? -GROW : c[ax] == 2 ? 1.0f + GROW : 0.5f);
                        fv.x = w3[0]; fv.y = w3[1]; fv.z = w3[2];
                    }
                }
            }
        } else {
            for (int b = 0; b < nb; b++) {
                float lo[3] = { boxes[b][0] - GROW, boxes[b][1] - GROW, boxes[b][2] - GROW };
                float hi[3] = { boxes[b][3] + GROW, boxes[b][4] + GROW, boxes[b][5] + GROW };
                for (int e = 0; e < 12; e++)
                    for (int p = 0; p < 2; p++) {
                        int c = EDGE[e][p];
                        FillVtx& fv = v[n++];
                        fv.color = COL;
                        fv.x = (c & 1) ? hi[0] : lo[0];
                        fv.y = (c & 2) ? hi[1] : lo[1];
                        fv.z = (c & 4) ? hi[2] : lo[2];
                    }
            }
        }

        if (n > 0) {
            sceGuDisable(GU_TEXTURE_2D);
            sceGuDisable(GU_BLEND);
            sceGuDisable(GU_CULL_FACE);
            sceGuDepthMask(GU_TRUE);
            sceGuDepthOffset(80);
            sceGumMatrixMode(GU_MODEL);
            sceGumPushMatrix();
            sceGumLoadIdentity();
            sceGumTranslate(&tr);
            sceGumDrawArray(GU_LINES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, n, 0, v);
            sceGumPopMatrix();
            sceGuDepthOffset(0);
            sceGuDepthMask(GU_FALSE);
            sceGuEnable(GU_CULL_FACE);
        }
    }

    sceGuDepthMask(GU_FALSE);
    sceGuEnable(GU_CULL_FACE);
    sceGuEnable(GU_TEXTURE_2D);
}

bool gameProgressScreenUp() { return g_saveRequested || !g_worldBuilt; }

void gameRender(MenuState& s) {

    if (g_worldBuilt && g_saveRequested) {
        static int saveStage = 0;
        struct SaveArgs { World* w; long seed; int gamemode; char dir[320]; char name[64]; };
        static SaveArgs sArgs;
        static volatile bool g_saveThreadDone = false;
        if (saveStage == 0) {
            sArgs.w = &g_world;
            const char* actDir = LevelStorage::getActiveDir();
            if (actDir && actDir[0] != '\0') {
                strncpy(sArgs.dir, actDir, sizeof(sArgs.dir) - 1);
                sArgs.dir[sizeof(sArgs.dir) - 1] = '\0';
                sArgs.seed = LevelStorage::getActiveSeed();
                sArgs.gamemode = LevelStorage::getActiveGameType();
                strncpy(sArgs.name, LevelStorage::getActiveName(), sizeof(sArgs.name) - 1);
                sArgs.name[sizeof(sArgs.name) - 1] = '\0';
            } else {
                bool sel = (s.worldSelected >= 0 && s.worldSelected < s.worlds.count);
                sArgs.seed = sel ? s.worlds.seeds[s.worldSelected] : 0;
                sArgs.gamemode = sel ? s.worlds.gameModes[s.worldSelected] : 1;
                char rel[320];
                snprintf(rel, sizeof(rel), "saves/%s", sel ? s.worlds.names[s.worldSelected] : "world");
                strncpy(sArgs.dir, assetPath(rel), sizeof(sArgs.dir) - 1);
                sArgs.dir[sizeof(sArgs.dir) - 1] = '\0';
                strncpy(sArgs.name, sel ? s.worlds.displayNames[s.worldSelected] : "World", sizeof(sArgs.name) - 1);
                sArgs.name[sizeof(sArgs.name) - 1] = '\0';
            }
            g_terrainProgress = 0;
            g_saveThreadDone = false;
            int thid = sceKernelCreateThread("world_save", [](SceSize, void* argp) -> int {
                SaveArgs* a = (SaveArgs*)argp;
                LevelStorage::save(a->w, a->dir, a->seed, a->gamemode, a->name);
                g_saveThreadDone = true;
                return 0;
            }, 0x22, 0x10000, 0, 0);
            if (thid >= 0) sceKernelStartThread(thid, sizeof(SaveArgs), &sArgs);
            else { LevelStorage::save(sArgs.w, sArgs.dir, sArgs.seed, sArgs.gamemode, sArgs.name); g_saveThreadDone = true; }
            drawGeneratingScreen(s, 0, "Saving chunks");
            saveStage = 1;
            return;
        }
        drawGeneratingScreen(s, g_terrainProgress, "Saving chunks");
        if (g_saveThreadDone) {
            g_saveRequested = false; saveStage = 0;
            g_terrainProgress = 0;
            extern MiningState g_mining;
            g_mining.active = false; g_mining.progress = 0.0f;

            if (g_quitAfterSave) {
                g_quitAfterSave = false;
                quitToMenuNoSave(s);
                s.screen = SCREEN_TITLE;
            }
        }
        return;
    }
    if (!g_worldBuilt) {
        if (g_genStage == GS_IDLE) g_genStage = GS_SHOW;

        if (g_genStage == GS_SHOW) {

            drawGeneratingScreen(s, 0);
            g_genStage = GS_TERRAIN;
            return;
        }
        if (g_genStage == GS_TERRAIN) {

            bool sel = (s.worldSelected >= 0 && s.worldSelected < s.worlds.count);
            long seedVal = sel ? s.worlds.seeds[s.worldSelected] : 0;

            struct TerrainArgs { World* w; long seed; int gamemode; char dir[320]; char name[64]; };
            static TerrainArgs tArgs;
            tArgs.w = &g_world;
            tArgs.seed = seedVal;
            tArgs.gamemode = sel ? s.worlds.gameModes[s.worldSelected] : 1;
            char rel[320];
            snprintf(rel, sizeof(rel), "saves/%s", sel ? s.worlds.names[s.worldSelected] : "world");
            strncpy(tArgs.dir, assetPath(rel), sizeof(tArgs.dir) - 1);
            tArgs.dir[sizeof(tArgs.dir) - 1] = '\0';
            strncpy(tArgs.name, sel ? s.worlds.displayNames[s.worldSelected] : "World", sizeof(tArgs.name) - 1);
            tArgs.name[sizeof(tArgs.name) - 1] = '\0';
            LevelStorage::setActiveWorld(tArgs.dir, tArgs.seed, tArgs.gamemode, tArgs.name);
            worldListTouch(tArgs.dir);

            playerSpawnEnsure();
            g_loadedFromDisk = false;
            g_genPhase = 0;
            g_terrainProgress = 0;
            g_terrainThreadDone = false;
            int thid = sceKernelCreateThread("terrain_gen", [](SceSize args, void* argp) -> int {
                TerrainArgs* a = (TerrainArgs*)argp;
                if (LevelStorage::hasSave(a->dir)) {
                    long s2; int gt;
                    LevelStorage::load(a->w, a->dir, &s2, &gt);
                    g_loadedFromDisk = true;
                } else {
                    worldInitTerrain(a->w, a->seed);

                    { int sx, sz, feetY; worldFindSpawn(a->w, &sx, &sz, &feetY);
                      g_level.player->x = sx + 0.5f; g_level.player->z = sz + 0.5f;
                      g_level.player->y = feetY + PLAYER_EYE; }
                    g_genPhase = 1;
                    g_saveShowProgress = false;
                    LevelStorage::save(a->w, a->dir, a->seed, a->gamemode, a->name, true);
                    g_saveShowProgress = true;
                }
                g_terrainThreadDone = true;
                return 0;
            }, 0x22, 0x10000, 0, 0);

            if (thid >= 0) {
                sceKernelStartThread(thid, sizeof(TerrainArgs), &tArgs);
                g_genStage = GS_TERRAIN_WAIT;
            } else {

                if (LevelStorage::hasSave(tArgs.dir)) {
                    long s2; int gt;
                    LevelStorage::load(&g_world, tArgs.dir, &s2, &gt);
                    g_loadedFromDisk = true;
                } else {
                    worldInitTerrain(&g_world, seedVal);

                    { int sx, sz, feetY; worldFindSpawn(&g_world, &sx, &sz, &feetY);
                      g_level.player->x = sx + 0.5f; g_level.player->z = sz + 0.5f;
                      g_level.player->y = feetY + PLAYER_EYE; }
                    g_genPhase = 1;
                    g_saveShowProgress = false;
                    LevelStorage::save(&g_world, tArgs.dir, seedVal, tArgs.gamemode, tArgs.name, true);
                    g_saveShowProgress = true;
                }
                g_terrainThreadDone = true;
                g_genStage = GS_TERRAIN_WAIT;
            }
            drawGeneratingScreen(s, 0);
            return;
        }
        if (g_genStage == GS_TERRAIN_WAIT) {
            const char* status = g_loadedFromDisk ? "Loading world"
                               : (g_genPhase == 1 ? "Saving chunks" : "Building terrain");

            drawGeneratingScreen(s, g_terrainProgress * 90 / 100, status);
            if (g_terrainThreadDone) {
                if (!g_haveTerrain) {
                    g_haveTerrain = loadTex(&g_terrain, "data/images/terrain.png");
                    if (g_haveTerrain) {
                        bool m1 = loadTexMip(&g_terrain, 0, "data/images/terrainMipMapLevel2.png");
                        bool m2 = loadTexMip(&g_terrain, 1, "data/images/terrainMipMapLevel3.png");
                        if (!m1 || !m2) textureGenMips(&g_terrain, 16);
                    }
                }
                if (!g_haveGuiBlocks)
                    g_haveGuiBlocks = loadTex16(&g_guiBlocks, "data/images/gui/gui_blocks.png", GU_PSM_5551);
                if (!g_haveClouds)
                    g_haveClouds = loadTex16(&g_clouds, "data/images/environment/clouds.png", GU_PSM_5551);
                if (!g_haveParticles)
                    g_haveParticles = loadTex(&g_particles, "data/images/particles.png");
                g_genStage = GS_MESHING;
            }
            return;
        }
        if (g_genStage == GS_MESHING) {
            const int total = WORLD_CHUNKS_X * WORLD_CHUNKS_Z;
            int done = worldBuildMeshesStep(&g_world, MESH_CHUNKS_PER_FRAME);
            drawGeneratingScreen(s, 90 + (done * 10 / total));

            if (done >= total) {
                g_worldBuilt = true; g_genStage = GS_IDLE;

                MeshWorker::start();
                extern int g_autosaveTick; g_autosaveTick = 0;
                particlesReset();
                if (g_loadedFromDisk && LevelStorage::loadedValidPlayerPos()) {

                    playerSpawnAt(g_level.player->y);
                } else {

                    int sx, sz, feetY;
                    worldFindSpawn(&g_world, &sx, &sz, &feetY);
                    g_level.player->x = sx + 0.5f; g_level.player->z = sz + 0.5f;
                    playerSpawnAt(feetY + PLAYER_EYE);

                    MobSpawner::populateInitial(&g_level);
                }

                int gamemode = (s.worldSelected >= 0 && s.worldSelected < s.worlds.count)
                             ? s.worlds.gameModes[s.worldSelected] : 1;
                gameModeInit(gamemode);

                if (g_loadedFromDisk) LevelStorage::applyLoadedHotbar();
            }
            return;

        }
    }

    if (g_craftOpen || g_armorOpen || g_furnaceOpen || g_chestOpen) {

        if (g_worldBuilt && g_level.player)
            worldRebuildStep(&g_world, g_level.player->x, g_level.player->y,
                             g_level.player->z, g_viewDist);
        return;
    }

    if (g_diagMode == 1) sceGuScissor(0, 0, 240, 136);
    else                 sceGuScissor(0, 0, 480, 272);

    float a = g_timerAlpha;
    float ix = g_level.player->xo + (g_level.player->x - g_level.player->xo) * a;
    float iy = g_level.player->yo + (g_level.player->y - g_level.player->yo) * a;
    if (g_level.player->sneaking) iy -= 0.08f;
    float iz = g_level.player->zo + (g_level.player->z - g_level.player->zo) * a;
    float iyaw   = g_level.player->yRotO   + (g_level.player->yRot   - g_level.player->yRotO)   * a;
    float ipitch = g_level.player->xRotO + (g_level.player->xRot - g_level.player->xRotO) * a;

    {
        static float s_down = 0.0f;
        float tgt = g_level.player->isSleeping() ? 1.0f : 0.0f;
        s_down += (tgt - s_down) * 0.15f;
        if (s_down > 0.002f)
            iy += (g_level.player->bedY + 1.0f - iy) * s_down;
        if (g_level.player->isSleeping()) {
            int sdir = worldData(&g_world, g_level.player->bedX,
                                 g_level.player->bedY, g_level.player->bedZ) & 3;
            iyaw = 180.0f - sdir * 90.0f;
            ipitch = 0.0f;
        }
    }

    if (g_photoPending) {
        ix = g_photoX; iy = g_photoY; iz = g_photoZ;
        iyaw = g_photoYaw; ipitch = g_photoPitch;
    }

    float px0 = ix, py0 = iy, pz0 = iz;

    unsigned char eyeBlk = worldBlock(&g_world, (int)ix, (int)iy, (int)iz);
    float fov = isWaterId(eyeBlk) ? 60.0f : 70.0f;

    float cp = cosf(ipitch * DEG2RAD), sp = sinf(ipitch * DEG2RAD);
    float cy = cosf(iyaw * DEG2RAD),   sy = sinf(iyaw * DEG2RAD);
    float fx = cp * sy, fy = sp, fz = cp * cy;
    float rx = cy,       rz = -sy;
    float ux = fy * rz,  uy = fz * rx - fx * rz, uz = -fy * rx;

    extern bool g_thirdPerson;

    bool thirdNow = g_thirdPerson && !g_level.player->isSleeping() && !g_photoPending;
    float camBack = 0.0f;
    if (thirdNow) {
        float best = 4.0f;
        for (int i = 0; i < 8; i++) {
            float xo = (float)((i & 1) * 2 - 1) * 0.1f;
            float yo = (float)(((i >> 1) & 1) * 2 - 1) * 0.1f;
            float zo = (float)(((i >> 2) & 1) * 2 - 1) * 0.1f;
            BlockHit ch = worldPick(&g_world, ix + xo, iy + yo, iz + zo, iyaw + 180.0f, -ipitch, 4.0f);
            if (ch.hit) {
                float hx = ch.x + ch.clickX - (ix + xo);
                float hy = ch.y + ch.clickY - (iy + yo);
                float hz = ch.z + ch.clickZ - (iz + zo);

                float dist = sqrtf(hx * hx + hy * hy + hz * hz) - 0.25f;
                if (dist < 0.0f) dist = 0.0f;
                if (dist < best) best = dist;
            }
        }
        camBack = best;
    }
    float nearOx = ix - fx * camBack, nearOy = iy - fy * camBack, nearOz = iz - fz * camBack;

    float dpCamX = 0.0f, dpCamY = 0.0f, dpCamZ = 0.0f;
    if (thirdNow) {
        const float baseCamX = nearOx, baseCamY = nearOy, baseCamZ = nearOz;
        const float CLEAR = 0.05f;
        for (int iter = 0; iter < 3; iter++) {
            bool moved = false;
            int cbx = (int)floorf(nearOx), cby = (int)floorf(nearOy), cbz = (int)floorf(nearOz);
            for (int ddx = -1; ddx <= 1; ddx++)
            for (int ddy = -1; ddy <= 1; ddy++)
            for (int ddz = -1; ddz <= 1; ddz++) {
                int bxx = cbx + ddx, byy = cby + ddy, bzz = cbz + ddz;
                if (!isSolidPhys(worldBlock(&g_world, bxx, byy, bzz))) continue;

                float cx = nearOx < bxx ? bxx : (nearOx > bxx + 1 ? bxx + 1 : nearOx);
                float cy = nearOy < byy ? byy : (nearOy > byy + 1 ? byy + 1 : nearOy);
                float cz = nearOz < bzz ? bzz : (nearOz > bzz + 1 ? bzz + 1 : nearOz);
                float vx = nearOx - cx, vy = nearOy - cy, vz = nearOz - cz;
                float d2 = vx * vx + vy * vy + vz * vz;
                if (d2 >= CLEAR * CLEAR) continue;
                if (d2 > 1e-8f) {
                    float d = sqrtf(d2), push = (CLEAR - d) / d;
                    nearOx += vx * push; nearOy += vy * push; nearOz += vz * push;
                } else {
                    float ex0 = nearOx - bxx, ex1 = (bxx + 1) - nearOx;
                    float ey0 = nearOy - byy, ey1 = (byy + 1) - nearOy;
                    float ez0 = nearOz - bzz, ez1 = (bzz + 1) - nearOz;
                    float mx = ex0 < ex1 ? -(ex0 + CLEAR) : (ex1 + CLEAR);
                    float my = ey0 < ey1 ? -(ey0 + CLEAR) : (ey1 + CLEAR);
                    float mz = ez0 < ez1 ? -(ez0 + CLEAR) : (ez1 + CLEAR);
                    if (fabsf(mx) <= fabsf(my) && fabsf(mx) <= fabsf(mz)) nearOx += mx;
                    else if (fabsf(my) <= fabsf(mz)) nearOy += my;
                    else nearOz += mz;
                }
                moved = true;
            }
            if (!moved) break;
        }
        dpCamX = nearOx - baseCamX; dpCamY = nearOy - baseCamY; dpCamZ = nearOz - baseCamZ;
    }

    const float TH = 1.3f, TV = 0.75f;
    float nearSolid = 2.0f;

    for (float gridY = -1.0f; gridY <= 1.01f; gridY += 0.5f) {
        for (float gridX = -1.0f; gridX <= 1.01f; gridX += 0.5f) {
            float sxx = gridX * TH, syy = gridY * TV;
            float dx = fx + rx * sxx + ux * syy, dy = fy + uy * syy, dz = fz + rz * sxx + uz * syy;
            float len = sqrtf(dx * dx + dy * dy + dz * dz);
            dx /= len; dy /= len; dz /= len;

            for (float t = 0.1f; t <= 0.65f && t < nearSolid; t += 0.05f) {
                if (isSolidPhys(worldBlock(&g_world, (int)floorf(nearOx + dx * t),
                                                     (int)floorf(nearOy + dy * t),
                                                     (int)floorf(nearOz + dz * t)))) {
                    nearSolid = t;
                    break;
                }
            }
        }
    }

    float targetNearZ = nearSolid * 0.4f;
    if (targetNearZ > 0.25f) targetNearZ = 0.25f;

    if (targetNearZ < 0.14f) targetNearZ = 0.14f;

    static float s_targetNearZ = 0.25f;
    if (fabsf(targetNearZ - s_targetNearZ) > 0.025f || targetNearZ == 0.14f || targetNearZ == 0.25f) {
        s_targetNearZ = targetNearZ;
    }

    static float s_nearZ = 0.25f;
    if (s_targetNearZ < s_nearZ) {
        s_nearZ = s_targetNearZ;
    } else {
        s_nearZ += (s_targetNearZ - s_nearZ) * 0.05f;
    }
    float NEAR_Z = s_nearZ;

    guPerspective(fov, NEAR_Z, g_viewDist);

    float roll = 0.0f, rgxUp = 0.0f, rgzUp = 0.0f;
    float bs = 0.0f, bc = 0.0f;
    if (g_viewBobbing) {
        float wda = g_level.player->walkDist - g_level.player->walkDistO;
        float b = -(g_level.player->walkDistO + wda * a);
        float bobv  = g_level.player->oBob  + (g_level.player->bob  - g_level.player->oBob)  * a;
        float tiltv = g_level.player->oTilt + (g_level.player->tilt - g_level.player->oTilt) * a;
        const float PIF = 3.14159265f;
        bs = sinf(b * PIF) * bobv * 0.5f;
        bc = fabsf(cosf(b * PIF)) * bobv;

        float rgx = cosf(iyaw * DEG2RAD), rgz = -sinf(iyaw * DEG2RAD);
        ix -= rgx * bs; iz -= rgz * bs;
        iy -= bc;
        ipitch -= tiltv;

        if (ipitch >  89.0f) ipitch =  89.0f;
        if (ipitch < -89.0f) ipitch = -89.0f;
    }

    if (g_level.player->hurtTime > 0 && g_level.player->hurtDuration > 0) {
        float f = (g_level.player->hurtTime - a) / (float)g_level.player->hurtDuration;
        if (f > 0.0f) {
            f = sinf(f * f * f * f * 3.14159265f);
            roll -= f * 14.0f * cosf(g_level.player->hurtDir * DEG2RAD);
        }
    }

    float icp = cosf(ipitch * DEG2RAD), isp = sinf(ipitch * DEG2RAD);
    float icy = cosf(iyaw * DEG2RAD),   isy = sinf(iyaw * DEG2RAD);
    float ifx = icp * isy, ify = isp, ifz = icp * icy;

    float ex = ix - ifx * camBack + dpCamX, ey = iy - ify * camBack + dpCamY, ez = iz - ifz * camBack + dpCamZ;
    float ctrX = ix + ifx, ctrY = iy + ify, ctrZ = iz + ifz;
    g_camX = ex; g_camY = ey; g_camZ = ez;

    extern float g_relBaseX, g_relBaseY, g_relBaseZ;
    g_relBaseX = floorf(ix); g_relBaseY = floorf(iy); g_relBaseZ = floorf(iz);

    loadWorldView(ex, ey, ez, ctrX, ctrY, ctrZ, roll, 0.0f, 0.0f, 0.0f);

    worldSetFrustumCamera(ex, ey, ez, ifx, ify, ifz, iyaw, fov,
                          (float)GU_SCR_WIDTH / (float)GU_SCR_HEIGHT, NEAR_Z, g_viewDist);

    updateDayColors(a);

    if (g_fancyGraphics) {

        sceGumMatrixMode(GU_PROJECTION);
        sceGumPushMatrix();
        sceGumLoadIdentity();
        sceGumPerspective(fov, (float)GU_SCR_WIDTH / (float)GU_SCR_HEIGHT, 0.15f, 400.0f);
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        sceGuEnable(GU_FOG);

        sceGuFog(0.0f, 150.0f, g_skyColorNow);
        renderSky(px0, py0, pz0);
        sceGumMatrixMode(GU_PROJECTION);
        sceGumPopMatrix();

        sceGumMatrixMode(GU_PROJECTION);
        sceGumPushMatrix();
        sceGumLoadIdentity();
        sceGumPerspective(fov, (float)GU_SCR_WIDTH / (float)GU_SCR_HEIGHT, 0.15f, 450.0f);
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        float distToCloud = fabsf(128.0f - py0);
        float cloudFogNear = distToCloud * 0.8f;
        if (cloudFogNear < 32.0f) cloudFogNear = 32.0f;
        sceGuFog(cloudFogNear, distToCloud + 64.0f, g_skyColorNow);
        renderClouds(a, px0, py0, pz0);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumPopMatrix();
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        sceGuEnable(GU_DEPTH_TEST);
    }

    loadWorldView(ex, ey, ez, ctrX, ctrY, ctrZ, roll, g_relBaseX, g_relBaseY, g_relBaseZ);

    extern unsigned int g_tCount, g_tAlloc, g_tEmit, g_tPack;
    g_tCount = g_tAlloc = g_tEmit = g_tPack = 0;

    animateWaterTexture();

    if (g_haveTerrain) {

        if (g_noMipmap) textureBindNoMip(&g_terrain);
        else            textureBind(&g_terrain);
    }
    else sceGuDisable(GU_TEXTURE_2D);

    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);

    sceGuDisable(GU_BLEND);

    sceGuEnable(GU_FOG);

    extern float g_fogCullDist;
    g_fogCullDist = 0.0f;
    int eyeBr = lightRawAt(&g_world, (int)floorf(ix), (int)floorf(iy), (int)floorf(iz));
    unsigned int M = g_brightColor[eyeBr] & 0xFF;

    if (isWaterId(eyeBlk)) {
        unsigned int b = (230 * M) / 255;
        unsigned int g = (102 * M) / 255;
        unsigned int r = (25 * M) / 255;
        sceGuFog(0.0f, 25.0f, 0xFF000000u | (b << 16) | (g << 8) | r);
        g_fogCullDist = 25.0f + 24.0f;
    }
    else if (isLavaId(eyeBlk)) {
        unsigned int b = (25 * M) / 255;
        unsigned int g = (51 * M) / 255;
        unsigned int r = (204 * M) / 255;
        sceGuFog(0.0f, 3.0f, 0xFF000000u | (b << 16) | (g << 8) | r);
        g_fogCullDist = 3.0f + 24.0f;
    }
    else {
        sceGuFog(g_viewDist * 0.6f, g_viewDist, g_skyColorNow);
    }

    sceGuFrontFace(GU_CCW);
    worldDraw(&g_world, px0, py0, pz0, g_viewDist, g_haveTerrain ? &g_terrain : 0);

    renderMiningCrack(px0, py0, pz0);

    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    EntityRenderDispatcher::getInstance()->renderAll(&g_level, a);
    renderAllTileEntities(&g_level, a);

    extern bool g_thirdPerson;

    if (g_photoPending ||
        (g_thirdPerson && !(g_level.player && g_level.player->isSleeping())))
        playerModelRender(a);

    if (g_haveTerrain) { if (g_noMipmap) textureBindNoMip(&g_terrain); else textureBind(&g_terrain); }
    sceGuDepthMask(GU_TRUE);
    sceGuDisable(GU_CULL_FACE);

    worldDrawWater(&g_world, px0, py0, pz0, g_viewDist);
    sceGuEnable(GU_CULL_FACE);

    if (g_haveParticles)
        particlesRender(&g_world, iyaw, ipitch, a,
                        g_haveTerrain ? &g_terrain : 0, &g_particles,
                        g_haveGuiBlocks ? &g_guiBlocks : 0);

    sceGuDepthMask(GU_FALSE);

    renderSelectionOutline(px0, py0, pz0);

    loadWorldView(ex, ey, ez, ctrX, ctrY, ctrZ, roll, 0.0f, 0.0f, 0.0f);

    extern bool g_thirdPerson;

    if (!g_thirdPerson && g_level.player && g_level.player->health > 0 &&
        !g_level.player->isSleeping() && !g_photoPending) itemHandDraw(a, bs, bc);

    if (g_worldBuilt) fireScreenEffect();

    guOrtho();

    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuDisable(GU_FOG);
    sceGuDisable(GU_DEPTH_TEST);

    sceGuFrontFace(GU_CW);
    sceGuScissor(0, 0, 480, 272);

    if (g_worldBuilt) drawInWallOverlay(px0, py0, pz0);

    if (g_worldBuilt && g_level.player && g_level.player->isSleeping())
        inBedRenderFade(s);

    if (g_worldBuilt && !g_photoPending) {
        if (g_invOpen) inventoryDraw(s);
        hotbarDraw(s);
    }
}
