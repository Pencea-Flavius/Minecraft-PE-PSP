
#include <pspctrl.h>
#include <pspgu.h>

#include "client/gui/screens/menu.h"
#include "client/gui/screens/screen.h"
#include "gpu/sprite.h"
#include "client/gui/screens/skin_page.h"
#include "client/renderer/entity/player_model.h"

#include <cmath>
#include <ctime>
#include <pspkernel.h>

#include <cstdio>
#include <cstring>
#include "platform/path.h"

static char s_splash[128];
static bool s_splashPicked = false;

static void pickSplash(unsigned seed) {
    s_splashPicked = true;
    FILE* f = fopen(assetPath("data/splashes.txt"), "r");
    if (!f) return;
    char line[128];
    unsigned n = 0;
    while (fgets(line, sizeof(line), f)) {
        char* e = line + strlen(line);
        while (e > line && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ')) *--e = '\0';
        if (line[0] == '\0' || line[0] == '#') continue;
        seed = seed * 1664525u + 1013904223u;
        if (seed % ++n == 0) strcpy(s_splash, line);
    }
    fclose(f);
}

static const float BTN_W = 96.0f;
static const float BTN_H = 24.0f;
static const float BTN_X = (VW - BTN_W) / 2.0f;
static const float PLAY_Y = 58.0f;
static const float SET_Y  = PLAY_Y + BTN_H + 5.0f;

static const float SKIN_CX   = (BTN_X + BTN_W + VW) / 2.0f;
static const float SKIN_TOP  = 44.0f;
static const float SKIN_W    = 52.0f;

static const float SKIN_H    = 58.0f;
static const float SKIN_NAME_Y = SKIN_TOP - 10.0f;
static const float SKINBTN_W = 50.0f;
static const float SKINBTN_H = 16.0f;
static const float SKINBTN_X = SKIN_CX - SKINBTN_W / 2.0f;
static const float SKINBTN_Y = SKIN_TOP + SKIN_H + 2.0f;

enum { BTN_PLAY = 0, BTN_SETTINGS = 1, BTN_SKINS = 2, numButtons = 3 };

static float s_skinRot = 0.0f;

static const int SKIN_STICK_DZ = 48;

static const unsigned int kTitleSeed[3] = {
    0x0251B8B0u, 0x1360B0C0u, 0x00000275u
};
#define TITLE_SEED_LEN 14
static int s_seedHold = 0;

struct TitleScreen : Screen {
    void renderContent(MenuState& s);
    void handleInput(MenuState& s, unsigned int pressed, unsigned int held);
};

void TitleScreen::handleInput(MenuState& s, unsigned int pressed, unsigned int held) {

    static const unsigned int SEED_MASK = PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_UP;
    s_seedHold = ((held & SEED_MASK) == SEED_MASK) ? (s_seedHold + 1) : 0;

    int& selected = s.selected;
    AppScreen& screen = s.screen;
    char (&statusMsg)[128] = s.statusMsg;
    int& optFocus = s.optFocus;
    int& optTabHighlight = s.optTabHighlight;
    int& optItemHighlight = s.optItemHighlight;
    int& optCategory = s.optCategory;

    if (skinPageIsOpen()) { skinPageInput(s, pressed); return; }

    bool stickTurning = false;
    if (selected == BTN_SKINS) {
        SceCtrlData pad;
        if (sceCtrlPeekBufferPositive(&pad, 1) > 0) {
            int dx = (int)pad.Lx - 128;
            if (dx > SKIN_STICK_DZ || dx < -SKIN_STICK_DZ) {

                s_skinRot -= (float)dx * (4.0f / 127.0f);
                if (s_skinRot >= 360.0f) s_skinRot -= 360.0f;
                if (s_skinRot < 0.0f)    s_skinRot += 360.0f;
                stickTurning = true;
            }
        }
    }

    if (selected < 0) selected = BTN_PLAY;
    if (pressed & PSP_CTRL_UP)   selected = (selected == BTN_SETTINGS) ? BTN_PLAY : selected;
    if (pressed & PSP_CTRL_DOWN) selected = (selected == BTN_PLAY) ? BTN_SETTINGS : selected;
    if (!stickTurning) {
        if (pressed & PSP_CTRL_RIGHT) selected = BTN_SKINS;
        if (pressed & PSP_CTRL_LEFT)  selected = (selected == BTN_SKINS) ? BTN_PLAY : selected;
    }

    if (pressed & PSP_CTRL_CROSS) {
        if (selected == BTN_PLAY) {
            screen = SCREEN_WORLDS;
            statusMsg[0] = '\0';
        } else if (selected == BTN_SETTINGS) {
            optFocus = 1;
            optTabHighlight = optCategory;
            optItemHighlight = 0;
            screen = SCREEN_OPTIONS;
            statusMsg[0] = '\0';
        } else {
            skinPageOpen();
        }
    }
}

void TitleScreen::renderContent(MenuState& s) {
    if (skinPageIsOpen()) { skinPageRender(s); return; }
    Font& font = s.font; bool haveFont = s.haveFont;
    bool haveGui = s.haveGui;
    Texture& logo = s.logo; bool haveLogo = s.haveLogo;
    int& selected = s.selected;

    float logoYV = 6.0f;

    float logoWV = (float)logo.realW / UI_SCALE;
    float logoHV = (float)logo.realH / UI_SCALE;
    float logoXV = (VW - logoWV) / 2.0f;
    if (haveLogo) {
        textureBind(&logo);
        sceGuDisable(GU_DEPTH_TEST);
        spriteDraw(&logo, logoXV * UI_SCALE, logoYV * UI_SCALE,
                  logoWV * UI_SCALE, logoHV * UI_SCALE,
                  0, 0, (float)logo.realW, (float)logo.realH, WHITE);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveFont) {

        if (!s_splashPicked)
            pickSplash((unsigned)time(0) * 2654435761u + sceKernelGetSystemTimeLow());
        const char* splash = s_splash;

        float t = (float)sceKernelGetSystemTimeLow() * 1e-6f;
        float scale = powf(sinf(t * 3.14f * 2.3f), 4.0f) * 0.06f + 1.3f;

        float len = (float)fontTextWidth(&font, splash);
        float fit = (VW * 0.3125f) / (len * 1.3f);
        if (fit > 1.0f) fit = 1.0f;

        sceGuDisable(GU_DEPTH_TEST);
        fontDrawTransformed(&font, (logoXV + logoWV) * 0.71f * UI_SCALE,
                            (logoYV + logoHV - 15.0f) * UI_SCALE,
                            splash, 0xFF00FFFFu ,
                            -20.0f, scale * fit * UI_SCALE, true);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveGui && haveFont) {
        sceGuDisable(GU_DEPTH_TEST);
        guiTButton(s, BTN_X, PLAY_Y, BTN_W, BTN_H, selected == BTN_PLAY);
        guiTButtonLabel(s, BTN_X, PLAY_Y, BTN_W, BTN_H, "Play",
                        selected == BTN_PLAY, true);
        guiTButton(s, BTN_X, SET_Y, BTN_W, BTN_H, selected == BTN_SETTINGS);
        guiTButtonLabel(s, BTN_X, SET_Y, BTN_W, BTN_H, "Settings",
                        selected == BTN_SETTINGS, true);
        guiTButton(s, SKINBTN_X, SKINBTN_Y, SKINBTN_W, SKINBTN_H, selected == BTN_SKINS);
        guiTButtonLabel(s, SKINBTN_X, SKINBTN_Y, SKINBTN_W, SKINBTN_H, "Skins",
                        selected == BTN_SKINS, true);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveFont) {
        sceGuDisable(GU_DEPTH_TEST);
        drawNameTag(s, SKIN_CX * UI_SCALE, SKIN_NAME_Y * UI_SCALE, pausePlayerName());
        sceGuEnable(GU_DEPTH_TEST);
    }

    playerModelRenderWornPreview(SKIN_CX * UI_SCALE - SKIN_W * UI_SCALE / 2.0f,
                                 SKIN_TOP * UI_SCALE, SKIN_W * UI_SCALE,
                                 SKIN_H * UI_SCALE, s_skinRot);

    if (haveFont) {
        sceGuDisable(GU_DEPTH_TEST);

        const char* copyright = "\xffMojang AB";
        float cw = fontTextWidth(&font, copyright) * UI_SCALE;
        fontDrawTextShadow(&font, 480.0f - cw - 4.0f, 272.0f - 9.0f * UI_SCALE,
                           copyright, WHITE, UI_SCALE);
        if (s_seedHold > 30) {
            char line[TITLE_SEED_LEN + 1];
            for (int i = 0; i < TITLE_SEED_LEN; i++) {
                unsigned int v = (kTitleSeed[i / 6] >> (5 * (i % 6))) & 31u;
                line[i] = v ? (char)(0x40u + v) : ' ';
            }
            line[TITLE_SEED_LEN] = '\0';
            float lw = fontTextWidth(&font, line) * UI_SCALE;
            fontDrawTextShadow(&font, (480.0f - lw) * 0.5f, 272.0f - 20.0f * UI_SCALE,
                               line, 0xFF80FFFFu, UI_SCALE);
        }
        sceGuEnable(GU_DEPTH_TEST);
    }
}

static TitleScreen s_titleScreen;
Screen& titleScreen() { return s_titleScreen; }
