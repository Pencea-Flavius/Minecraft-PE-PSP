
#include <pspctrl.h>
#include <pspgu.h>

#include "client/player/player.h"
#include "gpu/sprite.h"
#include "platform/audio/sound.h"
#include "platform/time.h"
#include "gpu/gui_atlas.h"

bool g_deadScreen = false;
static int   s_deadSel  = 0;
static float s_openTime = 0.0f;

static const float DEAD_WAIT = 1.5f;

void deadScreenOpen() {
    g_deadScreen = true;
    s_deadSel = 0;
    s_openTime = nowSeconds();
}

void deadHandleInput(MenuState& s, unsigned int pressed) {
    int before = s_deadSel;
    if (pressed & PSP_CTRL_UP)   s_deadSel = 0;
    if (pressed & PSP_CTRL_DOWN) s_deadSel = 1;
    if (s_deadSel != before) soundPlay("random.click", 1.0f, 1.0f);

    if (nowSeconds() - s_openTime < DEAD_WAIT) return;

    if (pressed & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE)) {
        soundPlay("random.click", 1.0f, 1.0f);
        if (s_deadSel == 0) {
            playerRespawn();
            g_deadScreen = false;
        } else {
            g_saveRequested = true;
            g_quitAfterSave = true;
            g_deadScreen = false;
        }
    }
    (void)s;
}

void deadRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    bool haveTouch = s.haveGui;

    sceGuDisable(GU_DEPTH_TEST);

    {
        struct ColorVertex { unsigned int color; float x, y, z; };
        ColorVertex* v = (ColorVertex*)sceGuGetMemory(4 * sizeof(ColorVertex));
        const unsigned int top = 0x60000050u, bot = 0xA0303080u;
        v[0].color = top; v[0].x = 0.0f;   v[0].y = 0.0f;   v[0].z = 0.0f;
        v[1].color = top; v[1].x = 480.0f; v[1].y = 0.0f;   v[1].z = 0.0f;
        v[2].color = bot; v[2].x = 0.0f;   v[2].y = 272.0f; v[2].z = 0.0f;
        v[3].color = bot; v[3].x = 480.0f; v[3].y = 272.0f; v[3].z = 0.0f;
        sceGuDisable(GU_TEXTURE_2D);
        sceGuDrawArray(GU_TRIANGLE_STRIP, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, v);
        sceGuEnable(GU_TEXTURE_2D);
    }

    if (haveFont) {
        const char* title = "You died!";
        float tw = fontTextWidth(&font, title) * UI_SCALE * 2.0f;
        fontDrawTextShadow(&font, (VW * UI_SCALE - tw) / 2.0f, 20.0f * UI_SCALE,
                           title, 0xFFFFFFFFu, UI_SCALE * 2.0f);
    }

    if (haveTouch && haveFont) {
        bool active = (nowSeconds() - s_openTime >= DEAD_WAIT);
        const float btnW = 100.0f, btnH = 20.0f, gap = 6.0f;
        const float startX = (VW - btnW) / 2.0f;
        const float startY = 60.0f;
        const char* labels[2] = { "Respawn", "Main menu" };

        for (int i = 0; i < 2; ++i) {
            bool hover = active && (s_deadSel == i);
            float bx = startX, by = startY + i * (btnH + gap);

            textureBind(&s.guiAtlas);
            spriteDraw(&s.guiAtlas, bx * UI_SCALE, by * UI_SCALE, btnW * UI_SCALE, btnH * UI_SCALE,
                       GA_BTN_PAIR_X + (hover ? 60.0f : 0.0f), GA_BTN_PAIR_Y, 60.0f, 20.0f,
                       active ? WHITE : 0xFF808080u);

            float lw = fontTextWidth(&font, labels[i]) * UI_SCALE;
            unsigned int lcol = !active ? 0xFF707070u : (hover ? 0xFFA0FFFFu : 0xFFE0E0E0u);
            fontDrawTextShadow(&font, bx * UI_SCALE + (btnW * UI_SCALE - lw) / 2.0f,
                               (by + 6.0f) * UI_SCALE, labels[i], lcol, UI_SCALE);
        }
    }

    sceGuEnable(GU_DEPTH_TEST);
}
