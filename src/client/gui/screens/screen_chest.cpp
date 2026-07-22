
#include "world/entity/local_player.h"
#include <pspctrl.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <cstdio>

#include "client/player/player.h"
#include "client/gui/hud.h"
#include "gpu/sprite.h"
#include "platform/audio/sound.h"
#include "world/item/item.h"
#include "world/level/level.h"
#include "world/level/tile/entity/chest_tile_entity.h"
#include "world/inventory/inventory.h"
#include "gpu/gui_atlas.h"

extern Level g_level;

bool g_chestOpen = false;
static ChestTileEntity* s_chest = nullptr;

static int s_pane = 0;
static int s_cursor[2] = { 0, 0 };
static float s_scroll[2] = { 0.0f, 0.0f };

static const int   MIN_CHARGE_MS = 200;
static int         s_chargeKey    = -1;
static unsigned int s_chargeStart = 0;
static float       s_barShare     = 0.0f;

static FillingContainer* paneContainer(int pane) {
    return pane == 0 ? (FillingContainer*)g_level.player->inventory : &s_chest->container;
}

bool chestCursorOnChest() { return s_pane == 1; }

static int paneSize(int pane) {
    return pane == 0 ? Inventory::SURVIVAL_SLOTS : ChestTileEntity::CONTAINER_SIZE;
}

static void moveAcross(int n) {

    if (g_level.player->inventory->isCreative()) return;
    FillingContainer* from = paneContainer(s_pane);
    FillingContainer* to   = paneContainer(1 - s_pane);
    int slot = s_cursor[s_pane];
    ItemInstance* src = from->getItem(slot);
    if (!src || src->isNull()) return;
    if (n > src->count) n = src->count;

    ItemInstance* taken = new ItemInstance(src->id, (short)n, src->data);
    short before = taken->count;
    if (!to->add(taken)) {

        n = before - taken->count;
        delete taken;
    }
    if (n <= 0) return;
    src->count -= n;
    if (src->count <= 0) from->clearSlot(slot);
    soundPlay("random.pop", 0.3f, 1.4f);
}

void chestOpen(ChestTileEntity* ce) {
    s_chest = ce;
    s_pane = 0;
    s_cursor[0] = s_cursor[1] = 0;
    s_scroll[0] = s_scroll[1] = 0.0f;
    g_chestOpen = true;
    soundPlay("random.click", 1.0f, 1.0f);
}

void chestHandleInput(MenuState& s, unsigned int pressed, unsigned int held) {
    (void)s;
    if (!s_chest) { g_chestOpen = false; return; }
    const int cols = 3;
    int& cur = s_cursor[s_pane];
    const int n = paneSize(s_pane);
    int before = s_pane * 1000 + cur;

    if (pressed & PSP_CTRL_LEFT) {
        if (cur % cols > 0) cur--;
        else if (s_pane == 1) s_pane = 0;
    }
    if (pressed & PSP_CTRL_RIGHT) {
        if (cur % cols < cols - 1) cur++;
        else if (s_pane == 0) s_pane = 1;
    }
    if ((pressed & PSP_CTRL_UP)   && cur >= cols)    cur -= cols;
    if ((pressed & PSP_CTRL_DOWN) && cur + cols < n) cur += cols;
    if (s_cursor[s_pane] >= paneSize(s_pane)) s_cursor[s_pane] = paneSize(s_pane) - 1;

    int curKey = s_pane * 1000 + s_cursor[s_pane];
    ItemInstance* src = paneContainer(s_pane)->getItem(s_cursor[s_pane]);
    int count = (src && !src->isNull()) ? src->count : 0;

    if ((pressed & PSP_CTRL_TRIANGLE) && count > 0) {
        moveAcross(count);
        s_chargeKey = -1; s_barShare = 0.0f;
    } else if ((pressed & PSP_CTRL_SQUARE) && count > 0) {
        moveAcross((count + 1) / 2);
        s_chargeKey = -1; s_barShare = 0.0f;
    }
    if ((held & PSP_CTRL_CROSS) && !g_level.player->inventory->isCreative() && count > 0) {
        unsigned int now = sceKernelGetSystemTimeLow();
        if (s_chargeKey != curKey) { s_chargeKey = curKey; s_chargeStart = now; }
        int heldMs = (int)((now - s_chargeStart) / 1000);
        float maxHold = 700.0f + 10.0f * count;
        float share = (heldMs - MIN_CHARGE_MS) / maxHold;
        s_barShare = share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
        if (count > 1 && s_barShare >= 1.0f) {
            moveAcross(count);
            s_chargeKey = -1; s_barShare = 0.0f;
        }
    } else if (s_chargeKey == curKey) {
        int heldMs = (int)((sceKernelGetSystemTimeLow() - s_chargeStart) / 1000);
        int want = (int)(count * s_barShare);
        if (want < 1 || heldMs < MIN_CHARGE_MS) want = 1;
        moveAcross(want);
        s_chargeKey = -1; s_barShare = 0.0f;
    } else {
        s_chargeKey = -1; s_barShare = 0.0f;
    }

    if (before != s_pane * 1000 + s_cursor[s_pane])
        soundPlay("random.click", 1.0f, 1.0f);

    if (pressed & PSP_CTRL_CIRCLE) {
        g_chestOpen = false;
        s_chest = nullptr;
        soundPlay("random.click", 1.0f, 1.0f);
    }
}

