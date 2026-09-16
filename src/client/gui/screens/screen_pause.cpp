
#include <pspctrl.h>
#include <pspgu.h>
#include <psputility.h>
#include <cstdio>
#include <cstring>

#include "client/player/player.h"
#include "client/gui/screens/screen.h"
#include "gpu/sprite.h"
#include "platform/audio/sound.h"
#include "gpu/gui_atlas.h"

bool g_paused        = false;
int  g_pauseSel      = 0;
int  g_thirdPerson   = 0;
bool g_quitConfirm   = false;
int  g_quitConfirmSel = 1;
bool g_optionsOpen   = false;

static const char* const kPauseButtons[] = {
    "Back to game", "Options", "Save", "Quit to title",
};
static const int PAUSE_BUTTONS = (int)(sizeof(kPauseButtons) / sizeof(kPauseButtons[0]));

static const float PAUSE_V2    = VW / 20.0f;
static const float PAUSE_BTN_W = 8.0f * PAUSE_V2;
static const float PAUSE_BTN_H = 20.0f;
static const float PAUSE_PITCH = 25.0f;
static const float PAUSE_BTN_Y = 24.0f;
static const float PAUSE_LIST_W = 8.0f * PAUSE_V2;
static const float PAUSE_LIST_X = VW - PAUSE_LIST_W - PAUSE_V2;
static const float PAUSE_LIST_Y = VH / 10.0f;
static const float PAUSE_LIST_H = 8.0f * (VH / 10.0f);

static const float PAUSE_ROW_H   = 15.0f;
static const float PAUSE_ROW_Y0  = 2.0f;
static const float PAUSE_NAME_X  = 3.0f;
static const float PAUSE_NAME_Y  = 4.0f;

const char* pausePlayerName() {
    static char s_name[64];
    static bool s_read = false;
    if (!s_read) {
        s_read = true;
        s_name[0] = '\0';
        sceUtilityGetSystemParamString(PSP_SYSTEMPARAM_ID_STRING_NICKNAME, s_name, sizeof(s_name));
        if (!s_name[0]) std::snprintf(s_name, sizeof(s_name), "Player");
    }
    return s_name;
}

int pausePlayerList(const char** outNames, bool* outIsLocal, int max) {
    if (max <= 0) return 0;
    outNames[0] = pausePlayerName();
    outIsLocal[0] = true;
    return 1;
}
struct PauseScreen : Screen {
    void renderContent(MenuState& s);
    void handleInput(MenuState& s, unsigned int pressed, unsigned int held);
};

void PauseScreen::handleInput(MenuState& s, unsigned int pressed, unsigned int ) {

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

    const int selBefore = g_pauseSel;
    if ((pressed & PSP_CTRL_UP)   && g_pauseSel > 0)                  g_pauseSel--;
    if ((pressed & PSP_CTRL_DOWN) && g_pauseSel < PAUSE_BUTTONS - 1)  g_pauseSel++;
    if (g_pauseSel != selBefore) soundPlay("random.click", 1.0f, 1.0f);

    if (pressed & (PSP_CTRL_CIRCLE | PSP_CTRL_SELECT)) {
        soundPlay("random.click", 1.0f, 1.0f);
        g_paused = false;
        return;
    }
    if (pressed & PSP_CTRL_CROSS) {
        soundPlay("random.click", 1.0f, 1.0f);
        switch (g_pauseSel) {
            case 0: g_paused = false; break;
            case 1:

                g_optionsOpen = true;
                g_paused = false;
                break;
            case 2: g_saveRequested = true; g_paused = false; break;
            case 3: g_quitConfirm = true; g_quitConfirmSel = 1; break;
        }
    }
    if (pressed & PSP_CTRL_TRIANGLE) {
        soundPlay("random.click", 1.0f, 1.0f);
        g_isoMapRequested = true;
        g_paused = false;
    }
}

void guiTButton(MenuState& s, float x, float y, float w, float h, bool pressed,
                float destCorner) {
    drawNinePatch(s, GA_SS_SLOT_X + (pressed ? 0.0f : 8.0f), GA_SS_SLOT_Y,
                  8.0f, 8.0f, 2.0f, x, y, w, h, destCorner);
}

void guiTButtonLabel(MenuState& s, float x, float y, float w, float h,
                     const char* label, bool hovered, bool active, float scale) {
    if (!s.haveFont) return;

    unsigned int col = !active ? GUI_DISABLED : hovered ? 0xFFA0FFFFu : 0xFFE0E0E0u;
    float lw = fontTextWidth(&s.font, label) * scale;
    fontDrawTextShadow(&s.font, x * UI_SCALE + (w * UI_SCALE - lw) / 2.0f,
                       (y + h / 2.0f) * UI_SCALE - 4.0f * scale, label, col, scale);
}

