#include "gpu/font.h"
#include <cmath>

#include "gpu/sprite.h"

#include <cstring>

namespace {
const int CELL = 8;
const int COLS = 16;
const int LINE_HEIGHT = 10;
}

bool fontLoad(const char* path, Font* out) {
    if (!textureLoad(path, &out->tex))
        return false;
    out->lineHeight = LINE_HEIGHT;

    const unsigned char* px = (const unsigned char*)out->tex.data;
    const int stride = out->tex.texW;

    for (int i = 0; i < 256; i++) {
        int xt = i % COLS;
        int yt = i / COLS;
        int cellX = xt * CELL;
        int cellY = yt * CELL;

        int x = CELL - 1;
        for (; x >= 0; x--) {
            bool emptyColumn = true;
            for (int y = 0; y < CELL && emptyColumn; y++) {
                int px_x = cellX + x;
                int px_y = cellY + y;
                unsigned char alpha = px[((size_t)px_y * stride + px_x) * 4 + 3];
                if (alpha > 0)
                    emptyColumn = false;
            }
            if (!emptyColumn)
                break;
        }

        if (i == ' ')
            x = 4 - 2;
        out->charWidth[i] = (unsigned char)(x + 2);
    }
    return true;
}

void fontFree(Font* f) {
    textureFree(&f->tex);
    memset(f, 0, sizeof(*f));
}

static void drawGlyphs(const Font* f, float x, float y, const char* text, unsigned int color, float scale) {
    textureBind(&f->tex);

    float cursorX = x, cursorY = y;
    for (const unsigned char* p = (const unsigned char*)text; *p; p++) {
        unsigned char ch = *p;
        if (ch == '\n') {
            cursorX = x;
            cursorY += f->lineHeight * scale;
            continue;
        }
        int xt = ch % COLS;
        int yt = ch / COLS;

        spriteDraw(&f->tex, floorf(cursorX + 0.5f), floorf(cursorY + 0.5f),
                   CELL * scale, CELL * scale,
                  (float)(xt * CELL), (float)(yt * CELL), (float)CELL, (float)CELL,
                  color);
        cursorX += f->charWidth[ch] * scale;
    }
}

void fontDrawText(const Font* f, float x, float y, const char* text, unsigned int color, float scale) {
    drawGlyphs(f, x, y, text, color, scale);
}

void fontDrawTextShadow(const Font* f, float x, float y, const char* text, unsigned int color, float scale) {
    unsigned int alphaByte = color & 0xFF000000u;
    unsigned int shadow = ((color & 0x00FCFCFCu) >> 2) | alphaByte;

    float off = floorf(scale); if (off < 1.0f) off = 1.0f;
    drawGlyphs(f, x + off, y + off, text, shadow, scale);
    drawGlyphs(f, x, y, text, color, scale);
}

int fontTextWidth(const Font* f, const char* text) {
    int maxLen = 0, len = 0;
    for (const unsigned char* p = (const unsigned char*)text; *p; p++) {
        if (*p == '\n') {
            if (len > maxLen) maxLen = len;
            len = 0;
        } else {
            len += f->charWidth[*p];
        }
    }
    return len > maxLen ? len : maxLen;
}
