
#include <pspctrl.h>
#include <pspgu.h>
#include "gpu/gu.h"
#include <cstdio>
#include <cstring>
#include <cmath>

#include "client/gui/screens/menu.h"
#include "client/gui/screens/screen.h"
#include "world/level/levelgen/level_source.h"
#include "gpu/sprite.h"
#include "client/gui/screens/world_icons.h"

struct WorldsScreen : Screen {
    void renderContent(MenuState& s);
    void handleInput(MenuState& s, unsigned int pressed, unsigned int held);
};

void WorldsScreen::handleInput(MenuState& s, unsigned int pressed, unsigned int ) {
    WorldList& worlds = s.worlds;
    int& worldSelected = s.worldSelected;
    int& uiRow = s.uiRow;
    int& topSelected = s.topSelected;
    int& deleteSelected = s.deleteSelected;
    char (&statusMsg)[128] = s.statusMsg;
    AppScreen& screen = s.screen;

    int numEntries = worlds.count + 1;

    if (pressed & PSP_CTRL_DOWN) {
        if (uiRow == 0) uiRow = 1;
        else if (uiRow == 1 && worldSelected < worlds.count) uiRow = 2;
    }
    if (pressed & PSP_CTRL_UP) {
        if (uiRow == 2) uiRow = 1;
        else if (uiRow == 1) uiRow = 0;
    }
    if (pressed & PSP_CTRL_RIGHT) {
        if (uiRow == 0) topSelected = 1;
        else if (uiRow == 1) {
            if (worldSelected < numEntries - 1) worldSelected++;
        }
    }
    if (pressed & PSP_CTRL_LEFT) {
        if (uiRow == 0) topSelected = 0;
        else if (uiRow == 1) {
            if (worldSelected > 0) worldSelected--;
        }
    }

    if (pressed & PSP_CTRL_CIRCLE)
        screen = SCREEN_TITLE;

    if (pressed & PSP_CTRL_CROSS) {
        if (uiRow == 0) {
            if (topSelected == 0) screen = SCREEN_TITLE;
            else {
                createFormReset(s);
                screen = SCREEN_CREATE;
            }
        } else if (uiRow == 1) {
            if (worldSelected < worlds.count) {
                snprintf(statusMsg, sizeof(statusMsg), "Loading: %s", worlds.names[worldSelected]);
                screen = SCREEN_GAME;
            } else {
                createFormReset(s);
                screen = SCREEN_CREATE;
            }
        } else if (uiRow == 2) {
            screen = SCREEN_DELETE;
            deleteSelected = 1;
        }
    }
}

