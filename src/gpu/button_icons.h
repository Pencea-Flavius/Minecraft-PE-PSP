
#ifndef MCPSP_GPU_BUTTON_ICONS_H
#define MCPSP_GPU_BUTTON_ICONS_H

#include "gpu/sprite.h"
#include "gpu/texture.h"
#include "gpu/gui_atlas.h"

enum ButtonIcon {
    BTN_ICON_L, BTN_ICON_R,
    BTN_ICON_UP, BTN_ICON_DOWN, BTN_ICON_LEFT, BTN_ICON_RIGHT,
    BTN_ICON_TRIANGLE, BTN_ICON_CIRCLE, BTN_ICON_SQUARE, BTN_ICON_CROSS,
    BTN_ICON_COUNT
};

struct ButtonIconRect { float x, y, w, h; };

inline const ButtonIconRect& buttonIconRect(ButtonIcon i) {

    static const ButtonIconRect kRects[BTN_ICON_COUNT] = {
        { GA_PSP_L }, { GA_PSP_R },
        { GA_PSP_UP }, { GA_PSP_DOWN }, { GA_PSP_LEFT }, { GA_PSP_RIGHT },
        { GA_PSP_TRIANGLE }, { GA_PSP_CIRCLE }, { GA_PSP_SQUARE }, { GA_PSP_CROSS },
    };
    return kRects[i];
}

inline void buttonIconDraw(const Texture* guiAtlas, ButtonIcon i,
                           float x, float y, float scale = 1.0f,
                           unsigned int color = 0xFFFFFFFF) {
    const ButtonIconRect& r = buttonIconRect(i);
    textureBind(guiAtlas);
    spriteDraw(guiAtlas, x, y, r.w * scale, r.h * scale, r.x, r.y, r.w, r.h, color);
}

#endif
