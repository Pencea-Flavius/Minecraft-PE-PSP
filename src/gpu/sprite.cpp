#include "gpu/sprite.h"

#include "gpu/texture.h"

#include <pspgu.h>

namespace {
struct TexVertex {
    float u, v;
    unsigned int color;
    float x, y, z;
};
}

void spriteDraw(const Texture* tex,
                float dx, float dy, float dw, float dh,
                float sx, float sy, float sw, float sh,
                unsigned int color) {
    (void)tex;

    TexVertex* v = (TexVertex*)sceGuGetMemory(2 * sizeof(TexVertex));

    v[0].u = sx;        v[0].v = sy;
    v[0].color = color; v[0].x = dx;      v[0].y = dy;      v[0].z = 0.0f;
    v[1].u = sx + sw;   v[1].v = sy + sh;
    v[1].color = color; v[1].x = dx + dw; v[1].y = dy + dh; v[1].z = 0.0f;

    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   2, 0, v);
}

void spriteDrawFull(const Texture* tex, float dx, float dy, unsigned int color) {
    spriteDraw(tex, dx, dy, (float)tex->realW, (float)tex->realH,
               0.0f, 0.0f, (float)tex->realW, (float)tex->realH, color);
}

void spriteDrawTiled(const Texture* tex,
                     float dx, float dy, float dw, float dh,
                     float tileScreenPx, unsigned int color) {
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);

    float sw = dw * ((float)tex->realW / tileScreenPx);
    float sh = dh * ((float)tex->realH / tileScreenPx);
    spriteDraw(tex, dx, dy, dw, dh, 0.0f, 0.0f, sw, sh, color);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
}
