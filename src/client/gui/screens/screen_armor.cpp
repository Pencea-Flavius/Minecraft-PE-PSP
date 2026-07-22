
#include <pspctrl.h>
#include <pspgu.h>
#include <cstdio>
#include <vector>

#include "client/player/player.h"
#include "client/gui/hud.h"
#include "gpu/sprite.h"
#include "platform/audio/sound.h"
#include "world/item/item.h"
#include "world/item/armor_item.h"
#include "world/level/level.h"
#include "world/entity/item_entity.h"
#include "world/entity/local_player.h"
#include "client/renderer/entity/player_model.h"
#include "gpu/gui_atlas.h"

extern Level g_level;

bool g_armorOpen = false;

static std::vector<int> s_list;
static int s_cursor = 0;
static int s_slotSel = 0;
static int s_focus = 0;

static void updateItems() {
    s_list.clear();
    for (int i = 0; i < g_level.player->inventory->getContainerSize(); ++i) {
        ItemInstance* it = g_level.player->inventory->getItem(i);
        if (!it || it->isNull()) continue;
        Item* item = it->getItem();
        if (item && item->isArmor()) s_list.push_back(i);
    }
    if (s_cursor >= (int)s_list.size()) s_cursor = (int)s_list.size() - 1;
    if (s_cursor < 0) s_cursor = 0;
}

static void giveBack(const ItemInstance& item) {
    ItemInstance* copy = new ItemInstance(item);
    if (!g_level.player->inventory->add(copy)) {
        LocalPlayer* p = g_level.player;
        if (p) g_level.addEntity(new ItemEntity(&g_level, p->x, p->y, p->z, *copy));
        delete copy;
    }
}

static void equipSelected() {
    LocalPlayer* p = g_level.player;
    if (!p || s_cursor >= (int)s_list.size()) return;
    int slotIdx = s_list[s_cursor];
    ItemInstance* inst = g_level.player->inventory->getItem(slotIdx);
    if (!inst || inst->isNull()) { updateItems(); return; }
    Item* item = inst->getItem();
    if (!item || !item->isArmor()) return;

    int wornSlot = ((ArmorItem*)item)->getSlot();
    ItemInstance old;
    bool hadOld = false;
    if (ItemInstance* o = p->getArmor(wornSlot)) { old = *o; hadOld = true; }

    p->setArmor(wornSlot, inst);
    g_level.player->inventory->clearSlot(slotIdx);
    if (hadOld) giveBack(old);

    soundPlay("random.click", 1.0f, 1.0f);
    updateItems();
}

static void takeAndClearSlot(int slot) {
    LocalPlayer* p = g_level.player;
    if (!p) return;
    ItemInstance* item = p->getArmor(slot);
    if (!item) return;
    giveBack(*item);
    p->setArmor(slot, nullptr);
    soundPlay("random.click", 1.0f, 1.0f);
    updateItems();
}

void armorOpen() {
    updateItems();
    s_cursor = 0; s_slotSel = 0;
    s_focus = s_list.empty() ? 1 : 0;
    g_armorOpen = true;
    soundPlay("random.click", 1.0f, 1.0f);
}

bool armorFocusIsWornSlot() { return s_focus != 0; }

void armorHandleInput(MenuState& s, unsigned int pressed) {
    (void)s;
    const int cols = 3;
    const int n = (int)s_list.size();
    int before = s_focus * 1000 + s_cursor + s_slotSel * 100;

    if (s_focus == 0) {
        if ((pressed & PSP_CTRL_LEFT) && s_cursor % cols > 0) s_cursor--;
        if (pressed & PSP_CTRL_RIGHT) {
            if (s_cursor % cols < cols - 1 && s_cursor + 1 < n) s_cursor++;
            else s_focus = 1;
        }
        if ((pressed & PSP_CTRL_UP)   && s_cursor >= cols)    s_cursor -= cols;
        if ((pressed & PSP_CTRL_DOWN) && s_cursor + cols < n) s_cursor += cols;
        if ((pressed & PSP_CTRL_CROSS) && n > 0) equipSelected();
    } else {
        if ((pressed & PSP_CTRL_UP)   && s_slotSel > 0) s_slotSel--;
        if ((pressed & PSP_CTRL_DOWN) && s_slotSel < 3) s_slotSel++;
        if ((pressed & PSP_CTRL_LEFT) && !s_list.empty()) s_focus = 0;
        if (pressed & PSP_CTRL_CROSS) takeAndClearSlot(s_slotSel);
    }

    if (before != s_focus * 1000 + s_cursor + s_slotSel * 100)
        soundPlay("random.click", 1.0f, 1.0f);

    if (pressed & PSP_CTRL_CIRCLE) {
        g_armorOpen = false;
        soundPlay("random.click", 1.0f, 1.0f);
    }
}

static void durabilityBar(MenuState& s, ItemInstance* it, float gx, float gy) {
    (void)s;
    Item* item = it->getItem();
    if (!item || item->maxDamage <= 0 || it->data <= 0) return;
    float frac = 1.0f - (float)it->data / (float)item->maxDamage;
    if (frac < 0) frac = 0;
    int r = (int)(255 * (1.0f - frac)), g = (int)(255 * frac);
    guiFill(gx * UI_SCALE, (gy + 14.0f) * UI_SCALE, 14.0f * UI_SCALE, 2.0f * UI_SCALE, 0xFF000000u);
    guiFill(gx * UI_SCALE, (gy + 14.0f) * UI_SCALE, 14.0f * frac * UI_SCALE, 1.0f * UI_SCALE,
            0xFF000000u | (g << 8) | r);
}

void armorRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    LocalPlayer* p = g_level.player;
    #define G(v) ((v) * UI_SCALE)

    sceGuDisable(GU_DEPTH_TEST);

    drawNinePatch(s, GA_SS_WINDOW_X, GA_SS_WINDOW_Y, 16, 16, 4, 0, 0, 240, 136);
    if (s.haveGui) {
        textureBind(&s.guiAtlas);
        spriteDraw(&s.guiAtlas, 0,      0, G(2),       G(23), GA_HDR_LEFT, WHITE);
        spriteDraw(&s.guiAtlas, G(2),   0, G(240 - 4), G(23), GA_HDR_BODY, WHITE);
        spriteDraw(&s.guiAtlas, G(238), 0, G(2),       G(23), GA_HDR_RIGHT, WHITE);
    }
    if (haveFont) {
        const char* title = "Armor";
        float tw = fontTextWidth(&font, title) * UI_SCALE;
        fontDrawTextShadow(&font, (480.0f - tw) / 2.0f, (G(23) - 8.0f * UI_SCALE) / 2.0f,
                           title, 0xFFFFFFFFu, UI_SCALE);
    }

    const float paneX = 12.0f, paneY = 32.0f;
    const float ItemSize = 32.0f;
    const int   cols = 3;
    const float paneW = cols * ItemSize, paneH = 3 * ItemSize;
    drawNinePatch(s, GA_SS_PANE_X, GA_SS_PANE_Y, 8, 8, 2, paneX - 2, paneY - 2, paneW + 4, paneH + 4);

    const int n = (int)s_list.size();

    int firstRow = s_cursor / cols - 1; if (firstRow < 0) firstRow = 0;
    sceGuScissor((int)G(paneX), (int)G(paneY), (int)G(paneW), (int)G(paneH));
    for (int i = 0; i < n; ++i) {
        int row = i / cols - firstRow;
        if (row < 0 || row > 2) continue;
        float cx = paneX + (i % cols) * ItemSize;
        float cy = paneY + row * ItemSize;
        if (s.haveGui) {
            textureBind(&s.guiAtlas);
            spriteDraw(&s.guiAtlas, G(cx), G(cy), G(ItemSize), G(ItemSize), GA_SLOT_BG, WHITE);
        }
        ItemInstance* it = g_level.player->inventory->getItem(s_list[i]);
        if (it && !it->isNull()) {
            drawBlockIcon(it->id, 0, G(cx + 8), G(cy + 8), G(16), WHITE);
            durabilityBar(s, it, cx + 9, cy + 8);
        }
    }
    sceGuScissor(0, 0, 480, 272);

    if (s.haveGui && n > 0) {
        float cx = paneX + (s_cursor % cols) * ItemSize;
        float cy = paneY + (s_cursor / cols - firstRow) * ItemSize;
        unsigned int tint = (s_focus == 0) ? WHITE : 0xFF808080u;
        textureBind(&s.guiAtlas);
        spriteDraw(&s.guiAtlas, G(cx - 2), G(cy - 2), G(ItemSize + 4), G(ItemSize + 5), GA_SEL_FRAME, tint);
    }
    if (n == 0 && haveFont)
        fontDrawTextShadow(&font, G(paneX), G(paneY + 40), "No armor", 0xFFB0B0B0u, UI_SCALE);

    const float slotX = 120.0f;
    for (int i = 0; i < 4; ++i) {
        float sy = paneY + 24.0f * i;
        drawNinePatch(s, GA_SS_SLOT_X, GA_SS_SLOT_Y, 8, 8, 2, slotX, sy, 20, 20);
        ItemInstance* worn = p ? p->getArmor(i) : nullptr;
        if (worn) {
            drawBlockIcon(worn->id, 0, G(slotX + 2), G(sy + 2), G(16), WHITE);
            durabilityBar(s, worn, slotX + 3, sy + 3);
        } else if (s.haveGui) {

            static const struct { float x, y, w, h; } kGhost[4] = {
                { GA_SLOT_HELMET }, { GA_SLOT_CHESTPLATE },
                { GA_SLOT_LEGGINGS }, { GA_SLOT_BOOTS } };
            textureBind(&s.guiAtlas);
            spriteDraw(&s.guiAtlas, G(slotX + 2), G(sy + 2), G(16), G(16),
                       kGhost[i].x, kGhost[i].y, kGhost[i].w, kGhost[i].h, 0xFFFFFFFFu);
        }
    }

    if (s.haveGui) {
        float sy = paneY + 24.0f * s_slotSel;
        unsigned int tint = (s_focus == 1) ? WHITE : 0xFF808080u;
        textureBind(&s.guiAtlas);
        spriteDraw(&s.guiAtlas, G(slotX - 2), G(sy - 2), G(24), G(25), GA_SEL_FRAME, tint);
    }

    {
        const float boxX = 152.0f, boxY = paneY - 2.0f, boxW = 80.0f, boxH = 100.0f;
        drawNinePatch(s, GA_SS_PANE_X, GA_SS_PANE_Y, 8, 8, 2, boxX, boxY, boxW, boxH);

        float scale = (boxH - 12.0f) * UI_SCALE / 32.0f;
        float cx = G(boxX + boxW / 2.0f);
        float cy = G(boxY + 6.0f) + 8.0f * scale;
        playerModelRenderPreview(cx, cy, scale);
    }

    sceGuEnable(GU_DEPTH_TEST);
    #undef G
}
