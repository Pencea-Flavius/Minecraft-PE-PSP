
#include <pspctrl.h>
#include <pspgu.h>
#include <cstdio>

#include "client/player/player.h"
#include "gpu/sprite.h"
#include "platform/audio/sound.h"
#include "gpu/gui_atlas.h"

bool g_paused        = false;
int  g_pauseSel      = 0;
bool g_muted         = false;
bool g_serverVisible = true;
bool g_thirdPerson   = false;
bool g_quitConfirm   = false;
int  g_quitConfirmSel = 1;

static const int PAUSE_SLOTS = 6;

void pauseHandleInput(MenuState& s, unsigned int pressed) {

    if (g_quitConfirm) {
        int before = g_quitConfirmSel;
        if (pressed & PSP_CTRL_LEFT)  g_quitConfirmSel = 0;
        if (pressed & PSP_CTRL_RIGHT) g_quitConfirmSel = 1;
        if (g_quitConfirmSel != before) soundPlay("random.click", 1.0f, 1.0f);
        if (pressed & PSP_CTRL_CIRCLE) { soundPlay("random.click", 1.0f, 1.0f); g_quitConfirm = false; return; }
        if (pressed & PSP_CTRL_CROSS) {
            soundPlay("random.click", 1.0f, 1.0f);
            g_quitConfirm = false;

            if (g_quitConfirmSel == 0) { quitToMenuNoSave(s); g_paused = false; }
        }
        return;
    }

    static int lastBtn = 0;

    const int selBefore = g_pauseSel;
    bool onIcons = (g_pauseSel >= 4);
    if (pressed & PSP_CTRL_LEFT) {
        if (!onIcons)               { lastBtn = g_pauseSel; g_pauseSel = 5; }
        else if (g_pauseSel == 5)   g_pauseSel = 4;

    }
    if (pressed & PSP_CTRL_RIGHT) {
        if (onIcons) {
            if (g_pauseSel == 4)    g_pauseSel = 5;
            else                    g_pauseSel = lastBtn;
        }
    }
    if (pressed & PSP_CTRL_UP) {
        if (!onIcons) { if (g_pauseSel > 0) g_pauseSel--; }
    }
    if (pressed & PSP_CTRL_DOWN) {
        if (!onIcons) { if (g_pauseSel < 3) g_pauseSel++; }
    }
    if (g_pauseSel != selBefore) soundPlay("random.click", 1.0f, 1.0f);

    if (pressed & PSP_CTRL_CIRCLE) {
        soundPlay("random.click", 1.0f, 1.0f);
        g_paused = false;
        return;
    }
    if (pressed & PSP_CTRL_CROSS) {
        soundPlay("random.click", 1.0f, 1.0f);
        switch (g_pauseSel) {
            case 0: g_paused = false; break;
            case 1: g_saveRequested = true; g_paused = false; break;
            case 2: g_quitConfirm = true; g_quitConfirmSel = 1; break;
            case 3: g_serverVisible = !g_serverVisible; break;
            case 4: g_muted = !g_muted; break;
            case 5: g_thirdPerson = !g_thirdPerson;
                    optionsSetThirdPerson(g_thirdPerson); break;
        }
    }
}

void pauseRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    bool haveTouch = s.haveGui;

    sceGuDisable(GU_DEPTH_TEST);

    drawRect(0.0f, 0.0f, 480.0f, 272.0f, 0x80000000u);

    if (haveFont) {
        const char* title = "Game menu";
        float tw = fontTextWidth(&font, title) * UI_SCALE;
        fontDrawTextShadow(&font, (VW * UI_SCALE - tw) / 2.0f, 6.0f * UI_SCALE,
                           title, 0xFFFFFFFFu, UI_SCALE);
    }

    if (haveTouch && haveFont) {
        const float btnW = 100.0f, btnH = 20.0f, gap = 4.0f;
        const float startX = (VW - btnW) / 2.0f;
        const float startY = 24.0f;
        const char* serverMsg = g_serverVisible ? "Server is visible" : "Server is invisible";
        const char* labels[4] = { "Back to game", "Save", "Quit", serverMsg };

        for (int i = 0; i < 4; ++i) {
            bool hover = (g_pauseSel == i);
            float bx = startX, by = startY + i * (btnH + gap);

            textureBind(&s.guiAtlas);

            float bu = hover ? 60.0f : 0.0f;
            spriteDraw(&s.guiAtlas, bx * UI_SCALE, by * UI_SCALE, btnW * UI_SCALE, btnH * UI_SCALE,
                       GA_BTN_PAIR_X + bu, GA_BTN_PAIR_Y, 60.0f, 20.0f, WHITE);

            float lw = fontTextWidth(&font, labels[i]) * UI_SCALE;
            unsigned int lcol = hover ? 0xFFA0FFFFu : 0xFFE0E0E0u;
            fontDrawTextShadow(&font, bx * UI_SCALE + (btnW * UI_SCALE - lw) / 2.0f,
                               (by + 6.0f) * UI_SCALE, labels[i], lcol, UI_SCALE);
        }
    }

    if (haveTouch) {
        const float SU = GA_SWITCH_X, SW = 39.0f, SH = 31.0f;
        const float iw = SW * 0.6667f * UI_SCALE, ih = SH * 0.6667f * UI_SCALE;
        struct { float sv; bool rightCell; int slot; } icons[2] = {
            { GA_SWITCH_Y +  0.0f, !g_muted,       4 },
            { GA_SWITCH_Y + 31.0f, g_thirdPerson,  5 },
        };
        textureBind(&s.guiAtlas);
        for (int i = 0; i < 2; ++i) {
            float dx = (4.0f + i * (SW * 0.6667f + 4.0f)) * UI_SCALE;
            float dy = 8.0f * UI_SCALE;
            float su = icons[i].rightCell ? SU + SW : SU;
            unsigned int tint = (g_pauseSel == icons[i].slot) ? 0xFFA0FFFFu : 0xFFFFFFFFu;
            spriteDraw(&s.guiAtlas, dx, dy, iw, ih, su, icons[i].sv, SW, SH, tint);
        }
    }

    if (g_quitConfirm && haveTouch && haveFont) {
        drawRect(0.0f, 0.0f, 480.0f, 272.0f, 0xB0000000u);
        const char* line = "Are you sure you want to exit without saving?";
        float lw = fontTextWidth(&font, line) * UI_SCALE;
        fontDrawTextShadow(&font, (VW * UI_SCALE - lw) / 2.0f, 48.0f * UI_SCALE,
                           line, 0xFFFFFFFFu, UI_SCALE);

        const float btnW = 90.0f, btnH = 20.0f, gap = 12.0f;
        const float totalW = btnW * 2 + gap;
        const float bx0 = (VW - totalW) / 2.0f, by = 66.0f;
        const char* clabels[2] = { "Quit", "Cancel" };
        for (int i = 0; i < 2; ++i) {
            bool hover = (g_quitConfirmSel == i);
            float bx = bx0 + i * (btnW + gap);
            textureBind(&s.guiAtlas);
            spriteDraw(&s.guiAtlas, bx * UI_SCALE, by * UI_SCALE, btnW * UI_SCALE, btnH * UI_SCALE,
                       GA_BTN_PAIR_X + (hover ? 60.0f : 0.0f), GA_BTN_PAIR_Y, 60.0f, 20.0f, WHITE);
            float cw = fontTextWidth(&font, clabels[i]) * UI_SCALE;
            unsigned int col = hover ? 0xFFA0FFFFu : 0xFFE0E0E0u;
            fontDrawTextShadow(&font, bx * UI_SCALE + (btnW * UI_SCALE - cw) / 2.0f,
                               (by + 6.0f) * UI_SCALE, clabels[i], col, UI_SCALE);
        }
    }

    {
        ButtonHint h[2];
        int n = 0;
        h[n++] = (ButtonHint){ BTN_ICON_CROSS,  PSP_CTRL_CROSS,  "Select" };
        h[n++] = (ButtonHint){ BTN_ICON_CIRCLE, PSP_CTRL_CIRCLE,
                               g_quitConfirm ? "Cancel" : "Back to game" };
        buttonHintsDraw(s, h, n);
    }

    sceGuEnable(GU_DEPTH_TEST);
}
