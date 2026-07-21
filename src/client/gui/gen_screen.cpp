#include "client/gui/gen_screen.h"
#include "gpu/gu.h"
#include "gpu/texture.h"
#include "gpu/sprite.h"
#include "gpu/font.h"
#include <pspgu.h>

void drawGeneratingScreen(MenuState& s, int percent, const char* status) {
    guOrtho();
    sceGuDisable(GU_DEPTH_TEST);

    if (s.haveBg) {
        textureBind(&s.dirtBg);
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        float tileScale = (float)s.dirtBg.realW / (32.0f * UI_SCALE);
        float bgW = (VW * UI_SCALE) * tileScale;
        float bgH = (VH * UI_SCALE) * tileScale;
        spriteDraw(&s.dirtBg, 0.0f, 0.0f, VW * UI_SCALE, VH * UI_SCALE,
                   0.0f, 0.0f, bgW, bgH, DIRT_TINT);
        sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    }

    const float cx = VW / 2.0f, cy = VH / 2.0f;

    if (s.haveFont) {
        const char* title = "Generating world";
        float tw = fontTextWidth(&s.font, title) * UI_SCALE;
        fontDrawTextShadow(&s.font, (VW * UI_SCALE - tw) / 2.0f, (cy - 4.0f - 16.0f) * UI_SCALE,
                            title, 0xFFFFFFFFu, UI_SCALE);

        float sw = fontTextWidth(&s.font, status) * UI_SCALE;
        fontDrawTextShadow(&s.font, (VW * UI_SCALE - sw) / 2.0f, (cy - 4.0f + 8.0f) * UI_SCALE,
                            status, 0xFFFFFFFFu, UI_SCALE);
    }

    const float barW = 100.0f, barH = 2.0f;
    float barX = cx - barW / 2.0f, barY = cy + 16.0f;
    drawRect(barX * UI_SCALE, barY * UI_SCALE, barW * UI_SCALE, barH * UI_SCALE, 0xFF808080u);
    if (percent > 0)
        drawRect(barX * UI_SCALE, barY * UI_SCALE, (float)percent * UI_SCALE, barH * UI_SCALE, 0xFF80FF80u);

    sceGuEnable(GU_DEPTH_TEST);
}
