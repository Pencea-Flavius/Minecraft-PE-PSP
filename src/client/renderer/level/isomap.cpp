
#include "world/level/world.h"
#include "gpu/gu.h"
#include "gpu/texture.h"
#include "platform/dcache.h"
#include "platform/power.h"
#include "world/level/levelgen/level_source.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/tile/entity/tile_entity.h"
#include "client/renderer/entity/entity_render_dispatcher.h"
#include "client/renderer/entity/player_model.h"
#include "client/renderer/tileentity/tile_entity_renderer.h"
#include "client/renderer/render.h"

#include <pspgu.h>
#include <pspgum.h>
#include <png.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

void streamFreeSection(ChunkSection* s);
int  seaCellQuarter(int cx, int cz);

static const int kIsoA[] = { 3, 2 };

static int PAD = 2;

static const int SEA_CHUNKS = 16;

unsigned int skyIsoBackdropColor();
unsigned int skyIsoCloudColor();
bool skyIsoCloudsOn();
bool skyIsoCloudsFancy();
float skyIsoCloudScroll();
bool skyIsoCloudAt(float wx, float wz);

static inline unsigned short abgrTo5650(unsigned int c) {
    return (unsigned short)(((c & 0xFF) >> 3) | (((c >> 8) & 0xFF) >> 2) << 5 | (((c >> 16) & 0xFF) >> 3) << 11);
}

static void isoClouds(unsigned short* canvas, int W, int H, int A, float cloudY) {
    const unsigned int c = skyIsoCloudColor();
    const unsigned a = (c >> 24) & 0xFF;
    const unsigned cr = (c & 0xFF) >> 3, cg = ((c >> 8) & 0xFF) >> 2, cb = ((c >> 16) & 0xFF) >> 3;
    for (int Y = 0; Y < H; Y++) {
        const float xPlusZ = 2.0f * ((float)(Y - PAD) / A + cloudY - WORLD_H);
        for (int X = 0; X < W; X++) {
            const float xMinusZ = (float)(X - PAD) / A - WORLD_W;
            if (!skyIsoCloudAt((xPlusZ + xMinusZ) * 0.5f, (xPlusZ - xMinusZ) * 0.5f)) continue;
            unsigned short& p = canvas[Y * W + X];
            unsigned r = p & 0x1F, g = (p >> 5) & 0x3F, b = (p >> 11) & 0x1F;
            r = (r * (255 - a) + cr * a) / 255;
            g = (g * (255 - a) + cg * a) / 255;
            b = (b * (255 - a) + cb * a) / 255;
            p = (unsigned short)(r | (g << 5) | (b << 11));
        }
    }
}

static void isoFillConvex(unsigned short* canvas, int W, int H,
                          const float* px, const float* py, int n, unsigned short col) {
    float y0 = py[0], y1 = py[0];
    for (int i = 1; i < n; i++) { if (py[i] < y0) y0 = py[i]; if (py[i] > y1) y1 = py[i]; }
    int ya = (int)ceilf(y0 - 0.5f), yb = (int)floorf(y1 - 0.5f);
    if (ya < 0) ya = 0;
    if (yb > H - 1) yb = H - 1;
    for (int Y = ya; Y <= yb; Y++) {
        const float sy = Y + 0.5f;
        float xl = 1e9f, xr = -1e9f;
        for (int i = 0; i < n; i++) {
            const int j = (i + 1) % n;
            const float ay = py[i], by = py[j];
            if ((sy < ay) == (sy < by)) continue;
            const float x = px[i] + (px[j] - px[i]) * (sy - ay) / (by - ay);
            if (x < xl) xl = x;
            if (x > xr) xr = x;
        }
        int xa = (int)ceilf(xl - 0.5f), xe = (int)floorf(xr - 0.5f);
        if (xa < 0) xa = 0;
        if (xe > W - 1) xe = W - 1;
        for (int X = xa; X <= xe; X++) canvas[Y * W + X] = col;
    }
}

