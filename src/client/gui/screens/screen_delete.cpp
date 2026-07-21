
#include <pspctrl.h>
#include <pspgu.h>
#include <cstdio>

#include "client/gui/screens/menu.h"
#include "gpu/sprite.h"

void deleteHandleInput(MenuState& s, unsigned int pressed) {
    int& deleteSelected = s.deleteSelected;
    WorldList& worlds = s.worlds;
    int& worldSelected = s.worldSelected;
    int& uiRow = s.uiRow;
    char (&statusMsg)[128] = s.statusMsg;
    AppScreen& screen = s.screen;

    if (pressed & PSP_CTRL_RIGHT) {
        if (deleteSelected == 0) deleteSelected = 1;
    }
    if (pressed & PSP_CTRL_LEFT) {
        if (deleteSelected == 1) deleteSelected = 0;
    }
    if (pressed & PSP_CTRL_CIRCLE) {
        screen = SCREEN_WORLDS;
    }
    if (pressed & PSP_CTRL_CROSS) {
        if (deleteSelected == 0) {
            if (worlds.count > 0 && worldSelected < worlds.count) {
                snprintf(statusMsg, sizeof(statusMsg), "Deleted: %s", worlds.names[worldSelected]);
                worldListDelete(&worlds, worldSelected);
                if (worldSelected >= worlds.count && worldSelected > 0) worldSelected--;
            }
            screen = SCREEN_WORLDS;
            uiRow = 1;
        } else {
            screen = SCREEN_WORLDS;
        }
    }
}

void deleteRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    Texture& dirtBg = s.dirtBg; bool haveBg = s.haveBg;
    Texture& touchGui = s.touchGui; bool haveTouch = s.haveTouch;
    WorldList& worlds = s.worlds;
    int& worldSelected = s.worldSelected;
    int& deleteSelected = s.deleteSelected;

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

    if (haveFont) {
        const char* t1 = "Are you sure you want to delete this world?";
        char t2[128];
        snprintf(t2, sizeof(t2), "'%s' will be lost forever!", worlds.names[worldSelected]);

        float t1W = fontTextWidth(&font, t1) * UI_SCALE;
        float t2W = fontTextWidth(&font, t2) * UI_SCALE;

        sceGuDisable(GU_DEPTH_TEST);
        fontDrawTextShadow(&font, (VW * UI_SCALE - t1W) / 2.0f, 35.0f * UI_SCALE, t1, 0xFFE0E0E0u, UI_SCALE);
        fontDrawTextShadow(&font, (VW * UI_SCALE - t2W) / 2.0f, 55.0f * UI_SCALE, t2, 0xFFE0E0E0u, UI_SCALE);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveTouch && haveFont) {
        float btnW = 80.0f;
        float btnH = 26.0f;
        float gap = 24.0f;

        float btnY = 90.0f;
        float totalW = btnW * 2.0f + gap;
        float startX = (VW - totalW) / 2.0f;

        const char* btnLabels[2] = {"Delete", "Cancel"};

        for (int i = 0; i < 2; ++i) {
            bool isHovered = (deleteSelected == i);
            float btnX = startX + i * (btnW + gap);

            float drawW = btnW;
            float drawH = btnH;
            float drawX = btnX;
            float drawY = btnY;

            textureBind(&touchGui);

            float btnU = isHovered ? 66.0f : 0.0f;

            spriteDraw(&touchGui, drawX * UI_SCALE, drawY * UI_SCALE, drawW * UI_SCALE, drawH * UI_SCALE,
                       btnU, 0.0f, 66.0f, 26.0f, WHITE);

            float lw = fontTextWidth(&font, btnLabels[i]) * UI_SCALE;
            unsigned int lcol = isHovered ? 0xFFA0FFFFu : 0xFFE0E0E0u;
            sceGuDisable(GU_DEPTH_TEST);
            fontDrawTextShadow(&font, (drawX * UI_SCALE) + (drawW * UI_SCALE - lw) / 2.0f, (drawY + 8.0f) * UI_SCALE, btnLabels[i], lcol, UI_SCALE);
            sceGuEnable(GU_DEPTH_TEST);
        }
    }
}
