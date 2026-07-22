
#include <pspctrl.h>
#include <pspgu.h>
#include <cstdio>
#include <cstring>

#include "client/gui/screens/menu.h"
#include "client/renderer/tileentity/tile_entity_renderer.h"
#include "gpu/sprite.h"
#include "platform/audio/sound.h"
#include "world/level/tile/entity/sign_tile_entity.h"

SignTileEntity* g_signEditing = 0;

void signHandleInput(MenuState& s, unsigned int pressed) {
    (void)s;
    SignTileEntity* ste = g_signEditing;
    if (!ste) return;

    int sel = ste->selectedLine;
    if ((pressed & PSP_CTRL_UP) && sel > 0) {
        ste->selectedLine = sel - 1;
        soundPlay("random.click", 1.0f, 1.0f);
    }
    if ((pressed & PSP_CTRL_DOWN) && sel < SignTileEntity::NUM_LINES - 1) {
        ste->selectedLine = sel + 1;
        soundPlay("random.click", 1.0f, 1.0f);
    }
    if (pressed & PSP_CTRL_CROSS) signEditLine(ste->selectedLine);
    if (pressed & (PSP_CTRL_CIRCLE | PSP_CTRL_START)) {
        ste->selectedLine = -1;
        g_signEditing = 0;
    }
}

void signRender(MenuState& s) {
    SignTileEntity* ste = g_signEditing;
    if (!ste) return;

    sceGuDisable(GU_DEPTH_TEST);
    drawRect(0.0f, 0.0f, 480.0f, 272.0f, 0x80000000u);

    const float scale = ((272.0f / 2.0f) / 32.0f) * 0.9f;
    const float bx = 480.0f / 2.0f - 32.0f * scale;

    const float by = (272.0f - 32.0f * scale) / 2.0f;
    Texture* tex = signBoardTexture();
    if (tex) {
        textureBind(tex);
        spriteDraw(tex, bx, by, 64.0f * scale, 32.0f * scale,
                   2.0f, 2.0f, 23.0f, 12.0f, WHITE);
    }

    if (s.haveFont) {
        const float textScale = 8.0f / 11.0f;
        const float ts = scale * textScale;
        for (int i = 0; i < SignTileEntity::NUM_LINES; i++) {
            const std::string& msg = ste->messages[i];
            char buf[40];

            if (i == ste->selectedLine && msg.length() < 14)
                snprintf(buf, sizeof(buf), "> %s <", msg.c_str());
            else {
                strncpy(buf, msg.c_str(), sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = 0;
            }
            float x = 480.0f / 2.0f - fontTextWidth(&s.font, buf) * ts / 2.0f;
            float y = by + scale * (2.0f + textScale * 10.0f * i);
            fontDrawText(&s.font, x, y, buf, 0xFF000000u, ts);
        }
    }

    ButtonHint h[4];
    int n = 0;
    h[n++] = (ButtonHint){ BTN_ICON_CROSS,  PSP_CTRL_CROSS,  "Write" };
    h[n++] = (ButtonHint){ BTN_ICON_CIRCLE, PSP_CTRL_CIRCLE, "Exit" };
    int sel = ste->selectedLine;
    bool canUp   = sel > 0;
    bool canDown = sel >= 0 && sel < SignTileEntity::NUM_LINES - 1;
    if (canUp)   h[n++] = (ButtonHint){ BTN_ICON_UP,   PSP_CTRL_UP,   canDown ? "" : "Move row" };
    if (canDown) h[n++] = (ButtonHint){ BTN_ICON_DOWN, PSP_CTRL_DOWN, "Move row" };

    buttonHintsDraw(s, h, n, 272.0f - 17.0f - 1.0f);

    sceGuEnable(GU_DEPTH_TEST);
}
