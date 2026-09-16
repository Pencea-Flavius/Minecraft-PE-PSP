#include "gpu/texture.h"

#include "platform/png_loader.h"
#include "gpu/gu.h"

#include <pspgu.h>
#include <pspkernel.h>
#include <cstdlib>
#include <cstring>
#include "platform/dcache.h"
#include <malloc.h>

static const Texture* s_lastBound = nullptr;

static int nextPow2(int v) {
    int p = 1;
    while (p < v)
        p <<= 1;
    return p;
}

unsigned int g_textureBindFailures = 0;
char g_textureLastFailed[80] = "";

unsigned int g_textureLoadFailures = 0;
const char*  g_textureFailReason = "";
unsigned int g_textureFailHeapUsed = 0;
unsigned int g_textureFailHeapBig  = 0;

static unsigned int probeLargestBlock(void) {
    for (unsigned int sz = 1u << 20; sz >= 4096u; sz >>= 1) {
        void* p = malloc(sz);
        if (p) { free(p); return sz; }
    }
    return 0;
}

static char s_failed[32][80];
static int  s_failedCount = 0;
static bool alreadyFailed(const char* path) {
    for (int i = 0; i < s_failedCount; i++)
        if (strcmp(s_failed[i], path) == 0) return true;
    return false;
}
void textureForgetFailures() { s_failedCount = 0; }

static void markFailed(const char* path, const char* why, bool optional = false) {
    if (optional) return;
    g_textureLoadFailures++;
    {
        struct mallinfo mi = mallinfo();
        g_textureFailHeapUsed = (unsigned int)mi.uordblks;
        g_textureFailHeapBig  = probeLargestBlock();
    }
    g_textureFailReason = why;
    std::strncpy(g_textureLastFailed, path, sizeof(g_textureLastFailed) - 1);
    g_textureLastFailed[sizeof(g_textureLastFailed) - 1] = 0;
    if (s_failedCount >= (int)(sizeof(s_failed) / sizeof(s_failed[0]))) return;
    std::strncpy(s_failed[s_failedCount], path, sizeof(s_failed[0]) - 1);
    s_failed[s_failedCount][sizeof(s_failed[0]) - 1] = 0;
    s_failedCount++;
}

static inline unsigned short pack16(const unsigned char* s, int psm) {
    if (psm == GU_PSM_4444)
        return (unsigned short)(((s[3] >> 4) << 12) | ((s[2] >> 4) << 8) |
                                ((s[1] >> 4) << 4)  |  (s[0] >> 4));
    if (psm == GU_PSM_5551)
        return (unsigned short)(((s[3] >> 7) << 15) | ((s[2] >> 3) << 10) |
                                ((s[1] >> 3) << 5)  |  (s[0] >> 3));

    return (unsigned short)(((s[2] >> 3) << 11) | ((s[1] >> 2) << 5) | (s[0] >> 3));
}

static void* texAlloc(bool wantVram, size_t bytes, bool* gotVram) {
#if !TEXTURE_USE_VRAM
    wantVram = false;
#endif
    if (wantVram) {
        if (void* v = guVramAllocTexture((unsigned int)bytes)) { *gotVram = true; return v; }
    }
    *gotVram = false;
    return memalign(64, bytes);
}

static void texFree(bool vram, void* p) {
    if (!p) return;
    if (vram) guVramFreeTexture(p); else free(p);
}

static bool textureLoadPsm(const char* path, Texture* out, int psm, bool wantVram = false,
                           bool optional = false, unsigned int offset = 0,
                           const unsigned char* mem = 0, unsigned int memSize = 0) {
#if TEXTURE_FORCE_8888
    psm = GU_PSM_8888;
#endif
    if (alreadyFailed(path)) { memset(out, 0, sizeof(*out)); return false; }

    memset(out, 0, sizeof(*out));

    int w = 0, h = 0;
    PngReader* png = mem ? pngOpenMem(mem + offset, memSize, &w, &h) : pngOpenAt(path, offset, &w, &h);
    if (!png) {

        markFailed(path, g_pngLastError[0] ? g_pngLastError : "open", optional);
        return false;
    }

    int texW = nextPow2(w);
    int texH = nextPow2(h);

    const bool is16 = (psm == GU_PSM_4444 || psm == GU_PSM_5551 || psm == GU_PSM_5650);
    const size_t bpp = is16 ? 2 : 4;
    const size_t bytes = (size_t)texW * texH * bpp;

    void* data = texAlloc(wantVram, bytes, &out->vram);
    if (!data) {

        pngClose(png);
        markFailed(path, "mem", optional);
        return false;
    }
    memset(data, 0, bytes);

    unsigned char* row = (unsigned char*)malloc((size_t)w * 4);
    if (!row) {
        texFree(out->vram, data);
        pngClose(png);
        markFailed(path, "row", optional);
        return false;
    }
    for (int y = 0; y < h; y++) {
        if (!pngReadRow(png, row)) {

            free(row);
            texFree(out->vram, data);
            pngClose(png);
            markFailed(path, g_pngLastError[0] ? g_pngLastError : "decode", optional);
            return false;
        }
        if (is16) {
            const unsigned char* s = row;
            unsigned short* d = (unsigned short*)data + (size_t)y * texW;
            for (int x = 0; x < w; x++, s += 4)
                d[x] = pack16(s, psm);
        } else {
            memcpy((unsigned char*)data + (size_t)y * texW * 4, row, (size_t)w * 4);
        }
    }
    free(row);
    pngClose(png);

    dcacheFlush(data, bytes);

    out->realW = w;
    out->realH = h;
    out->texW = texW;
    out->texH = texH;
    out->data = data;
    out->psm  = psm;
    return true;
}

