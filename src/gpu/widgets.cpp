#include "gpu/widgets.h"

#include "gpu/sprite.h"
#include "gpu/gui_atlas.h"

void pocketButtonDraw(const Font* font, const Texture* , const Texture* iconAtlas,
                      const PocketButton* b, bool hovered, float labelScale) {
    bool pressedVisual = b->active && hovered;
    float scale = pressedVisual ? 0.95f : 1.0f;
    float drawW = b->size * scale;
    float drawH = b->size * scale;
    float cx = b->x + b->size * 0.5f;
    float cy = b->y + b->size * 0.5f;

    float iconU = b->iconSrcX + (pressedVisual ? b->iconSrcSize : 0.0f);
    unsigned int iconTint = !b->active ? 0xFF808080u : 0xFFFFFFFFu;
    textureBind(iconAtlas);
    spriteDraw(iconAtlas,
               cx - drawW * 0.5f, cy - drawH * 0.5f, drawW, drawH,
               iconU, b->iconSrcY, b->iconSrcSize, b->iconSrcSize, iconTint);

    unsigned int textColor;
    float labelY;
    if (!b->active) {
        textColor = 0xFFA0A0A0u;
        labelY = b->y + 11.0f * labelScale;
    } else if (hovered) {
        textColor = 0xFFA0FFFFu;
        labelY = b->y + 12.0f * labelScale;
    } else {
        textColor = 0xFFE0E0E0u;
        labelY = b->y + 11.0f * labelScale;
    }
    int tw = fontTextWidth(font, b->label) * labelScale;
    fontDrawTextShadow(font, b->x + (b->size - tw) / 2.0f, labelY, b->label, textColor, labelScale);
}
