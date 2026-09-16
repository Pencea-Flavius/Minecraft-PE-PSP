#include "platform/png_loader.h"

#include <png.h>
#include <cstdio>
#include <cstdlib>
#include <csetjmp>

struct PngReader {
    png_structp png;
    png_infop   info;
    FILE*       fp;
};

char g_pngLastError[64] = "";

static void pngErrorFn(png_structp png, png_const_charp msg) {
    if (msg) {
        int i = 0;
        for (; msg[i] && i < (int)sizeof(g_pngLastError) - 1; i++) g_pngLastError[i] = msg[i];
        g_pngLastError[i] = 0;
    }

    longjmp(png_jmpbuf(png), 1);
}

static void pngWarnFn(png_structp, png_const_charp) {}

PngReader* pngOpen(const char* path, int* outW, int* outH) {
    return pngOpenAt(path, 0, outW, outH);
}

PngReader* pngOpenAt(const char* path, unsigned int offset, int* outW, int* outH) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    if (offset && fseek(fp, (long)offset, SEEK_SET) != 0) { fclose(fp); return 0; }

    g_pngLastError[0] = 0;
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0,
                                             pngErrorFn, pngWarnFn);
    if (!png) { fclose(fp); return 0; }
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, 0, 0); fclose(fp); return 0; }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, 0);
        fclose(fp);
        return 0;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    int w = (int)png_get_image_width(png, info);
    int h = (int)png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);

    if (bitDepth == 16)
        png_set_strip_16(png);
    if (colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);
    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    if (png_get_interlace_type(png, info) != PNG_INTERLACE_NONE) {
        png_destroy_read_struct(&png, &info, 0);
        fclose(fp);
        return 0;
    }

    PngReader* r = (PngReader*)malloc(sizeof(PngReader));
    if (!r) { png_destroy_read_struct(&png, &info, 0); fclose(fp); return 0; }
    r->png = png; r->info = info; r->fp = fp;
    *outW = w;
    *outH = h;
    return r;
}

bool pngReadRow(PngReader* r, unsigned char* rgbaRow) {
    if (setjmp(png_jmpbuf(r->png))) return false;
    png_read_row(r->png, (png_bytep)rgbaRow, 0);
    return true;
}

void pngClose(PngReader* r) {
    if (!r) return;
    png_destroy_read_struct(&r->png, &r->info, 0);
    fclose(r->fp);
    free(r);
}
