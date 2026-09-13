#include "client/gui/gen_screen.h"
#include "gpu/gu.h"
#include "gpu/texture.h"
#include "gpu/sprite.h"
#include "gpu/font.h"
#include <pspgu.h>

void drawGeneratingScreen(MenuState& s, int percent, const char* status, const char* title) {
    guOrtho();
    sceGuDisable(GU_DEPTH_TEST);

    if (s.haveBg) {
        drawDirtBackground(s);
    }

    const float cx = VW / 2.0f, cy = VH / 2.0f;

    if (s.haveFont) {
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