void WorldsScreen::renderContent(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    bool haveGui = s.haveGui;
    bool haveBg = s.haveBg;
    Texture& touchGui = s.touchGui; bool haveTouch = s.haveTouch;
    Texture& defaultWorld = s.defaultWorld; bool haveDefaultWorld = s.haveDefaultWorld;
    WorldList& worlds = s.worlds;
    int& worldSelected = s.worldSelected;
    int& uiRow = s.uiRow;
    int& topSelected = s.topSelected;
    float& listScrollX = s.listScrollX;

    if (haveGui && haveTouch && haveFont && haveDefaultWorld && haveBg) {
        sceGuDisable(GU_DEPTH_TEST);

        const float listScale = 0.88f;

        const float listText = 2.0f;
        const float itemWidthV = 120.0f * listScale;
        float targetScrollX = worldSelected * itemWidthV;

        listScrollX += (targetScrollX - listScrollX) * (1.0f - powf(0.7f, (float)guFrameSteps()));
        if (listScrollX > targetScrollX - 0.5f && listScrollX < targetScrollX + 0.5f) {
            listScrollX = targetScrollX;
        }

        int numEntries = worlds.count + 1;

        float rowY = 36.0f;

        for (int i = 0; i < numEntries; i++) {
            float xOffsetV = (i * itemWidthV) - listScrollX;
            float xCenterV = VW / 2.0f + xOffsetV;
            float xV = xCenterV - itemWidthV / 2.0f;

            float dist = (xCenterV > VW / 2.0f) ? (xCenterV - VW / 2.0f) : (VW / 2.0f - xCenterV);
            float a0 = 1.1f - dist * 0.0055f;
            if (a0 > 1.0f) a0 = 1.0f;
            if (a0 < 0.2f) a0 = 0.2f;

            bool isSelected = (i == worldSelected);
            bool isListActive = (uiRow == 1);
            bool isHovered = (isSelected && isListActive);

            unsigned int alpha = (unsigned int)(255.0f * a0);
            unsigned int colWhite = (alpha << 24) | 0xFFFFFFu;
            unsigned int colGrey  = (alpha << 24) | 0xA0A0A0u;
            unsigned int colTitle = (alpha << 24) | (isHovered ? 0xA0FFFFu : 0xFFFFFFu);

            if (isSelected) {
                float y0V = rowY - 16.0f * listScale;
                float y1V = rowY + 92.0f * listScale;
                unsigned int borderRGB = isHovered ? 0xFFFFFFu : 0x808080u;
                unsigned int borderColor = (alpha << 24) | borderRGB;

                const unsigned int ROW_FILL_ALPHA = 0xC8;
                unsigned int bgColor = ((alpha * ROW_FILL_ALPHA / 255) << 24) | 0x2D2D2Du;

                const float bw = 1.0f;
                const float cx = xV - bw, cy = y0V;
                const float cw = itemWidthV + bw * 2.0f, ch = y1V - y0V;
                drawRect(cx * UI_SCALE, cy * UI_SCALE, cw * UI_SCALE, bw * UI_SCALE, borderColor);
                drawRect(cx * UI_SCALE, (cy + ch - bw) * UI_SCALE, cw * UI_SCALE, bw * UI_SCALE, borderColor);
                drawRect(cx * UI_SCALE, cy * UI_SCALE, bw * UI_SCALE, ch * UI_SCALE, borderColor);
                drawRect((cx + cw - bw) * UI_SCALE, cy * UI_SCALE, bw * UI_SCALE, ch * UI_SCALE, borderColor);

                drawRect(xV * UI_SCALE, (y0V + bw) * UI_SCALE, itemWidthV * UI_SCALE,
                         (ch - bw * 2.0f) * UI_SCALE, bgColor);
            }

            if (i < worlds.count) {

                int iconDist = i > worldSelected ? i - worldSelected : worldSelected - i;
                Texture* img = iconDist <= 1 ? worldIcon(worlds.names[i]) : 0;
                if (!img) img = &defaultWorld;
                textureBind(img);

                float imgW = 64.0f * listScale, imgH = 45.0f * listScale;
                float imgX = xCenterV - imgW / 2.0f;
                float imgY = rowY - 8.0f * listScale;

                float srcX = 0.0f, srcY = 0.0f;
                float srcW = (float)img->realW, srcH = (float)img->realH;
                const float wantAspect = imgW / imgH;
                if (srcW > srcH * wantAspect) {
                    float w = srcH * wantAspect;
                    srcX = (srcW - w) * 0.5f; srcW = w;
                } else {
                    float h = srcW / wantAspect;
                    srcY = (srcH - h) * 0.5f; srcH = h;
                }

                spriteDraw(img, imgX * UI_SCALE, imgY * UI_SCALE, imgW * UI_SCALE, imgH * UI_SCALE,
                           srcX, srcY, srcW, srcH, colWhite);

                float xText = xCenterV - 55.0f * listScale;

                const float rowTextMaxW = (120.0f - 10.0f) * listScale * UI_SCALE / listText;

                const float TEXT_Y0    = 139.0f;
                const float SHADOW     = 2.0f;
                const float GAP_NAME   = 3.0f;
                const float GAP_DATE   = 2.0f;
                const float GAP_FOLDER = 2.0f;

                #define LINE_BOTTOM(y, str) ((y) + fontTextInkRows(&font, (str)) * listText + SHADOW)
                const char* t0 = worlds.displayNames[i];
                const char* t1 = worlds.dates[i];
                const char* t2 = worlds.names[i];
                const char* t3 = worlds.gameModes[i] == 1 ? "Creative" : "Survival";
                const float TEXT_Y1 = LINE_BOTTOM(TEXT_Y0, t0) + GAP_NAME;
                const float TEXT_Y2 = LINE_BOTTOM(TEXT_Y1, t1) + GAP_DATE;
                const float TEXT_Y3 = LINE_BOTTOM(TEXT_Y2, t2) + GAP_FOLDER;
                #undef LINE_BOTTOM

                fontDrawTextClipped(&font, xText * UI_SCALE, TEXT_Y0, t0, colTitle, listText, rowTextMaxW);

                fontDrawTextClipped(&font, xText * UI_SCALE, TEXT_Y1, t1, colGrey, listText, rowTextMaxW);

                fontDrawTextClipped(&font, xText * UI_SCALE, TEXT_Y2, t2, colGrey, listText, rowTextMaxW);

                fontDrawTextShadow(&font, xText * UI_SCALE, TEXT_Y3, t3, colGrey, listText);
            } else {

                textureBind(&touchGui);

                const float iconPx = 54.0f * listText;
                float iconX = xCenterV * UI_SCALE - iconPx / 2.0f;
                float iconY = (rowY + 4.0f * listScale) * UI_SCALE;
                float iconV = isHovered ? 86.0f : 32.0f;
                spriteDraw(&touchGui, iconX, iconY, iconPx, iconPx,
                           168.0f, iconV, 54.0f, 54.0f, colWhite);

                const char* cn = "Create new";

                float cnw = fontTextWidth(&font, cn) * listText;

                fontDrawTextShadow(&font, xCenterV * UI_SCALE - cnw / 2.0f,
                                   iconY + iconPx + 4.0f, cn, colTitle, listText);
            }
        }

        if (worlds.count > 0 && worldSelected < worlds.count) {
            bool delHovered = (uiRow == 2);
            textureBind(&touchGui);

            const float X_CELL = 26.0f;
            const float X_ZOOM = 2.0f;
            const float btnPx  = X_CELL * X_ZOOM;
            const float btnX   = floorf((VW * UI_SCALE - btnPx) / 2.0f);

            const float btnY   = VH * UI_SCALE - btnPx - 2.0f;

            const float INSET  = delHovered ? 2.0f : 0.0f;

            textureBind(&touchGui);
            spriteDraw(&touchGui, btnX + INSET, btnY + INSET,
                       btnPx - INSET * 2.0f, btnPx - INSET * 2.0f,
                       delHovered ? 158.0f : 132.0f, 0.0f, X_CELL, X_CELL, WHITE);
        }

        {

            float lb = 4.0f * MENU_PX + menuBarButtonW(s, "Back");
            float rb = VW - 4.0f * MENU_PX - menuBarButtonW(s, "Create new");
            drawMenuHeader(s, "Select world", 0.0f, VW, MENU_BAR_H, MENU_BAR_TEXT, lb, rb - lb);
        }
        {
            float bw = menuBarButtonW(s, "Back");
            menuBarButton(s, 4.0f * MENU_PX, bw, "Back", uiRow == 0 && topSelected == 0);
            float cw = menuBarButtonW(s, "Create new");
            menuBarButton(s, VW - cw - 4.0f * MENU_PX, cw, "Create new",
                          uiRow == 0 && topSelected == 1);
        }

        sceGuEnable(GU_DEPTH_TEST);
    }
}

static WorldsScreen s_worldsScreen;
Screen& worldsScreen() { return s_worldsScreen; }