static void drawPane(MenuState& s, int paneIdx, float paneX) {
    Font& font = s.font; bool haveFont = s.haveFont;
    #define G(v) ((v) * UI_SCALE)
    const float paneY = 32.0f, ItemSize = 32.0f;
    const int   cols = 3, visRows = 3;
    const float paneW = cols * ItemSize, paneH = visRows * ItemSize;
    FillingContainer* c = paneContainer(paneIdx);
    const int n = paneSize(paneIdx);
    const int rows = 1 + (n - 1) / cols;

    drawNinePatch(s, GA_SS_PANE_X, GA_SS_PANE_Y, 8, 8, 2, paneX - 2, paneY - 2, paneW + 4, paneH + 4);

    {
        int curRow = s_cursor[paneIdx] / cols;
        int maxScroll = rows - visRows; if (maxScroll < 0) maxScroll = 0;
        int scrollRow = curRow - visRows / 2; if (scrollRow < 0) scrollRow = 0;
        if (scrollRow > maxScroll) scrollRow = maxScroll;
        float target = scrollRow * ItemSize;
        s_scroll[paneIdx] += (target - s_scroll[paneIdx]) * 0.35f;
    }
    float scroll = s_scroll[paneIdx];

    sceGuScissor((int)G(paneX), (int)G(paneY), (int)G(paneW), (int)G(paneH));
    if (s.haveGui) {
        textureBind(&s.guiAtlas);
        for (int i = 0; i < n; ++i) {
            float cy = paneY + (i / cols) * ItemSize - scroll;
            if (cy < paneY - ItemSize || cy > paneY + paneH) continue;
            spriteDraw(&s.guiAtlas, G(paneX + (i % cols) * ItemSize), G(cy),
                       G(ItemSize), G(ItemSize), GA_SLOT_BG, WHITE);
        }
    }
    for (int i = 0; i < n; ++i) {
        float cy = paneY + (i / cols) * ItemSize - scroll;
        if (cy < paneY - ItemSize || cy > paneY + paneH) continue;
        ItemInstance* it = c->getItem(i);
        if (!it || it->isNull()) continue;
        float cx = paneX + (i % cols) * ItemSize;
        drawBlockIcon(it->id, (unsigned char)(it->data < 0 ? 0 : it->data),
                      G(cx + 8), G(cy + 8), G(16), WHITE);
        if (it->count > 1 && haveFont)
            drawStackCount(font, it->count, G(cx + 8), G(cy + 8), G(16));

        Item* itm = (it->id > 0 && it->id < 4096) ? Item::items[it->id] : nullptr;
        if (itm && itm->maxDamage > 0 && it->data > 0) {
            float frac = 1.0f - (float)it->data / (float)itm->maxDamage;
            if (frac < 0.0f) frac = 0.0f;
            unsigned int r = (unsigned int)((1.0f - frac) * 255.0f);
            unsigned int g = (unsigned int)(frac * 255.0f);
            guiFill(G(cx + 9), G(cy + 8 + 14), G(14),        G(1), 0xFF000000u);
            guiFill(G(cx + 9), G(cy + 8 + 14), G(14) * frac, G(1), 0xFF000000u | (g << 8) | r);
        }
    }
    sceGuScissor(0, 0, 480, 272);

    if (s.haveGui) {
        int cur = s_cursor[paneIdx];
        float cx = paneX + (cur % cols) * ItemSize;
        float cy = paneY + (cur / cols) * ItemSize - scroll;
        unsigned int tint = (paneIdx == s_pane) ? WHITE : 0xFF808080u;
        textureBind(&s.guiAtlas);
        sceGuScissor(0, (int)G(paneY) - 2, 480, (int)G(paneH) + 7);
        spriteDraw(&s.guiAtlas, G(cx - 2), G(cy - 2), G(ItemSize + 4), G(ItemSize + 5), GA_SEL_FRAME, tint);
        sceGuScissor(0, 0, 480, 272);

        if (paneIdx == s_pane && s_barShare > 0.0f) {
            float bw = ItemSize - 8.0f;
            guiFill(G(cx + 4), G(cy + ItemSize - 7), G(bw),              G(4), 0xFF404040u);
            guiFill(G(cx + 4), G(cy + ItemSize - 7), G(bw * s_barShare), G(4), 0xFF00FF00u);
        }
    }
    #undef G
}

void chestRender(MenuState& s) {
    #define G(v) ((v) * UI_SCALE)
    if (!s_chest) return;
    Font& font = s.font; bool haveFont = s.haveFont;

    sceGuDisable(GU_DEPTH_TEST);
    drawNinePatch(s, GA_SS_WINDOW_X, GA_SS_WINDOW_Y, 16, 16, 4, 0, 0, 240, 136);

    if (s.haveGui) {
        textureBind(&s.guiAtlas);
        spriteDraw(&s.guiAtlas, 0,      0, G(2),       G(23), GA_HDR_LEFT, WHITE);
        spriteDraw(&s.guiAtlas, G(2),   0, G(240 - 4), G(23), GA_HDR_BODY, WHITE);
        spriteDraw(&s.guiAtlas, G(238), 0, G(2),       G(23), GA_HDR_RIGHT, WHITE);
    }
    if (haveFont) {
        const float ty = (G(23) - 8.0f * UI_SCALE) / 2.0f;
        const float paneW = 3 * 32.0f;
        struct { const char* t; float x; int pane; } hdr[2] = {
            { "Inventory", 20.0f,  0 }, { "Chest", 124.0f, 1 } };
        for (int i = 0; i < 2; ++i) {
            float tw = fontTextWidth(&font, hdr[i].t) * UI_SCALE;
            fontDrawTextShadow(&font, G(hdr[i].x) + (G(paneW) - tw) / 2.0f, ty, hdr[i].t,
                               hdr[i].pane == s_pane ? 0xFFFFFFFFu : 0xFFB0B0B0u, UI_SCALE);
        }
    }

    drawPane(s, 0, 20.0f);
    drawPane(s, 1, 124.0f);

    sceGuEnable(GU_DEPTH_TEST);
    #undef G
}
