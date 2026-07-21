
#include <pspctrl.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <cstdio>
#include <vector>

#include "client/player/player.h"
#include "client/gui/hud.h"
#include "gpu/sprite.h"
#include "platform/audio/sound.h"
#include "world/item/item.h"
#include "world/level/level.h"
#include "world/level/tile/entity/furnace_tile_entity.h"
#include "world/entity/item_entity.h"
#include "world/entity/local_player.h"
#include "gpu/gui_atlas.h"

extern Level g_level;

bool g_furnaceOpen = false;
static FurnaceTileEntity* s_furnace = nullptr;

static std::vector<int> s_list;
static int s_cursor = 0;
static int s_slotSel = 0;
static int s_focus = 0;

static const int   MIN_CHARGE_MS = 200;
static int         s_chargeKey    = -1;
static unsigned int s_chargeStart = 0;
static float       s_barShare     = 0.0f;

static void updateItems() {
    s_list.clear();
    for (int i = 0; i < g_inv.getContainerSize(); ++i) {
        ItemInstance* it = g_inv.getItem(i);
        if (!it || it->isNull()) continue;
        if (FurnaceTileEntity::getBurnDuration(*it) > 0 ||
            !FurnaceTileEntity::furnaceResult(it->id).isNull())
            s_list.push_back(i);
    }
    if (s_cursor >= (int)s_list.size()) s_cursor = (int)s_list.size() - 1;
    if (s_cursor < 0) s_cursor = 0;
}

static bool allowedInSlot(const ItemInstance& it, int slot) {
    if (slot == FurnaceTileEntity::SLOT_FUEL)
        return FurnaceTileEntity::getBurnDuration(it) > 0;
    if (slot == FurnaceTileEntity::SLOT_INGREDIENT)
        return !FurnaceTileEntity::furnaceResult(it.id).isNull();
    return false;
}

static void moveToFurnace(int invSlot, int n) {
    ItemInstance* src = g_inv.getItem(invSlot);
    if (!src || src->isNull() || !s_furnace) return;

    int slot = s_slotSel;
    if (!allowedInSlot(*src, slot)) {
        slot = (slot == FurnaceTileEntity::SLOT_FUEL) ? FurnaceTileEntity::SLOT_INGREDIENT
                                                      : FurnaceTileEntity::SLOT_FUEL;
        if (!allowedInSlot(*src, slot)) return;
    }

    ItemInstance& dst = s_furnace->items[slot];
    if (n > src->count) n = src->count;
    int moved = 0;
    if (dst.isNull()) {
        dst = ItemInstance(src->id, (short)n, src->data);
        moved = n;
    } else if (dst.id == src->id && dst.data == src->data) {
        int space = dst.getMaxStackSize() - dst.count;
        moved = n < space ? n : space;
        dst.count += moved;
    }
    if (moved <= 0) return;
    src->count -= moved;
    if (src->count <= 0) g_inv.clearSlot(invSlot);
    soundPlay("random.click", 1.0f, 1.0f);
    updateItems();
}

static void takeSlot(int slot, int n = 0) {
    if (!s_furnace) return;
    ItemInstance& it = s_furnace->items[slot];
    if (it.isNull()) return;
    if (n <= 0 || n > it.count) n = it.count;
    ItemInstance* copy = new ItemInstance(it);
    copy->count = n;
    if (!g_inv.add(copy)) {
        LocalPlayer* p = g_level.player;
        if (p) g_level.addEntity(new ItemEntity(&g_level, p->x, p->y, p->z, *copy));
        delete copy;
    }
    it.count -= n;
    if (it.count <= 0) it.setNull();
    soundPlay("random.click", 1.0f, 1.0f);
    updateItems();
}

int furnaceTargetSlotForCursor() {
    if (s_focus != 0 || s_list.empty()) return -1;
    ItemInstance* src = g_inv.getItem(s_list[s_cursor]);
    if (!src || src->isNull()) return -1;
    if (allowedInSlot(*src, s_slotSel)) return s_slotSel;
    int other = (s_slotSel == FurnaceTileEntity::SLOT_FUEL)
              ? FurnaceTileEntity::SLOT_INGREDIENT : FurnaceTileEntity::SLOT_FUEL;
    return allowedInSlot(*src, other) ? other : -1;
}
bool furnaceFocusIsSlots() { return s_focus != 0; }

void furnaceOpen(FurnaceTileEntity* fe) {
    s_furnace = fe;
    updateItems();
    s_cursor = 0; s_slotSel = FurnaceTileEntity::SLOT_INGREDIENT;
    s_focus = s_list.empty() ? 1 : 0;
    g_furnaceOpen = true;
    soundPlay("random.click", 1.0f, 1.0f);
}

