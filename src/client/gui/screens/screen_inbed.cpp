
#include <pspctrl.h>
#include <pspgu.h>

#include "client/player/player.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "gpu/sprite.h"
#include "client/gui/hud.h"
#include "platform/audio/sound.h"
#include "gpu/gui_atlas.h"

void inBedHandleInput(MenuState& s, unsigned int pressed) {
    (void)s;

    if (pressed & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_LTRIGGER)) {
        soundPlay("random.click", 1.0f, 1.0f);

        g_level.player->stopSleepInBed(true, true);
    }
}

void inBedRenderFade(MenuState& s) {
    (void)s;
    LocalPlayer* p = g_level.player;
    if (!p) return;

    float share = (float)p->sleepCounter / (float)Player::SLEEP_DURATION;
    if (share < 0.0f) share = 0.0f;
    if (share > 1.0f) share = 1.0f;
    unsigned int a = (unsigned int)(share * 220.0f);
    guiFill(0.0f, 0.0f, 480.0f, 272.0f, (a << 24));
}

void inBedRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    bool haveTouch = s.haveGui;
    LocalPlayer* p = g_level.player;
    if (!p) return;

    sceGuDisable(GU_DEPTH_TEST);

    if (haveTouch && haveFont) {
        const char* label = "Leave Bed";
        const float btnW = 100.0f, btnH = 20.0f;

        const float bx = (VW - btnW) / 2.0f, by = VH - btnH - 26.0f;

        textureBind(&s.guiAtlas);
        spriteDraw(&s.guiAtlas, bx * UI_SCALE, by * UI_SCALE, btnW * UI_SCALE, btnH * UI_SCALE,
                   GA_BTN_PAIR_X + 60.0f, GA_BTN_PAIR_Y, 60.0f, 20.0f, WHITE);
        float lw = fontTextWidth(&font, label) * UI_SCALE;
        fontDrawTextShadow(&font, bx * UI_SCALE + (btnW * UI_SCALE - lw) / 2.0f,
                           (by + 6.0f) * UI_SCALE, label, 0xFFA0FFFFu, UI_SCALE);
    }

    sceGuEnable(GU_DEPTH_TEST);
}
