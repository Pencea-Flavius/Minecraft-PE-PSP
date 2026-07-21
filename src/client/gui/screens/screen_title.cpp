
#include <pspctrl.h>
#include <pspgu.h>

#include "client/gui/screens/menu.h"
#include "gpu/sprite.h"
#include "gpu/widgets.h"

static const float btnSizeV = 75.0f;
static const float yBaseV   = 2.0f + VH / 3.0f;
static const float spacingV = (VW - 3.0f * btnSizeV) / 4.0f;
static PocketButton buttons[3] = {
    { (spacingV + 0 * (btnSizeV + spacingV)) * UI_SCALE, yBaseV * UI_SCALE, btnSizeV * UI_SCALE, 0.0f, 176.0f, 75.0f, "Join Game",  true },
    { (spacingV + 1 * (btnSizeV + spacingV)) * UI_SCALE, yBaseV * UI_SCALE, btnSizeV * UI_SCALE, 0.0f, 101.0f, 75.0f, "Start Game", true },
    { (spacingV + 2 * (btnSizeV + spacingV)) * UI_SCALE, yBaseV * UI_SCALE, btnSizeV * UI_SCALE, 0.0f,  26.0f, 75.0f, "Options",    true },
};
static const int numButtons = 3;

void titleHandleInput(MenuState& s, unsigned int pressed) {

    int& selected = s.selected;
    AppScreen& screen = s.screen;
    char (&statusMsg)[128] = s.statusMsg;
    int& joinListSelected = s.joinListSelected;
    int& optFocus = s.optFocus;
    int& optTabHighlight = s.optTabHighlight;
    int& optItemHighlight = s.optItemHighlight;
    int& optEditingRow = s.optEditingRow;
    int& optCategory = s.optCategory;

    if (pressed & PSP_CTRL_RIGHT)
        selected = (selected < 0) ? 1 : (selected + 1) % numButtons;
    if (pressed & PSP_CTRL_LEFT)
        selected = (selected < 0) ? 1 : (selected + numButtons - 1) % numButtons;

    if ((pressed & PSP_CTRL_CROSS) && selected >= 0) {
        if (selected == 1) {
            screen = SCREEN_WORLDS;
            statusMsg[0] = '\0';
        } else if (selected == 0) {
            joinListSelected = 0;
            screen = SCREEN_JOIN;
            statusMsg[0] = '\0';
        } else {
            optFocus = 1;
            optTabHighlight = optCategory;
            optItemHighlight = 0;
            optEditingRow = -1;
            screen = SCREEN_OPTIONS;
            statusMsg[0] = '\0';
        }
    }
}

void titleRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    Texture& guiAtlas = s.guiAtlas; bool haveGui = s.haveGui;
    Texture& logo = s.logo; bool haveLogo = s.haveLogo;
    Texture& dirtBg = s.dirtBg; bool haveBg = s.haveBg;
    Texture& touchGui = s.touchGui; bool haveTouch = s.haveTouch;
    int& selected = s.selected;

    if (haveBg) {
        textureBind(&dirtBg);
        sceGuDisable(GU_DEPTH_TEST);
        spriteDrawTiled(&dirtBg, 0, 0, 480, 272, 32.0f * UI_SCALE, DIRT_TINT);
        sceGuEnable(GU_DEPTH_TEST);
    }

    float logoHV = 0.0f, logoYV = 4.0f;
    if (haveLogo) {
        float wh = 0.5f * ((VW / 2.0f < logo.realW / 2.0f) ? VW / 2.0f : logo.realW / 2.0f);
        float scale = 2.0f * wh / logo.realW;
        float logoWV = scale * logo.realW;
        logoHV = scale * logo.realH;
        textureBind(&logo);
        sceGuDisable(GU_DEPTH_TEST);
        spriteDraw(&logo, (VW / 2.0f - wh) * UI_SCALE, logoYV * UI_SCALE,
                  logoWV * UI_SCALE, logoHV * UI_SCALE,
                  0, 0, (float)logo.realW, (float)logo.realH, WHITE);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveFont) {
        const char* version = "v0.6.1 alpha";
        float vw = fontTextWidth(&font, version) * UI_SCALE;
        float vy = (logoYV + logoHV + 2.0f) * UI_SCALE;
        sceGuDisable(GU_DEPTH_TEST);
        fontDrawTextShadow(&font, (480.0f - vw) / 2.0f, vy, version, 0xFFCCCCCCu, UI_SCALE);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveGui && haveTouch && haveFont) {
        sceGuDisable(GU_DEPTH_TEST);
        for (int i = 0; i < numButtons; i++)
            pocketButtonDraw(&font, &guiAtlas, &touchGui, &buttons[i], i == selected, UI_SCALE);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveFont) {
        sceGuDisable(GU_DEPTH_TEST);
        const char* copyright = "\xffMojang AB";
        float cw = fontTextWidth(&font, copyright) * UI_SCALE;
        fontDrawTextShadow(&font, 480.0f - cw - 4.0f, 272.0f - 9.0f * UI_SCALE, copyright, WHITE, UI_SCALE);
        sceGuEnable(GU_DEPTH_TEST);
    }
}
