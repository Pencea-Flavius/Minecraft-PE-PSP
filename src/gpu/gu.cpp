#include "gpu/gu.h"

#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

static unsigned int __attribute__((aligned(16))) g_list[524288 / 4];
static void* g_listUncached = 0;

static unsigned int g_vramOffset = 0;

static void* g_fb[2] = { 0, 0 };
static int   g_drawIdx = 0;

static unsigned int guMemSize(unsigned int width, unsigned int height,
                              unsigned int psm) {
    unsigned int bytesPerPixel;
    switch (psm) {
        case GU_PSM_T4:   return (width * height) >> 1;
        case GU_PSM_T8:   bytesPerPixel = 1; break;
        case GU_PSM_5650:
        case GU_PSM_5551:
        case GU_PSM_4444:
        case GU_PSM_T16:  bytesPerPixel = 2; break;
        case GU_PSM_8888:
        case GU_PSM_T32:  bytesPerPixel = 4; break;
        default:          bytesPerPixel = 4; break;
    }
    return width * height * bytesPerPixel;
}

static void* guVramAlloc(unsigned int width, unsigned int height,
                         unsigned int psm) {
    void* result = (void*)(unsigned long)g_vramOffset;
    g_vramOffset += guMemSize(width, height, psm);
    return result;
}

void guInit(void) {

    g_listUncached = (void*)((unsigned int)g_list | 0x40000000u);

    g_fb[0] = guVramAlloc(GU_BUF_WIDTH, GU_SCR_HEIGHT, GU_PSM_5650);
    g_fb[1] = guVramAlloc(GU_BUF_WIDTH, GU_SCR_HEIGHT, GU_PSM_5650);
    void* zbp = guVramAlloc(GU_BUF_WIDTH, GU_SCR_HEIGHT, GU_PSM_4444);
    g_drawIdx = 0;

    sceGuInit();
    sceGuStart(GU_DIRECT, g_listUncached);

    sceGuDrawBuffer(GU_PSM_5650, g_fb[0], GU_BUF_WIDTH);
    sceGuDispBuffer(GU_SCR_WIDTH, GU_SCR_HEIGHT, g_fb[1], GU_BUF_WIDTH);
    sceGuDepthBuffer(zbp, GU_BUF_WIDTH);

    sceGuOffset(2048 - (GU_SCR_WIDTH / 2), 2048 - (GU_SCR_HEIGHT / 2));
    sceGuViewport(2048, 2048, GU_SCR_WIDTH, GU_SCR_HEIGHT);
    sceGuDepthRange(65535, 0);

    sceGuScissor(0, 0, GU_SCR_WIDTH, GU_SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);

    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_FLAT);
    sceGuEnable(GU_CULL_FACE);
    sceGuEnable(GU_CLIP_PLANES);

    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void guTerm(void) {
    sceGuTerm();
}

void guStartFrame(unsigned int clearColor) {
    sceGuStart(GU_DIRECT, g_listUncached);

    sceGuDrawBuffer(GU_PSM_5650, g_fb[g_drawIdx], GU_BUF_WIDTH);

    sceGuScissor(0, 0, GU_SCR_WIDTH, GU_SCR_HEIGHT);

    sceGuClearColor(clearColor);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
}

void guFinishFrame(void) {
    sceGuFinish();
    sceGuSync(0, 0);
}

void guPresent(void) {

    void* addr = (void*)((unsigned int)sceGeEdramGetAddr() + (unsigned int)g_fb[g_drawIdx]);
    sceDisplaySetFrameBuf(addr, GU_BUF_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_565,
                          PSP_DISPLAY_SETBUF_NEXTFRAME);
    g_drawIdx ^= 1;

    sceDisplayWaitVblankStart();
}

void guEndFrame(void) {
    guFinishFrame();
    guPresent();
}

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
bool guSavePhotoPng(const char* path) {
    const int shotBytes = GU_BUF_WIDTH * GU_SCR_HEIGHT * 2;

    unsigned short* shot = (unsigned short*)memalign(64, shotBytes);
    if (!shot) return false;

    sceKernelDcacheWritebackInvalidateRange(shot, shotBytes);
    sceGuStart(GU_DIRECT, g_listUncached);
    sceGuCopyImage(GU_PSM_5650, 0, 0, GU_SCR_WIDTH, GU_SCR_HEIGHT, GU_BUF_WIDTH,
                   (void*)((unsigned int)sceGeEdramGetAddr() + (unsigned int)g_fb[g_drawIdx]),
                   0, 0, GU_BUF_WIDTH, shot);
    sceGuFinish();
    sceGuSync(0, 0);

    const unsigned short* shotRd = (const unsigned short*)((unsigned int)shot | 0x40000000u);

    FILE* f = fopen(path, "wb");
    if (!f) { free(shot); return false; }
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    png_infop info = png ? png_create_info_struct(png) : 0;
    if (!png || !info || setjmp(png_jmpbuf(png))) {
        if (png) png_destroy_write_struct(&png, info ? &info : 0);
        fclose(f);
        free(shot);
        return false;
    }
    png_init_io(png, f);
    png_set_IHDR(png, info, GU_SCR_WIDTH, GU_SCR_HEIGHT, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    unsigned char row[GU_SCR_WIDTH * 3];
    for (int y = 0; y < GU_SCR_HEIGHT; y++) {
        const unsigned short* src = shotRd + y * GU_BUF_WIDTH;
        for (int x = 0; x < GU_SCR_WIDTH; x++) {
            unsigned short p = src[x];
            row[x * 3 + 0] = (unsigned char)(( p        & 0x1F) << 3);
            row[x * 3 + 1] = (unsigned char)(((p >> 5)  & 0x3F) << 2);
            row[x * 3 + 2] = (unsigned char)(((p >> 11) & 0x1F) << 3);
        }
        png_write_row(png, row);
    }
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);
    fclose(f);
    free(shot);
    return true;
}

void guOrtho(void) {
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumOrtho(0, GU_SCR_WIDTH, GU_SCR_HEIGHT, 0, -1.0f, 1.0f);
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}

void guPerspective(float fovDeg, float nearZ, float farZ) {
    const float aspect = (float)GU_SCR_WIDTH / (float)GU_SCR_HEIGHT;

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(fovDeg, aspect, nearZ, farZ);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}