void furnaceHandleInput(MenuState& s, unsigned int pressed, unsigned int held) {
    (void)s;
    if (!s_furnace) { g_furnaceOpen = false; return; }
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

        ItemInstance* src = (n > 0) ? g_inv.getItem(s_list[s_cursor]) : nullptr;
        int count = (src && !src->isNull()) ? src->count : 0;
        if (g_inv.isCreative()) {
            if ((pressed & PSP_CTRL_CROSS) && n > 0) moveToFurnace(s_list[s_cursor], 1);
        } else if ((held & PSP_CTRL_CROSS) && count > 0) {
            unsigned int now = sceKernelGetSystemTimeLow();
            if (s_chargeKey != s_cursor) { s_chargeKey = s_cursor; s_chargeStart = now; }
            int heldMs = (int)((now - s_chargeStart) / 1000);
            float maxHold = 700.0f + 10.0f * count;
            float share = (heldMs - MIN_CHARGE_MS) / maxHold;
            s_barShare = share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
            if (count > 1 && s_barShare >= 1.0f) {
                moveToFurnace(s_list[s_cursor], count);
                s_chargeKey = -1; s_barShare = 0.0f;
            }
        } else if (s_chargeKey == s_cursor && n > 0) {
            int heldMs = (int)((sceKernelGetSystemTimeLow() - s_chargeStart) / 1000);
            int want = (int)(count * s_barShare);
            if (want < 1 || heldMs < MIN_CHARGE_MS) want = 1;
            moveToFurnace(s_list[s_cursor], want);
            s_chargeKey = -1; s_barShare = 0.0f;
        } else {
            s_chargeKey = -1; s_barShare = 0.0f;
        }

        if (n > 0 && count > 0) {
            if (pressed & PSP_CTRL_TRIANGLE) {
                moveToFurnace(s_list[s_cursor], count);
                s_chargeKey = -1; s_barShare = 0.0f;
            } else if (pressed & PSP_CTRL_SQUARE) {
                moveToFurnace(s_list[s_cursor], (count + 1) / 2);
                s_chargeKey = -1; s_barShare = 0.0f;
            }
        }
    } else {
        s_chargeKey = -1; s_barShare = 0.0f;

        if (pressed & PSP_CTRL_UP) {
            if (s_slotSel == FurnaceTileEntity::SLOT_FUEL) s_slotSel = FurnaceTileEntity::SLOT_INGREDIENT;
        }
        if (pressed & PSP_CTRL_DOWN) {
            if (s_slotSel == FurnaceTileEntity::SLOT_INGREDIENT) s_slotSel = FurnaceTileEntity::SLOT_FUEL;
        }
        if (pressed & PSP_CTRL_RIGHT) s_slotSel = FurnaceTileEntity::SLOT_RESULT;
        if (pressed & PSP_CTRL_LEFT) {
            if (s_slotSel == FurnaceTileEntity::SLOT_RESULT) s_slotSel = FurnaceTileEntity::SLOT_INGREDIENT;
            else if (!s_list.empty()) s_focus = 0;
        }
        if (pressed & PSP_CTRL_CROSS)    takeSlot(s_slotSel, 1);
        if (pressed & PSP_CTRL_TRIANGLE) takeSlot(s_slotSel);
        if (pressed & PSP_CTRL_SQUARE) {
            int c = s_furnace ? s_furnace->items[s_slotSel].count : 0;
            if (c > 0) takeSlot(s_slotSel, (c + 1) / 2);
        }
    }

    if (before != s_focus * 1000 + s_cursor + s_slotSel * 100)
        soundPlay("random.click", 1.0f, 1.0f);

    if (pressed & PSP_CTRL_CIRCLE) {
        g_furnaceOpen = false;
        s_furnace = nullptr;
        soundPlay("random.click", 1.0f, 1.0f);
    }
}

static void drawFurnaceSlot(MenuState& s, int slot, float gx, float gy) {
    drawNinePatch(s, GA_SS_SLOT_X, GA_SS_SLOT_Y, 8, 8, 2, gx, gy, 20, 20);
    ItemInstance& it = s_furnace->items[slot];
    if (!it.isNull()) {
        drawBlockIcon(it.id, (unsigned char)(it.data < 0 ? 0 : it.data),
                      (gx + 2) * UI_SCALE, (gy + 2) * UI_SCALE, 16 * UI_SCALE, 0xFFFFFFFFu);
        if (it.count > 1 && s.haveFont)
            drawStackCount(s.font, it.count, (gx + 2) * UI_SCALE, (gy + 2) * UI_SCALE, 16 * UI_SCALE);
    }
}

void furnaceRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    #define G(v) ((v) * UI_SCALE)
    if (!s_furnace) return;

    sceGuDisable(GU_DEPTH_TEST);

    drawNinePatch(s, GA_SS_WINDOW_X, GA_SS_WINDOW_Y, 16, 16, 4, 0, 0, 240, 136);
    if (s.haveGui) {
        textureBind(&s.guiAtlas);
        spriteDraw(&s.guiAtlas, 0,      0, G(2),       G(23), GA_HDR_LEFT, WHITE);
        spriteDraw(&s.guiAtlas, G(2),   0, G(240 - 4), G(23), GA_HDR_BODY, WHITE);
        spriteDraw(&s.guiAtlas, G(238), 0, G(2),       G(23), GA_HDR_RIGHT, WHITE);
    }
    if (haveFont) {
        const char* title = "Furnace";
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
        ItemInstance* it = g_inv.getItem(s_list[i]);
        if (it && !it->isNull()) {
            drawBlockIcon(it->id, (unsigned char)(it->data < 0 ? 0 : it->data),
                          G(cx + 8), G(cy + 8), G(16), WHITE);
            if (it->count > 1 && haveFont)
                drawStackCount(font, it->count, G(cx + 8), G(cy + 8), G(16));
        }
    }
    sceGuScissor(0, 0, 480, 272);
    if (s.haveGui && n > 0) {
        float cx = paneX + (s_cursor % cols) * ItemSize;
        float cy = paneY + (s_cursor / cols - firstRow) * ItemSize;
        unsigned int tint = (s_focus == 0) ? WHITE : 0xFF808080u;
        textureBind(&s.guiAtlas);
        spriteDraw(&s.guiAtlas, G(cx - 2), G(cy - 2), G(ItemSize + 4), G(ItemSize + 5), GA_SEL_FRAME, tint);

        if (s_focus == 0 && s_barShare > 0.0f) {
            float bw = ItemSize - 8.0f;
            guiFill(G(cx + 4), G(cy + ItemSize - 7), G(bw),              G(4), 0xFF404040u);
            guiFill(G(cx + 4), G(cy + ItemSize - 7), G(bw * s_barShare), G(4), 0xFF00FF00u);
        }
    }
    if (n == 0 && haveFont)
        fontDrawTextShadow(&font, G(paneX), G(paneY + 40), "Nothing to smelt", 0xFFB0B0B0u, UI_SCALE);

    const float sx = 136.0f;
    const float ingY = 34.0f, fuelY = 82.0f, resX = 192.0f, resY = 58.0f;
    drawFurnaceSlot(s, FurnaceTileEntity::SLOT_INGREDIENT, sx, ingY);
    drawFurnaceSlot(s, FurnaceTileEntity::SLOT_FUEL,       sx, fuelY);
    drawFurnaceSlot(s, FurnaceTileEntity::SLOT_RESULT,     resX, resY);

    if (s.haveGui) {
        const float fx = 138.0f, fy = 60.0f;
        const float ax = 163.0f, ay = 61.0f;
        textureBind(&s.guiAtlas);

        spriteDraw(&s.guiAtlas, G(fx), G(fy), G(16), G(16), GA_SS_FLAME_X, GA_SS_FLAME_Y, 16, 16, WHITE);
        int lit = s_furnace->getLitProgress(16);
        if (lit > 0)
            spriteDraw(&s.guiAtlas, G(fx), G(fy + 16 - lit), G(16), G((float)lit),
                       GA_SS_FLAME_X + 16.0f, GA_SS_FLAME_Y + 16 - lit, 16, lit, WHITE);

        spriteDraw(&s.guiAtlas, G(ax), G(ay), G(22), G(15), GA_SS_ARROW_X, GA_SS_ARROW_Y, 22, 15, WHITE);
        int cook = s_furnace->getBurnProgress(22);
        if (cook > 0)
            spriteDraw(&s.guiAtlas, G(ax), G(ay), G((float)cook), G(15),
                       GA_SS_ARROW_X, GA_SS_ARROW_Y + 16.0f, cook, 15, WHITE);
    }

    if (s.haveGui) {
        float bx = (s_slotSel == FurnaceTileEntity::SLOT_RESULT) ? resX : sx;
        float by = (s_slotSel == FurnaceTileEntity::SLOT_RESULT) ? resY
                 : (s_slotSel == FurnaceTileEntity::SLOT_FUEL)   ? fuelY : ingY;
        unsigned int tint = (s_focus == 1) ? WHITE : 0xFF808080u;
        textureBind(&s.guiAtlas);
        spriteDraw(&s.guiAtlas, G(bx - 2), G(by - 2), G(24), G(25), GA_SEL_FRAME, tint);
    }

    sceGuEnable(GU_DEPTH_TEST);
    #undef G
}
