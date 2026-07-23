#include "client/renderer/water_anim.h"
#include "gpu/texture.h"
#include "platform/time.h"
#include <pspkernel.h>
#include <cmath>
#include <cstdlib>

extern Texture g_terrain;
extern bool    g_haveTerrain;

static float waterCurrent[256];
static float waterNext[256];
static float waterHeat[256];
static float waterHeata[256];

static float lavaCurrent[256];
static float lavaNext[256];
static float lavaHeat[256];
static float lavaHeata[256];

static float sideCurrent[256];
static float sideNext[256];
static float sideHeat[256];
static float sideHeata[256];
static int   sideTickCount = 0;

static float fireBufA[2][16 * 20];
static float fireBufB[2][16 * 20];
static float* fireCurrent[2] = { fireBufA[0], fireBufA[1] };
static float* fireNext[2]    = { fireBufB[0], fireBufB[1] };

static float mthRandom() {
    return (float)rand() / (float)RAND_MAX;
}

void animateWaterTexture() {
    static float timer = 0.0f;
    float now = nowSeconds();
    if (now - timer < 0.05f) return;
    timer = now;

    if (!g_haveTerrain) return;

    unsigned int* texPixels = (unsigned int*)g_terrain.data;
    int texW = g_terrain.texW;

    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            float pow = 0;
            for (int xx = x - 1; xx <= x + 1; xx++) {
                int xi = (xx) & 15;
                int yi = (y) & 15;
                pow += waterCurrent[xi + yi * 16];
            }
            waterNext[x + y * 16] = pow / 3.3f + waterHeat[x + y * 16] * 0.8f;
        }
    }

    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            waterHeat[x + y * 16] += waterHeata[x + y * 16] * 0.05f;
            if (waterHeat[x + y * 16] < 0) waterHeat[x + y * 16] = 0;
            waterHeata[x + y * 16] -= 0.1f;
            if (mthRandom() < 0.05f) {
                waterHeata[x + y * 16] = 0.5f;
            }
        }
    }

    for (int i=0; i<256; i++) {
        float tmp = waterNext[i];
        waterNext[i] = waterCurrent[i];
        waterCurrent[i] = tmp;
    }

    sideTickCount++;
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            float pow = 0;
            for (int xx = y - 2; xx <= y; xx++) {
                pow += sideCurrent[(x & 15) + (xx & 15) * 16];
            }
            sideNext[x + y * 16] = pow / 3.2f + sideHeat[x + y * 16] * 0.8f;
        }
    }
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            sideHeat[x + y * 16] += sideHeata[x + y * 16] * 0.05f;
            if (sideHeat[x + y * 16] < 0) sideHeat[x + y * 16] = 0;
            sideHeata[x + y * 16] -= 0.3f;
            if (mthRandom() < 0.2f) sideHeata[x + y * 16] = 0.5f;
        }
    }
    for (int i = 0; i < 256; i++) {
        float tmp = sideNext[i];
        sideNext[i] = sideCurrent[i];
        sideCurrent[i] = tmp;
    }

    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            float pow = 0;
            int xxo = (int)(sinf((float)(y) * (float)(M_PI) * 2.0f / 16.0f) * 1.2f);
            int yyo = (int)(sinf((float)(x) * (float)(M_PI) * 2.0f / 16.0f) * 1.2f);
            for (int xx = x - 1; xx <= x + 1; xx++) {
                for (int yy = y - 1; yy <= y + 1; yy++) {
                    int xi = (xx + xxo) & 15;
                    int yi = (yy + yyo) & 15;
                    pow += lavaCurrent[xi + yi * 16];
                }
            }
            lavaNext[x + y * 16] = pow / 10.0f + (lavaHeat[((x + 0) & 15) + ((y + 0) & 15) * 16] + lavaHeat[((x + 1) & 15) + ((y + 0) & 15) * 16] + lavaHeat[((x + 1) & 15) + ((y + 1) & 15) * 16] + lavaHeat[((x + 0) & 15) + ((y + 1) & 15) * 16]) / 4.0f * 0.8f;
            lavaHeat[x + y * 16] = lavaHeat[x + y * 16] + lavaHeata[x + y * 16] * 0.01f;
            if (lavaHeat[x + y * 16] < 0.0f) {
                lavaHeat[x + y * 16] = 0.0f;
            }
            lavaHeata[x + y * 16] = lavaHeata[x + y * 16] - 0.06f;
            if (mthRandom() < 0.005) {
                lavaHeata[x + y * 16] = 1.5f;
            }
        }
    }

    for (int i=0; i<256; i++) {
        float tmp = lavaNext[i];
        lavaNext[i] = lavaCurrent[i];
        lavaCurrent[i] = tmp;
    }

    for (int f = 0; f < 2; f++) {
        float* cur = fireCurrent[f];
        float* nxt = fireNext[f];
        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 20; y++) {
                int count = 18;
                float pow = cur[x + (y + 1) % 20 * 16] * (float)count;
                for (int xx = x - 1; xx <= x + 1; xx++) {
                    for (int yy = y; yy <= y + 1; yy++) {
                        if (xx >= 0 && yy >= 0 && xx < 16 && yy < 20) pow += cur[xx + yy * 16];
                        count++;
                    }
                }
                nxt[x + y * 16] = pow / ((float)count * 1.06f);
                if (y >= 19)
                    nxt[x + y * 16] = (float)(mthRandom() * mthRandom() * mthRandom() * 4.0 +
                                              mthRandom() * 0.1f + 0.2f);
            }
        }
        float* tmp = fireNext[f]; fireNext[f] = fireCurrent[f]; fireCurrent[f] = tmp;
    }

    for (int i = 0; i < 256; i++) {
        int x = i % 16;
        int y = i / 16;

        float wpow = waterCurrent[i];
        if (wpow > 1) wpow = 1;
        if (wpow < 0) wpow = 0;
        float wpp = wpow * wpow;
        unsigned int waterCol = ((unsigned)(146 + wpp * 50) << 24) | (255u << 16) |
                                ((unsigned)(50 + wpp * 64) << 8) | (unsigned)(32 + wpp * 32);

        float spow = sideCurrent[(i - sideTickCount * 16) & 255];
        if (spow > 1) spow = 1;
        if (spow < 0) spow = 0;
        float spp = spow * spow;
        unsigned int sideCol = ((unsigned)(146 + spp * 50) << 24) | (255u << 16) |
                               ((unsigned)(50 + spp * 64) << 8) | (unsigned)(32 + spp * 32);

        float lpow = lavaCurrent[i] * 2.0f;
        if (lpow > 1) lpow = 1;
        if (lpow < 0) lpow = 0;
        float lpp = lpow * lpow;
        int lr = (int)(lpow * 100.0f + 155.0f);
        int lg = (int)(lpp * 255.0f);
        int lb = (int)(lpp * lpp * 128.0f);
        unsigned int lavaCol = (255u << 24) | (lb << 16) | (lg << 8) | lr;

        texPixels[(12 * 16 + y) * texW + (13 * 16 + x)] = waterCol;

        texPixels[(12 * 16 + y) * texW + (14 * 16 + x)] = sideCol;
        texPixels[(12 * 16 + y) * texW + (15 * 16 + x)] = sideCol;
        texPixels[(13 * 16 + y) * texW + (14 * 16 + x)] = sideCol;
        texPixels[(13 * 16 + y) * texW + (15 * 16 + x)] = sideCol;
        texPixels[(14 * 16 + y) * texW + (13 * 16 + x)] = lavaCol;

        texPixels[(14 * 16 + y) * texW + (14 * 16 + x)] = lavaCol;
        texPixels[(14 * 16 + y) * texW + (15 * 16 + x)] = lavaCol;
        texPixels[(15 * 16 + y) * texW + (14 * 16 + x)] = lavaCol;
        texPixels[(15 * 16 + y) * texW + (15 * 16 + x)] = lavaCol;
    }

    for (int f = 0; f < 2; f++) {
        float* cur = fireCurrent[f];
        for (int i = 0; i < 256; i++) {
            int x = i % 16, y = i / 16;
            float fpow = cur[i] * 1.8f;
            if (fpow > 1.0f) fpow = 1.0f;
            if (fpow < 0.0f) fpow = 0.0f;
            int fr = (int)(fpow * 155.0f + 100.0f);
            int fg = (int)(fpow * fpow * 255.0f);
            int fb = (int)(fpow * fpow * fpow * fpow * fpow * fpow * fpow * fpow * fpow * fpow * 255.0f);
            int fa = (fpow < 0.5f) ? 0 : 255;
            unsigned int fireCol = ((unsigned)fa << 24) | ((unsigned)fb << 16) | ((unsigned)fg << 8) | (unsigned)fr;
            texPixels[((1 + f) * 16 + y) * texW + (15 * 16 + x)] = fireCol;
        }
    }

    sceKernelDcacheWritebackInvalidateRange(&texPixels[16 * texW], 32 * texW * sizeof(unsigned int));

    sceKernelDcacheWritebackInvalidateRange(&texPixels[192 * texW],
                                            (256 - 192) * texW * sizeof(unsigned int));
}
