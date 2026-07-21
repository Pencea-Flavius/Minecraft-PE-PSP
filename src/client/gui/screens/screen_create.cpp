
#include <pspctrl.h>
#include <pspgu.h>
#include <cstdio>

#include "client/gui/screens/menu.h"
#include "gpu/sprite.h"

void createHandleInput(MenuState& s, unsigned int pressed) {
    int& createSelected = s.createSelected;
    int& newWorldGamemode = s.newWorldGamemode;
    char (&newWorldName)[64] = s.newWorldName;
    char (&newWorldSeed)[64] = s.newWorldSeed;
    WorldList& worlds = s.worlds;
    int& worldSelected = s.worldSelected;
    int& uiRow = s.uiRow;
    char (&statusMsg)[128] = s.statusMsg;
    AppScreen& screen = s.screen;

    if (pressed & PSP_CTRL_DOWN) {
        if (createSelected == 0) createSelected = 1;
        else if (createSelected < 4) createSelected++;
    }
    if (pressed & PSP_CTRL_UP) {
        if (createSelected > 1) createSelected--;
        else if (createSelected == 1) createSelected = 0;
    }
    if (pressed & PSP_CTRL_RIGHT) {
        if (createSelected == 1) createSelected = 0;
    }
    if (pressed & PSP_CTRL_LEFT) {
        if (createSelected == 0) createSelected = 1;
    }
    if (pressed & PSP_CTRL_CIRCLE) {
        screen = SCREEN_WORLDS;
    }
    if (pressed & PSP_CTRL_CROSS) {
        if (createSelected == 0) {
            screen = SCREEN_WORLDS;
        } else if (createSelected == 1) {
            startOsk(1, "Enter World Name:", newWorldName);
        } else if (createSelected == 2) {
            startOsk(2, "Enter World Seed:", newWorldSeed);
        } else if (createSelected == 3) {
            newWorldGamemode = (newWorldGamemode == 0) ? 1 : 0;
        } else if (createSelected == 4) {
            char created[64];
            long seed = worldSeedFromString(newWorldSeed);
            if (worldListCreate(&worlds, newWorldName, created, newWorldGamemode, seed)) {
                snprintf(statusMsg, sizeof(statusMsg), "Loading: %s", created);
                worldSelected = worlds.count - 1;
                screen = SCREEN_GAME;
            } else {
                screen = SCREEN_WORLDS;
            }
            uiRow = 1;
        }
    }
}

void createRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    Texture& dirtBg = s.dirtBg; bool haveBg = s.haveBg;
    Texture& touchGui = s.touchGui; bool haveTouch = s.haveTouch;
    int& createSelected = s.createSelected;
    int& newWorldGamemode = s.newWorldGamemode;
    char (&newWorldName)[64] = s.newWorldName;
    char (&newWorldSeed)[64] = s.newWorldSeed;

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

        float hw = fontTextWidth(&font, "Create World") * UI_SCALE;
        fontDrawTextShadow(&font, (hW * UI_SCALE - hw) / 2.0f, (barBtnH - 8.0f) / 2.0f * UI_SCALE, "Create World", 0xFFE0E0E0u, UI_SCALE);

        textureBind(&touchGui);
        bool backHovered = (createSelected == 0);
        {
            float scale = backHovered ? 0.95f : 1.0f;
            float drawW = btnW * scale, drawH = barBtnH * scale;
            float drawX = hW + (btnW - drawW) / 2.0f, drawY = (barBtnH - drawH) / 2.0f;
            float backU = backHovered ? 184.0f : 150.0f;
            spriteDraw(&touchGui, drawX * UI_SCALE, drawY * UI_SCALE, drawW * UI_SCALE, drawH * UI_SCALE,
                       backU, 0.0f, 34.0f, 26.0f, WHITE);
        }

        float tbW = 200.0f;
        float tbH = 13.0f;
        float startX = (VW - tbW) / 2.0f;
        float textYOff = (tbH - 8.0f) / 2.0f;

        float nameY = barBtnH + 10.0f;
        float seedY = nameY + tbH + 11.0f;

        fontDrawTextShadow(&font, startX * UI_SCALE, (nameY - 8.0f) * UI_SCALE, "World name:", 0xFFCCCCCCu, UI_SCALE);
        drawRect(startX * UI_SCALE, nameY * UI_SCALE, tbW * UI_SCALE, tbH * UI_SCALE, 0xFF000000u);
        unsigned int borderCol = (createSelected == 1) ? 0xFFFFFFFFu : 0xFFA0A0A0u;
        drawRect(startX * UI_SCALE, nameY * UI_SCALE, tbW * UI_SCALE, 1.0f * UI_SCALE, borderCol);
        drawRect(startX * UI_SCALE, (nameY + tbH - 1.0f) * UI_SCALE, tbW * UI_SCALE, 1.0f * UI_SCALE, borderCol);
        drawRect(startX * UI_SCALE, nameY * UI_SCALE, 1.0f * UI_SCALE, tbH * UI_SCALE, borderCol);
        drawRect((startX + tbW - 1.0f) * UI_SCALE, nameY * UI_SCALE, 1.0f * UI_SCALE, tbH * UI_SCALE, borderCol);

        const char* nText = newWorldName[0] ? newWorldName : "New world";
        unsigned int nColor = newWorldName[0] ? 0xFFFFFFFFu : 0xFF5E5E5Eu;
        fontDrawTextClipped(&font, (startX + 4.0f) * UI_SCALE, (nameY + textYOff) * UI_SCALE, nText, nColor, UI_SCALE, tbW - 8.0f);

        fontDrawTextShadow(&font, startX * UI_SCALE, (seedY - 8.0f) * UI_SCALE, "World seed:", 0xFFCCCCCCu, UI_SCALE);
        drawRect(startX * UI_SCALE, seedY * UI_SCALE, tbW * UI_SCALE, tbH * UI_SCALE, 0xFF000000u);
        borderCol = (createSelected == 2) ? 0xFFFFFFFFu : 0xFFA0A0A0u;
        drawRect(startX * UI_SCALE, seedY * UI_SCALE, tbW * UI_SCALE, 1.0f * UI_SCALE, borderCol);
        drawRect(startX * UI_SCALE, (seedY + tbH - 1.0f) * UI_SCALE, tbW * UI_SCALE, 1.0f * UI_SCALE, borderCol);
        drawRect(startX * UI_SCALE, seedY * UI_SCALE, 1.0f * UI_SCALE, tbH * UI_SCALE, borderCol);
        drawRect((startX + tbW - 1.0f) * UI_SCALE, seedY * UI_SCALE, 1.0f * UI_SCALE, tbH * UI_SCALE, borderCol);
        fontDrawTextClipped(&font, (startX + 4.0f) * UI_SCALE, (seedY + textYOff) * UI_SCALE, newWorldSeed, 0xFFFFFFFFu, UI_SCALE, tbW - 8.0f);

        float gmW = 100.0f;
        float gmH = 19.0f;
        float gmY = seedY + tbH + 11.0f;
        float gmX = (VW - gmW) / 2.0f;

        textureBind(&touchGui);
        bool gmHovered = (createSelected == 3);
        spriteDraw(&touchGui, gmX * UI_SCALE, gmY * UI_SCALE, gmW * UI_SCALE, gmH * UI_SCALE, gmHovered ? 66.0f : 0.0f, 0.0f, 66.0f, 26.0f, WHITE);

        const char* gmLabel = (newWorldGamemode == 0) ? "Survival" : "Creative";
        float gw = fontTextWidth(&font, gmLabel) * UI_SCALE;
        unsigned int gmCol = gmHovered ? 0xFFA0FFFFu : 0xFFE0E0E0u;
        fontDrawTextShadow(&font, gmX * UI_SCALE + (gmW * UI_SCALE - gw) / 2.0f, (gmY + (gmH - 8.0f) / 2.0f) * UI_SCALE, gmLabel, gmCol, UI_SCALE);

        const char* desc = (newWorldGamemode == 0) ? "Mobs, health and gather resources" : "Unlimited resources and flying";
        float dw = fontTextWidth(&font, desc) * UI_SCALE;
        float descY = gmY + gmH + 5.0f;
        fontDrawTextShadow(&font, (VW * UI_SCALE - dw) / 2.0f, descY * UI_SCALE, desc, 0xFFCCCCCCu, UI_SCALE);

        float cW = 80.0f;
        float cH = 19.0f;
        float cY = descY + 8.0f;
        float cX = (VW - cW) / 2.0f;

        textureBind(&touchGui);
        bool cHovered = (createSelected == 4);
        spriteDraw(&touchGui, cX * UI_SCALE, cY * UI_SCALE, cW * UI_SCALE, cH * UI_SCALE, cHovered ? 66.0f : 0.0f, 0.0f, 66.0f, 26.0f, WHITE);
        float clw = fontTextWidth(&font, "Create") * UI_SCALE;
        unsigned int cCol = cHovered ? 0xFFA0FFFFu : 0xFFE0E0E0u;
        fontDrawTextShadow(&font, cX * UI_SCALE + (cW * UI_SCALE - clw) / 2.0f, (cY + (cH - 8.0f) / 2.0f) * UI_SCALE, "Create", cCol, UI_SCALE);

        sceGuEnable(GU_DEPTH_TEST);
    }
}
