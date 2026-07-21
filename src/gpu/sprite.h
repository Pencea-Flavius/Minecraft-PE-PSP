
#ifndef MCPSP_GPU_SPRITE_H
#define MCPSP_GPU_SPRITE_H

struct Texture;

void spriteDraw(const Texture* tex,
                float dx, float dy, float dw, float dh,
                float sx, float sy, float sw, float sh,
                unsigned int color);

void spriteDrawFull(const Texture* tex, float dx, float dy, unsigned int color);

void spriteDrawTiled(const Texture* tex,
                     float dx, float dy, float dw, float dh,
                     float tileScreenPx, unsigned int color);

#endif
