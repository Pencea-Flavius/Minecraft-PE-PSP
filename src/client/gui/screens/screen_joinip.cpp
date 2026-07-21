
#include <pspctrl.h>
#include <pspgu.h>
#include <cstdio>

#include "client/gui/screens/menu.h"
#include "gpu/sprite.h"

void joinIpHandleInput(MenuState& s, unsigned int pressed) {
    int& joinIpSelected = s.joinIpSelected;
    char (&joinIp)[64] = s.joinIp;
    char (&statusMsg)[128] = s.statusMsg;
    AppScreen& screen = s.screen;

    if (pressed & PSP_CTRL_DOWN) {
        if (joinIpSelected == 0) joinIpSelected = 1;
        else if (joinIpSelected == 1) joinIpSelected = 2;
    }
    if (pressed & PSP_CTRL_UP) {
        if (joinIpSelected == 2) joinIpSelected = 1;
        else if (joinIpSelected == 1) joinIpSelected = 0;
    }
    if (pressed & PSP_CTRL_RIGHT) {
        if (joinIpSelected == 1) joinIpSelected = 0;
    }
    if (pressed & PSP_CTRL_LEFT) {
        if (joinIpSelected == 0) joinIpSelected = 1;
    }
    if (pressed & PSP_CTRL_CIRCLE) {
        screen = SCREEN_TITLE;
    }
    if (pressed & PSP_CTRL_CROSS) {
        if (joinIpSelected == 0) {
            screen = SCREEN_TITLE;
        } else if (joinIpSelected == 1) {
            startOsk(3, "Enter Server IP:", joinIp, 15);
        } else if (joinIpSelected == 2) {
            if (joinIp[0] != '\0') {

                snprintf(statusMsg, sizeof(statusMsg), "Joining: %s", joinIp);
                screen = SCREEN_TITLE;
            }
        }
    }
}

void joinIpRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    Texture& dirtBg = s.dirtBg; bool haveBg = s.haveBg;
    Texture& touchGui = s.touchGui; bool haveTouch = s.haveTouch;
    int& joinIpSelected = s.joinIpSelected;
    char (&joinIp)[64] = s.joinIp;

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
        textureBind(&touchGui);
        float barBtnH = 26.0f;
        float btnW = 34.0f;

        float hW = VW - btnW;
        spriteDraw(&touchGui, 0.0f, 0.0f, 2.0f * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE,
                   150.0f, 26.0f, 2.0f, 25.0f, WHITE);
        spriteDraw(&touchGui, 2.0f * UI_SCALE, 0.0f, (hW - 4.0f) * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE,
                   153.0f, 26.0f, 8.0f, 25.0f, WHITE);
        spriteDraw(&touchGui, (hW - 2.0f) * UI_SCALE, 0.0f, 2.0f * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE,
                   162.0f, 26.0f, 2.0f, 25.0f, WHITE);
        spriteDraw(&touchGui, 0.0f, (barBtnH - 1.0f) * UI_SCALE, hW * UI_SCALE, 3.0f * UI_SCALE,
                   153.0f, 52.0f, 8.0f, 3.0f, WHITE);

        const char* headerMsg = "Join on server";
        float hw = fontTextWidth(&font, headerMsg) * UI_SCALE;
        fontDrawTextShadow(&font, (hW * UI_SCALE - hw) / 2.0f, (barBtnH - 8.0f) / 2.0f * UI_SCALE, headerMsg, 0xFFE0E0E0u, UI_SCALE);

        textureBind(&touchGui);
        bool backHovered = (joinIpSelected == 0);
        {
            float scale = backHovered ? 0.95f : 1.0f;
            float drawW = btnW * scale, drawH = barBtnH * scale;
            float drawX = hW + (btnW - drawW) / 2.0f, drawY = (barBtnH - drawH) / 2.0f;
            float backU = backHovered ? 184.0f : 150.0f;
            spriteDraw(&touchGui, drawX * UI_SCALE, drawY * UI_SCALE, drawW * UI_SCALE, drawH * UI_SCALE,
                       backU, 0.0f, 34.0f, 26.0f, WHITE);
        }

        float jbW = 100.0f, jbH = 19.0f;
        float jbY = VH * 2.0f / 3.0f;
        float jbX = (VW - jbW) / 2.0f;
        bool jbActive = joinIp[0] != '\0';
        bool jbHovered = (joinIpSelected == 2);

        textureBind(&touchGui);

        unsigned int jbTint = !jbActive ? 0xFF808080u : WHITE;
        spriteDraw(&touchGui, jbX * UI_SCALE, jbY * UI_SCALE, jbW * UI_SCALE, jbH * UI_SCALE,
                   jbHovered ? 66.0f : 0.0f, 0.0f, 66.0f, 26.0f, jbTint);
        float jlw = fontTextWidth(&font, "Join Game") * UI_SCALE;
        unsigned int jbCol = !jbActive ? 0xFFA0A0A0u : (jbHovered ? 0xFFA0FFFFu : 0xFFE0E0E0u);
        fontDrawTextShadow(&font, jbX * UI_SCALE + (jbW * UI_SCALE - jlw) / 2.0f,
                           (jbY + (jbH - 8.0f) / 2.0f) * UI_SCALE, "Join Game", jbCol, UI_SCALE);

        float tbW = jbW + 20.0f;
        float tbH = 16.0f;
        float tbX = (VW - tbW) / 2.0f;
        float tbY = jbY - tbH - 6.0f;

        drawRect(tbX * UI_SCALE, tbY * UI_SCALE, tbW * UI_SCALE, tbH * UI_SCALE, 0xFF000000u);
        unsigned int tbBorderCol = (joinIpSelected == 1) ? 0xFFFFFFFFu : 0xFFA0A0A0u;
        drawRect(tbX * UI_SCALE, tbY * UI_SCALE, tbW * UI_SCALE, 1.0f * UI_SCALE, tbBorderCol);
        drawRect(tbX * UI_SCALE, (tbY + tbH - 1.0f) * UI_SCALE, tbW * UI_SCALE, 1.0f * UI_SCALE, tbBorderCol);
        drawRect(tbX * UI_SCALE, tbY * UI_SCALE, 1.0f * UI_SCALE, tbH * UI_SCALE, tbBorderCol);
        drawRect((tbX + tbW - 1.0f) * UI_SCALE, tbY * UI_SCALE, 1.0f * UI_SCALE, tbH * UI_SCALE, tbBorderCol);

        const char* ipText = joinIp[0] ? joinIp : "Server IP";
        unsigned int ipColor = joinIp[0] ? 0xFFFFFFFFu : 0xFF5E5E5Eu;
        fontDrawTextClipped(&font, (tbX + 2.0f) * UI_SCALE, (tbY + (tbH - 8.0f) / 2.0f) * UI_SCALE, ipText, ipColor, UI_SCALE, tbW - 4.0f);

        sceGuEnable(GU_DEPTH_TEST);
    }
}
