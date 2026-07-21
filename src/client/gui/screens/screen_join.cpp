
#include <pspctrl.h>
#include <pspgu.h>

#include "client/gui/screens/menu.h"
#include "gpu/sprite.h"

void joinHandleInput(MenuState& s, unsigned int pressed) {
    int& joinListSelected = s.joinListSelected;
    int& joinIpSelected = s.joinIpSelected;
    AppScreen& screen = s.screen;

    if (pressed & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT))
        joinListSelected = 1 - joinListSelected;
    if (pressed & PSP_CTRL_CIRCLE) {
        screen = SCREEN_TITLE;
    }
    if (pressed & PSP_CTRL_CROSS) {
        if (joinListSelected == 0) {
            screen = SCREEN_TITLE;
        } else {
            joinIpSelected = 1;
            screen = SCREEN_JOIN_IP;
        }
    }
}

void joinRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    Texture& dirtBg = s.dirtBg; bool haveBg = s.haveBg;
    Texture& touchGui = s.touchGui; bool haveTouch = s.haveTouch;
    int& joinListSelected = s.joinListSelected;

    if (haveBg) {
        textureBind(&dirtBg);
        sceGuDisable(GU_DEPTH_TEST);
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        float tileScale = (float)dirtBg.realW / (32.0f * UI_SCALE);
        float bgW = (VW * UI_SCALE) * tileScale;
        float bgH = (VH * UI_SCALE) * tileScale;
        spriteDraw(&dirtBg, 0.0f, 0.0f, VW * UI_SCALE, VH * UI_SCALE,
                   0.0f, 0.0f, bgW, bgH, DIRT_TINT);
        sceGuTexWrap(GU_CLAMP, GU_CLAMP);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveTouch && haveFont) {
        sceGuDisable(GU_DEPTH_TEST);
        float barBtnH = 26.0f;
        float sideBtnW = 66.0f;
        float headerX = sideBtnW;
        float headerW = VW - sideBtnW * 2.0f;

        textureBind(&touchGui);
        spriteDraw(&touchGui, headerX * UI_SCALE, 0.0f, 2.0f * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE,
                   150.0f, 26.0f, 2.0f, 25.0f, WHITE);
        spriteDraw(&touchGui, (headerX + 2.0f) * UI_SCALE, 0.0f, (headerW - 4.0f) * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE,
                   153.0f, 26.0f, 8.0f, 25.0f, WHITE);
        spriteDraw(&touchGui, (headerX + headerW - 2.0f) * UI_SCALE, 0.0f, 2.0f * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE,
                   162.0f, 26.0f, 2.0f, 25.0f, WHITE);
        spriteDraw(&touchGui, headerX * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE, headerW * UI_SCALE, 3.0f * UI_SCALE,
                   153.0f, 52.0f, 8.0f, 3.0f, WHITE);

        const char* headerMsg = "WiFi is disabled";
        float hw = fontTextWidth(&font, headerMsg) * UI_SCALE;
        fontDrawTextShadow(&font, (headerX + headerW / 2.0f) * UI_SCALE - hw / 2.0f,
                           (barBtnH - 8.0f) / 2.0f * UI_SCALE, headerMsg, 0xFFE0E0E0u, UI_SCALE);

        const char* sideLabels[2] = { "Back", "Join By IP" };
        for (int i = 0; i < 2; ++i) {
            bool hovered = (joinListSelected == i);
            float bx = (i == 0) ? 0.0f : (VW - sideBtnW);
            textureBind(&touchGui);
            spriteDraw(&touchGui, bx * UI_SCALE, 0.0f, sideBtnW * UI_SCALE, barBtnH * UI_SCALE,
                       hovered ? 66.0f : 0.0f, 0.0f, 66.0f, 26.0f, WHITE);
            float lw = fontTextWidth(&font, sideLabels[i]) * UI_SCALE;
            unsigned int lcol = hovered ? 0xFFA0FFFFu : 0xFFE0E0E0u;
            fontDrawTextShadow(&font, bx * UI_SCALE + (sideBtnW * UI_SCALE - lw) / 2.0f,
                               (barBtnH - 8.0f) / 2.0f * UI_SCALE, sideLabels[i], lcol, UI_SCALE);
        }

        sceGuEnable(GU_DEPTH_TEST);
    }
}
