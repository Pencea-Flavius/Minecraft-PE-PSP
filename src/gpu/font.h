
#ifndef MCPSP_GPU_FONT_H
#define MCPSP_GPU_FONT_H

#include "gpu/texture.h"

struct Font {
    Texture tex;
    unsigned char charWidth[256];
    int lineHeight;
};

bool fontLoad(const char* path, Font* out);

void fontFree(Font* f);

void fontDrawText(const Font* f, float x, float y, const char* text, unsigned int color, float scale = 1.0f);

void fontDrawTextShadow(const Font* f, float x, float y, const char* text, unsigned int color, float scale = 1.0f);

int fontTextWidth(const Font* f, const char* text);

#endif