bool textureLoad(const char* path, Texture* out) {
    return textureLoadPsm(path, out, GU_PSM_8888);
}

bool textureLoad4444(const char* path, Texture* out) {
    return textureLoadPsm(path, out, GU_PSM_4444);
}

bool textureLoad16(const char* path, Texture* out, int psm) {
    return textureLoadPsm(path, out, psm);
}

bool textureLoad16Optional(const char* path, Texture* out, int psm) {
    return textureLoadPsm(path, out, psm, false, true);
}

bool textureLoad16At(const char* path, unsigned int offset, Texture* out, int psm) {
    return textureLoadPsm(path, out, psm, false, true, offset);
}

bool textureLoad16Mem(const char* name, const unsigned char* mem, unsigned int offset, unsigned int size,
                      Texture* out, int psm) {
    return textureLoadPsm(name, out, psm, false, true, offset, mem, size);
}

bool textureLoadVram(const char* path, Texture* out, int psm) {
    return textureLoadPsm(path, out, psm, true);
}

void textureFree(Texture* tex) {

    if (tex == s_lastBound) s_lastBound = 0;

    texFree(tex->vram, tex->data);
    for (int i = 0; i < tex->mipCount; i++)
        texFree(tex->vram, tex->mip[i]);
    memset(tex, 0, sizeof(*tex));
}

bool textureLoadMipLevel(Texture* tex, int level, const char* path) {
    if (level < 0 || level >= TEXTURE_MAX_MIPS) return false;
    int w = 0, h = 0;
    PngReader* png = pngOpen(path, &w, &h);
    if (!png) return false;

    bool gotVram = false;
    void* data = texAlloc(tex->vram, (size_t)w * h * 4, &gotVram);
    if (!data) { pngClose(png); return false; }

    if (tex->vram && !gotVram) { free(data); pngClose(png); return false; }

    for (int y = 0; y < h; y++)
        if (!pngReadRow(png, (unsigned char*)data + (size_t)y * w * 4)) {
            texFree(gotVram, data);
            pngClose(png); return false;
        }
    pngClose(png);
    dcacheFlush(data, (size_t)w * h * 4);

    texFree(tex->vram, tex->mip[level]);
    tex->mip[level] = data;
    if (level + 1 > tex->mipCount) tex->mipCount = level + 1;
    return true;
}

static void downsample2x(const unsigned char* src, int srcW, int srcH, unsigned char* dst) {
    int dstW = srcW / 2, dstH = srcH / 2;
    for (int y = 0; y < dstH; y++) {
        const unsigned char* row0 = src + (size_t)(y * 2)     * srcW * 4;
        const unsigned char* row1 = src + (size_t)(y * 2 + 1) * srcW * 4;
        unsigned char* out = dst + (size_t)y * dstW * 4;
        for (int x = 0; x < dstW; x++) {
            const unsigned char* p[4] = {
                row0 + (size_t)(x * 2)     * 4, row0 + (size_t)(x * 2 + 1) * 4,
                row1 + (size_t)(x * 2)     * 4, row1 + (size_t)(x * 2 + 1) * 4
            };
            int aSum = p[0][3] + p[1][3] + p[2][3] + p[3][3];
            unsigned char* o = out + x * 4;
            if (aSum == 0) {
                o[0] = o[1] = o[2] = o[3] = 0;
                continue;
            }
            for (int c = 0; c < 3; c++) {
                int wsum = p[0][c] * p[0][3] + p[1][c] * p[1][3] + p[2][c] * p[2][3] + p[3][c] * p[3][3];
                o[c] = (unsigned char)(wsum / aSum);
            }

            bool pureCutout = (p[0][3] == 0 || p[0][3] == 255) && (p[1][3] == 0 || p[1][3] == 255) &&
                              (p[2][3] == 0 || p[2][3] == 255) && (p[3][3] == 0 || p[3][3] == 255);
            o[3] = pureCutout ? (unsigned char)255 : (unsigned char)((aSum + 2) / 4);
        }
    }
}