static void isoCloudsFancy(unsigned short* canvas, int W, int H, int A, float cloudY,
                           unsigned short bg) {
    const float CELL = 12.0f, THICK = 4.0f;
    const unsigned int c = skyIsoCloudColor();

    auto blendOver = [&](unsigned a, float shade) -> unsigned short {
        const unsigned cr = (unsigned)(((c & 0xFF) >> 3) * shade);
        const unsigned cg = (unsigned)((((c >> 8) & 0xFF) >> 2) * shade);
        const unsigned cb = (unsigned)((((c >> 16) & 0xFF) >> 3) * shade);
        unsigned r = bg & 0x1F, g = (bg >> 5) & 0x3F, b = (bg >> 11) & 0x1F;
        r = (r * (255 - a) + cr * a) / 255;
        g = (g * (255 - a) + cg * a) / 255;
        b = (b * (255 - a) + cb * a) / 255;
        return (unsigned short)(r | (g << 5) | (b << 11));
    };
    const unsigned short colTop = blendOver(0xD0, 1.0f), colSide = blendOver(0xC0, 1.0f);

    const float off = skyIsoCloudScroll();
    const float minXpZ = 2.0f * ((0.0f - PAD) / A + cloudY - THICK - WORLD_H) - CELL * 2;
    const float maxXpZ = 2.0f * ((float)(H - PAD) / A + cloudY - WORLD_H) + CELL * 2;
    const float minXmZ = (0.0f - PAD) / A - WORLD_W - CELL * 2;
    const float maxXmZ = (float)(W - PAD) / A - WORLD_W + CELL * 2;
    const int kLo = (int)floorf(minXpZ / CELL) - 1, kHi = (int)ceilf(maxXpZ / CELL) + 1;
    auto P = [&](float x, float y, float z, float* ox, float* oy) {
        *ox = (x - z) * A + WORLD_W * A + PAD;
        *oy = (x + z) * A * 0.5f - y * A + WORLD_H * A + PAD;
    };

    auto solid = [&](int i, int j) { return skyIsoCloudAt((i + 0.5f) * CELL - off, (j + 0.5f) * CELL); };

    for (int k = kLo; k <= kHi; k++) {
        for (int i = (int)floorf((k * CELL + minXmZ) / (2 * CELL)) - 1;
                 i <= (int)ceilf((k * CELL + maxXmZ) / (2 * CELL)) + 1; i++) {
            const int j = k - i;
            if (!solid(i, j)) continue;
            const float x0 = i * CELL - off, x1 = x0 + CELL, z0 = j * CELL, z1 = z0 + CELL;
            const float yt = cloudY, yb = cloudY - THICK;
            float qx[4], qy[4];
            if (!solid(i + 1, j)) {
                P(x1, yt, z0, &qx[0], &qy[0]); P(x1, yt, z1, &qx[1], &qy[1]);
                P(x1, yb, z1, &qx[2], &qy[2]); P(x1, yb, z0, &qx[3], &qy[3]);
                isoFillConvex(canvas, W, H, qx, qy, 4, colSide);
            }
            if (!solid(i, j + 1)) {
                P(x0, yt, z1, &qx[0], &qy[0]); P(x1, yt, z1, &qx[1], &qy[1]);
                P(x1, yb, z1, &qx[2], &qy[2]); P(x0, yb, z1, &qx[3], &qy[3]);
                isoFillConvex(canvas, W, H, qx, qy, 4, colSide);
            }
            P(x0, yt, z0, &qx[0], &qy[0]); P(x1, yt, z0, &qx[1], &qy[1]);
            P(x1, yt, z1, &qx[2], &qy[2]); P(x0, yt, z1, &qx[3], &qy[3]);
            isoFillConvex(canvas, W, H, qx, qy, 4, colTop);
        }
    }
}

static inline int isoSectionOfY(float y) {
    int si = (int)floorf(y) >> 4;
    return si < 0 ? 0 : (si >= N_SECTIONS ? N_SECTIONS - 1 : si);
}
static inline int isoSectionKey(float x, float y, float z) {
    const int cx = (int)floorf(x) >> 4, cz = (int)floorf(z) >> 4;
    if (!worldChunkInBounds(cx, cz)) return -1;
    return (cx * WORLD_CHUNKS_Z + cz) * N_SECTIONS + isoSectionOfY(y);
}

static bool s_secHasThings[WORLD_CHUNKS_X * WORLD_CHUNKS_Z * N_SECTIONS];

static void isoMarkThings() {
    memset(s_secHasThings, 0, sizeof(s_secHasThings));
    for (size_t i = 0; i < g_level.entities.size(); i++) {
        Entity* e = g_level.entities[i];
        if (!e || e->removed || e->invisible) continue;
        const int k = isoSectionKey(e->x, e->bb.y0, e->z);
        if (k >= 0) s_secHasThings[k] = true;
    }
    for (size_t i = 0; i < g_level.tileEntities.size(); i++) {
        TileEntity* te = g_level.tileEntities[i];
        if (!te || te->removed || te->rendererId == TR_NO_RENDER) continue;
        const int k = isoSectionKey(te->x + 0.5f, (float)te->y, te->z + 0.5f);
        if (k >= 0) s_secHasThings[k] = true;
    }
    if (LocalPlayer* p = g_level.player) {
        const int k = isoSectionKey(p->x, p->bb.y0, p->z);
        if (k >= 0 && p->health > 0) s_secHasThings[k] = true;
    }
}

