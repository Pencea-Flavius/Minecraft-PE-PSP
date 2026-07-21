
#ifndef MCPSP_GPU_WIDGETS_H
#define MCPSP_GPU_WIDGETS_H

#include "gpu/font.h"
#include "gpu/texture.h"

struct PocketButton {
    float x, y, size;
    float iconSrcX, iconSrcY, iconSrcSize;
    const char* label;
    bool active;
};

void pocketButtonDraw(const Font* font, const Texture* guiAtlas, const Texture* iconAtlas,
                      const PocketButton* b, bool hovered, float labelScale = 1.0f);

#endif