void textureGenMips(Texture* tex, int minSize) {
    if (minSize < 1) minSize = 1;
    int w = tex->texW, h = tex->texH;
    int i = 0;
    const unsigned char* src = (const unsigned char*)tex->data;
    while (w / 2 >= minSize && h / 2 >= minSize && i < TEXTURE_MAX_MIPS) {
        int dstW = w / 2, dstH = h / 2;
        if (!tex->mip[i]) {
            bool gotVram = false;
            tex->mip[i] = texAlloc(tex->vram, (size_t)dstW * dstH * 4, &gotVram);
            if (!tex->mip[i]) break;

            if (tex->vram && !gotVram) { free(tex->mip[i]); tex->mip[i] = 0; break; }
        }
        downsample2x(src, w, h, (unsigned char*)tex->mip[i]);
        dcacheFlush(tex->mip[i], (size_t)dstW * dstH * 4);
        src = (const unsigned char*)tex->mip[i];
        w = dstW; h = dstH;
        i++;
    }
    tex->mipCount = i;
}

static void swizzleBlock(void* dst, const void* src, int rowBytes, int height) {
    int wBlocks = rowBytes / 16;
    int hBlocks = height / 8;
    int srcPitch = (rowBytes - 16) / 4;
    int srcRow = rowBytes * 8;

    const unsigned char* ysrc = (const unsigned char*)src;
    unsigned int* out = (unsigned int*)dst;

    for (int by = 0; by < hBlocks; by++) {
        const unsigned char* xsrc = ysrc;
        for (int bx = 0; bx < wBlocks; bx++) {
            const unsigned int* s = (const unsigned int*)xsrc;
            for (int j = 0; j < 8; j++) {
                *out++ = *s++;
                *out++ = *s++;
                *out++ = *s++;
                *out++ = *s++;
                s += srcPitch;
            }
            xsrc += 16;
        }
        ysrc += srcRow;
    }
}

static void swizzleLevelInPlace(void** dataPtr, int w, int h) {
    int rowBytes = w * 4;
    size_t size = (size_t)rowBytes * h;
    void* tmp = memalign(16, size);
    if (!tmp) return;
    swizzleBlock(tmp, *dataPtr, rowBytes, h);
    memcpy(*dataPtr, tmp, size);
    free(tmp);
    dcacheFlush(*dataPtr, size);
}

void textureSwizzle(Texture* tex) {
    if (tex->swizzled) return;
    swizzleLevelInPlace(&tex->data, tex->texW, tex->texH);
    int w = tex->texW, h = tex->texH;
    for (int i = 0; i < tex->mipCount; i++) {
        w /= 2; h /= 2;
        if (tex->mip[i]) swizzleLevelInPlace(&tex->mip[i], w, h);
    }
    tex->swizzled = true;
}

void textureSwizzleBandInto(Texture* tex, int yTop, int bandH, const void* linearBandRGBA) {
    int rowBytes = tex->texW * 4;
    int blockX = rowBytes / 16;
    size_t bandSize = (size_t)rowBytes * bandH;
    void* tmp = memalign(16, bandSize);
    if (!tmp) return;
    swizzleBlock(tmp, linearBandRGBA, rowBytes, bandH);
    size_t offset = (size_t)(yTop / 8) * blockX * 128;
    memcpy((unsigned char*)tex->data + offset, tmp, bandSize);
    free(tmp);
    dcacheFlush((unsigned char*)tex->data + offset, bandSize);
}

void textureBindLastBoundReset() { s_lastBound = nullptr; }

static float s_mipBias = -1.5f;

void textureMipAuto() { sceGuTexLevelMode(GU_TEXTURE_AUTO, s_mipBias); }

void textureBind(const Texture* tex) {

    if (!tex || !tex->data || tex->texW <= 0 || tex->texH <= 0) {
        g_textureBindFailures++;
        s_lastBound = 0;
        sceGuDisable(GU_TEXTURE_2D);
        return;
    }

    if (tex != s_lastBound) {
    s_lastBound = tex;

    sceGuTexMode(tex->psm, tex->mipCount, 0, tex->swizzled ? 1 : 0);
    sceGuTexImage(0, tex->texW, tex->texH, tex->texW, tex->data);
    int w = tex->texW, h = tex->texH;
    for (int i = 0; i < tex->mipCount; i++) {
        w /= 2; h /= 2;
        sceGuTexImage(i + 1, w, h, w, tex->mip[i]);
    }
    }

    {
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);

    sceGuTexFilter(tex->mipCount ? GU_NEAREST_MIPMAP_LINEAR : GU_NEAREST, GU_NEAREST);

    if (tex->mipCount) sceGuTexLevelMode(GU_TEXTURE_AUTO, s_mipBias);
    else               sceGuTexLevelMode(GU_TEXTURE_CONST, 0.0f);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    }
    sceGuEnable(GU_TEXTURE_2D);

    sceGuTexFlush();
}

void textureBindNoMip(const Texture* tex) {
    if (!tex || !tex->data || tex->texW <= 0 || tex->texH <= 0) {
        g_textureBindFailures++;
        textureBindLastBoundReset();
        sceGuDisable(GU_TEXTURE_2D);
        return;
    }
    textureBindLastBoundReset();
    sceGuTexMode(tex->psm, 0, 0, tex->swizzled ? 1 : 0);
    sceGuTexImage(0, tex->texW, tex->texH, tex->texW, tex->data);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexLevelMode(GU_TEXTURE_CONST, 0.0f);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexFlush();
}