void PauseScreen::renderContent(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    bool haveGui = s.haveGui;

    sceGuDisable(GU_DEPTH_TEST);

    if (haveGui && haveFont) {

        const char* title = "Game menu";
        float tw = fontTextWidth(&font, title) * UI_SCALE;
        fontDrawTextShadow(&font,
                           (PAUSE_V2 + PAUSE_BTN_W / 2.0f) * UI_SCALE - tw / 2.0f,
                           (PAUSE_BTN_Y - 11.0f) * UI_SCALE, title, 0xFFFFFFFFu, UI_SCALE);

        for (int i = 0; i < PAUSE_BUTTONS; i++) {
            bool hover = (g_pauseSel == i);
            float by = PAUSE_BTN_Y + i * PAUSE_PITCH;
            guiTButton(s, PAUSE_V2, by, PAUSE_BTN_W, PAUSE_BTN_H, hover);
            guiTButtonLabel(s, PAUSE_V2, by, PAUSE_BTN_W, PAUSE_BTN_H,
                            kPauseButtons[i], hover, true);
        }

        const unsigned int LIST_EDGE = 0x69000000u;
        const unsigned int LIST_FILL = 0x452D2D2Du;
        drawRect(PAUSE_LIST_X * UI_SCALE, PAUSE_LIST_Y * UI_SCALE,
                 PAUSE_LIST_W * UI_SCALE, PAUSE_LIST_H * UI_SCALE, LIST_EDGE);
        drawRect((PAUSE_LIST_X + 1.0f) * UI_SCALE, (PAUSE_LIST_Y + 1.0f) * UI_SCALE,
                 (PAUSE_LIST_W - 2.0f) * UI_SCALE, (PAUSE_LIST_H - 2.0f) * UI_SCALE, LIST_FILL);

        const char* names[16]; bool isLocal[16];
        int n = pausePlayerList(names, isLocal, 16);
        for (int i = 0; i < n; i++) {
            float ry = PAUSE_LIST_Y + PAUSE_ROW_Y0 + i * PAUSE_ROW_H;
            if (ry + PAUSE_ROW_H > PAUSE_LIST_Y + PAUSE_LIST_H) break;

            drawRect(PAUSE_LIST_X * UI_SCALE, ry * UI_SCALE,
                     PAUSE_LIST_W * UI_SCALE, PAUSE_ROW_H * UI_SCALE, LIST_FILL);
            drawRect(PAUSE_LIST_X * UI_SCALE, ry * UI_SCALE,
                     1.0f * UI_SCALE, PAUSE_ROW_H * UI_SCALE, LIST_EDGE);
            drawRect((PAUSE_LIST_X + PAUSE_LIST_W - 1.0f) * UI_SCALE, ry * UI_SCALE,
                     1.0f * UI_SCALE, PAUSE_ROW_H * UI_SCALE, LIST_EDGE);
            drawRect(PAUSE_LIST_X * UI_SCALE, (ry + PAUSE_ROW_H - 1.0f) * UI_SCALE,
                     PAUSE_LIST_W * UI_SCALE, 1.0f * UI_SCALE, LIST_EDGE);

            unsigned int col = isLocal[i] ? 0xFFFFFFFFu : 0xFF777777u;
            fontDrawTextClipped(&font, (PAUSE_LIST_X + PAUSE_NAME_X) * UI_SCALE,
                                (ry + PAUSE_NAME_Y) * UI_SCALE, names[i], col, UI_SCALE,
                                PAUSE_LIST_W - PAUSE_NAME_X * 2.0f);
        }
    }

    if (g_quitConfirm && haveGui && haveFont) {
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
            guiTButton(s, bx, by, btnW, btnH, hover);
            guiTButtonLabel(s, bx, by, btnW, btnH, clabels[i], hover, true);
        }
    }

    {
        ButtonHint h[3];
        int n = 0;
        h[n++] = menuFaceHint(true, "Select");
        if (!g_quitConfirm) h[n++] = (ButtonHint){ BTN_ICON_TRIANGLE, PSP_CTRL_TRIANGLE, "Isometric Photo" };
        h[n++] = menuFaceHint(false, g_quitConfirm ? "Cancel" : "Back to game");
        buttonHintsDraw(s, h, n);
    }

    sceGuEnable(GU_DEPTH_TEST);
}

static PauseScreen s_pauseScreen;
Screen& pauseScreen() { return s_pauseScreen; }