static void isoDrawThings(int cx, int cz, int si) {
    const int key = (cx * WORLD_CHUNKS_Z + cz) * N_SECTIONS + si;
    for (size_t i = 0; i < g_level.entities.size(); i++) {
        Entity* e = g_level.entities[i];
        if (!e || e->removed || e->invisible) continue;
        if (isoSectionKey(e->x, e->bb.y0, e->z) == key)
            EntityRenderDispatcher::getInstance()->render(e, 1.0f);
    }
    for (size_t i = 0; i < g_level.tileEntities.size(); i++) {
        TileEntity* te = g_level.tileEntities[i];
        if (!te || te->removed || te->rendererId == TR_NO_RENDER) continue;
        if (isoSectionKey(te->x + 0.5f, (float)te->y, te->z + 0.5f) == key)
            renderTileEntityOne(te, 1.0f);
    }
    if (LocalPlayer* p = g_level.player)
        if (p->health > 0 && isoSectionKey(p->x, p->bb.y0, p->z) == key) playerModelRender(1.0f);
}

bool isoMapRender(World* w, const Texture* terrain, const char* path, void (*progress)(int pct)) {
    if (!worldFitsInWindow(w)) return false;

    int A = 0, W = 0, H = 0;
    unsigned short* canvas = 0;
    for (unsigned k = 0; k < sizeof(kIsoA) / sizeof(kIsoA[0]) && !canvas; k++) {
        A = kIsoA[k];
        PAD = 3 * A;
        W = WORLD_W * 2 * A + 2 * PAD + 1;
        H = WORLD_D * A + WORLD_H * A + 2 * PAD + 1;
        canvas = (unsigned short*)malloc((size_t)W * H * 2);
    }
    if (!canvas) return false;

    const int B = 32 * A + 2 * PAD + 1;
    const int bx = GU_SCR_WIDTH / 2 - 16 * A - PAD, by = GU_SCR_HEIGHT / 2 - 16 * A - PAD;

    const int BW = GU_BUF_WIDTH;
    unsigned short* box = (unsigned short*)memalign(64, (size_t)BW * B * 2);
    if (!box) { free(canvas); return false; }
    dcacheFlush(box, (size_t)BW * B * 2);
    unsigned short* boxU = (unsigned short*)((unsigned int)box | 0x40000000u);

    const unsigned short bg = abgrTo5650(skyIsoBackdropColor());
    for (int i = 0; i < W * H; i++) canvas[i] = bg;

    const float sx = (float)A / (GU_SCR_WIDTH / 2), sy = (float)A / (GU_SCR_HEIGHT / 2);
    ScePspFMatrix4 proj = {
        {  sx, -sy * 0.5f, -1.0f / 32.0f, 0.0f },
        { 0.0f,  sy,       -1.0f / 32.0f, 0.0f },
        { -sx, -sy * 0.5f, -1.0f / 32.0f, 0.0f },
        { 0.0f, 0.0f,      24.0f / 32.0f, 1.0f },
    };

    const float saveCamX = g_camX, saveCamY = g_camY, saveCamZ = g_camZ, saveYaw = g_camYawNow;
    g_camX = WORLD_W * 0.5f + 10000.0f; g_camY = 10000.0f; g_camZ = WORLD_D * 0.5f + 10000.0f;
    g_camYawNow = 135.0f;
    isoMarkThings();

    chunkMeshHeapProbe();

    void* offscreen = guVramAllocTexture((unsigned int)GU_BUF_WIDTH * GU_SCR_HEIGHT * 2);

    auto drawSection = [&](const ChunkSection* s, int cbx, int cby,
                           int thingsCx = -1, int thingsCz = -1, int thingsSi = -1) -> bool {

        for (int y = 0; y < B; y++) {
            const int cy = cby + y;
            for (int x = 0; x < B; x++) {
                const int cx = cbx + x;
                boxU[y * BW + bx + x] = (cx >= 0 && cx < W && cy >= 0 && cy < H) ? canvas[cy * W + cx] : bg;
            }
        }

        if (!offscreen) guWaitDrawBufferHidden();
        if (!guStartFrame(0xFF000000u)) return false;
        void* fb = guDrawBufferVram();
        if (offscreen) {

            sceGuDrawBufferList(GU_PSM_5650,
                                (void*)((unsigned int)offscreen - (unsigned int)sceGeEdramGetAddr()),
                                GU_BUF_WIDTH);
            fb = offscreen;
        }
        sceGuCopyImage(GU_PSM_5650, 0, 0, GU_SCR_WIDTH, B, BW, box, 0, by, GU_BUF_WIDTH, fb);

        guSetDither(1);
        sceGumMatrixMode(GU_PROJECTION); sceGumLoadMatrix(&proj);
        sceGumMatrixMode(GU_VIEW);       sceGumLoadIdentity();

        if (thingsCx >= 0) { g_relBaseX = (float)(thingsCx * CHUNK_SX); g_relBaseY = (float)(thingsSi * SECTION_SY); g_relBaseZ = (float)(thingsCz * CHUNK_SZ); }
        else               { g_relBaseX = (float)s->ox; g_relBaseY = (float)s->oy; g_relBaseZ = (float)s->oz; }
        sceGuFrontFace(GU_CCW);

        if (terrain) textureBindNoMip(terrain);

        sceGuDisable(GU_ALPHA_TEST);
        chunkDrawSection(s);
        sceGuEnable(GU_ALPHA_TEST);
        chunkDrawNoMipSection(s, NOMIP_ALL);
        chunkDrawLeavesSection(s);
        if (thingsCx >= 0) {
            isoDrawThings(thingsCx, thingsCz, thingsSi);

            sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity();
            sceGuDisable(GU_LIGHTING);
            sceGuDepthOffset(0);
            sceGuColor(0xFFFFFFFFu);
            sceGuEnable(GU_TEXTURE_2D);
            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
            sceGuEnable(GU_ALPHA_TEST);
            sceGuEnable(GU_CULL_FACE);
            sceGuFrontFace(GU_CCW);
            sceGuDepthMask(GU_FALSE);
            if (terrain) textureBindNoMip(terrain);
        }
        sceGuDepthMask(GU_TRUE);
        chunkDrawWaterSection(s);
        sceGuDepthMask(GU_FALSE);

        sceGuCopyImage(GU_PSM_5650, 0, by, GU_SCR_WIDTH, B, GU_BUF_WIDTH, fb, 0, 0, BW, box);
        guFinishFrame();

        for (int y = 0; y < B; y++) {
            const int cy = cby + y;
            if (cy < 0 || cy >= H) continue;
            for (int x = 0; x < B; x++) {
                const int cx = cbx + x;
                if (cx >= 0 && cx < W) canvas[cy * W + cx] = boxU[y * BW + bx + x];
            }
        }
        return true;
    };
    auto hasGeometry = [](const ChunkSection* s) {
        return s->vertexCount || s->noMipCount || s->leavesCount || s->waterCount;
    };

    unsigned char edgeCol[WORLD_H];
    activeBorderColumn(edgeCol, &g_edgeSkyFromY);
    g_edgeColumn = edgeCol;

    const float cloudY = activeLevelSource().cloudHeight();
    if (skyIsoCloudsOn() && cloudY < 0.0f) {
        if (skyIsoCloudsFancy()) isoCloudsFancy(canvas, W, H, A, cloudY, bg);
        else                     isoClouds(canvas, W, H, A, cloudY);
    }

    ChunkMesh seaTmpl, seaOne;
    memset(&seaTmpl, 0, sizeof(seaTmpl));
    memset(&seaOne, 0, sizeof(seaOne));
    seaTmpl.ox = seaTmpl.oz = -8 * CHUNK_SX;
    bool tmplTried[N_SECTIONS] = { false };

    const int lo = -SEA_CHUNKS, hi = WORLD_CHUNKS_X + SEA_CHUNKS;
    const int keyLo = 2 * lo, keyHi = 2 * (hi - 1) + N_SECTIONS - 1;
    int drawn = 0;
    bool ok = true;
    for (int key = keyLo; key <= keyHi && ok; key++) {
        for (int cx = lo; cx < hi && ok; cx++) {
            for (int si = 0; si < N_SECTIONS && ok; si++) {
                const int cz = key - cx - si;
                if (cz < lo || cz >= hi) continue;
                const int ox = cx * CHUNK_SX, oz = cz * CHUNK_SZ;
                const int cbx = (ox - oz) * A + (WORLD_W - 16) * A;
                const int cby = (ox + oz) * A / 2 - si * SECTION_SY * A + (WORLD_H - 16) * A;
                if (cbx + B <= 0 || cbx >= W || cby + B <= 0 || cby >= H) continue;

                if (worldChunkInBounds(cx, cz)) {
                    if (!worldChunkReady(w, cx, cz) || worldSlotBusy(worldSlot(w, cx, cz))) continue;
                    ChunkMesh* c = worldMesh(w, cx, cz);
                    ChunkSection* s = &c->sec[si];
                    const bool had = s->mesh || s->water || s->leaves || s->noMip;
                    const bool edge = cx == 0 || cz == 0 || cx == WORLD_CHUNKS_X - 1 || cz == WORLD_CHUNKS_Z - 1;
                    const bool build = s->dirty || edge;
                    if (build) {
                        if (!worldChunkMeshable(w, cx, cz)) continue;
                        chunkBuildSection(c, w, si);
                        if (s->dirty) continue;
                    }
                    const bool things = s_secHasThings[(cx * WORLD_CHUNKS_Z + cz) * N_SECTIONS + si];
                    if (hasGeometry(s) || things)
                        ok = drawSection(s, cbx, cby, things ? cx : -1, cz, si);
                    if (build) {

                        if (!had) streamFreeSection(s);
                        else      s->dirty = true;
                    }
                } else {
                    if (!edgeSectionVisible(edgeCol, si)) continue;
                    const bool nearWorld = cx >= -1 && cz >= -1 && cx <= WORLD_CHUNKS_X && cz <= WORLD_CHUNKS_Z;
                    ChunkSection* s;
                    if (nearWorld) {
                        seaOne.ox = ox; seaOne.oz = oz;
                        s = &seaOne.sec[si];
                        s->dirty = true;
                        chunkBuildSection(&seaOne, w, si);
                        if (s->dirty) continue;
                    } else {
                        s = &seaTmpl.sec[si];
                        if (!tmplTried[si]) {
                            tmplTried[si] = true;
                            s->dirty = true;
                            chunkBuildSection(&seaTmpl, w, si);
                        }
                        if (s->dirty) continue;
                        s->ox = ox; s->oz = oz;
                        if (edgeColumnTopRotates(edgeCol)) g_chunkDrawQuarter = seaCellQuarter(cx, cz);
                    }
                    if (hasGeometry(s)) ok = drawSection(s, cbx, cby);
                    g_chunkDrawQuarter = 0;
                    if (nearWorld) streamFreeSection(s);
                }

                if ((++drawn & 31) == 0) {
                    chunkMeshHeapProbe();
                    if (progress) progress((key - keyLo) * 90 / (keyHi - keyLo + 1));
                }
            }
        }
    }
    g_edgeColumn = 0;
    g_camX = saveCamX; g_camY = saveCamY; g_camZ = saveCamZ; g_camYawNow = saveYaw;
    for (int si = 0; si < N_SECTIONS; si++) streamFreeSection(&seaTmpl.sec[si]);
    free(box);
    if (offscreen) guVramFreeTexture(offscreen);

    if (ok) {
        if (progress) progress(90);
        PowerHold hold;
        FILE* f = fopen(path, "wb");
        png_structp png = f ? png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0) : 0;
        png_infop info = png ? png_create_info_struct(png) : 0;
        unsigned char* row = (unsigned char*)malloc((size_t)W * 3);
        if (!png || !info || !row || setjmp(png_jmpbuf(png))) {
            ok = false;
        } else {
            png_init_io(png, f);
            png_set_compression_level(png, 1);
            png_set_IHDR(png, info, W, H, 8, PNG_COLOR_TYPE_RGB,
                         PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            png_write_info(png, info);
            for (int y = 0; y < H; y++) {
                const unsigned short* src = canvas + y * W;
                for (int x = 0; x < W; x++) {
                    unsigned short p = src[x];
                    unsigned r = p & 0x1F, g = (p >> 5) & 0x3F, b = (p >> 11) & 0x1F;
                    row[x * 3 + 0] = (unsigned char)((r << 3) | (r >> 2));
                    row[x * 3 + 1] = (unsigned char)((g << 2) | (g >> 4));
                    row[x * 3 + 2] = (unsigned char)((b << 3) | (b >> 2));
                }
                png_write_row(png, row);
            }
            png_write_end(png, info);
        }
        if (png) png_destroy_write_struct(&png, info ? &info : 0);
        free(row);
        if (f) fclose(f);
    }
    free(canvas);
    return ok;
}
