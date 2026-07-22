
#include <pspctrl.h>
#include <pspgu.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

#include "client/player/player.h"
#include "client/gui/hud.h"
#include "gpu/sprite.h"
#include "platform/audio/sound.h"
#include "world/item/crafting/recipes.h"
#include "world/item/item.h"
#include "world/level/level.h"
#include "world/entity/item_entity.h"
#include "world/entity/local_player.h"
#include "gpu/gui_atlas.h"

extern Level g_level;

bool g_craftOpen = false;

static const unsigned int rgbActive         = 0xFFF0F0F0u;
static const unsigned int rgbInactive       = 0xC0635558u;
static const unsigned int rgbInactiveShadow = 0xC0AAAAAAu;

struct ReqItem {
    ItemInstance item;
    int has;
    bool enough() const { return has >= item.count; }
};

struct CraftItem {
    ItemInstance item;
    Recipe*      recipe;
    std::string  sortText;
    int          inventoryCount;
    ReqItem      needed[4];
    int          nNeeded;
    bool         canCraft;
};

static std::vector<CraftItem> s_items;
static std::vector<int>       s_cat[4];
static int  s_curCat = 0;
static int  s_cursor = 0;
static int  s_focus  = 1;
static float s_scrollY = 0.0f;
static int  s_pressAnim = 0;

static const int kCatBits[4] = { ItemCategory::Structures, ItemCategory::Tools,
                                 ItemCategory::FoodArmor,  ItemCategory::Decorations };

static bool isStonecutterItem(const ItemInstance& ins) {
    if (ins.id >= 256) return false;
    switch (ins.id) {
        case BLOCK_LAPIS_BLOCK: case BLOCK_FURNACE: case BLOCK_STONECUTTER:
            return false;
        case BLOCK_SLAB:
            return ins.data != 2 ;
        case BLOCK_STONE: case BLOCK_COBBLESTONE: case BLOCK_MOSSY_COBBLE:
        case BLOCK_SAND: case BLOCK_GRAVEL: case BLOCK_SANDSTONE:
        case BLOCK_BRICKS: case BLOCK_STONE_BRICKS: case BLOCK_NETHER_BRICK:
        case BLOCK_QUARTZ_BLOCK: case BLOCK_DOUBLE_SLAB: case BLOCK_OBSIDIAN:
        case BLOCK_STAIRS_COBBLESTONE: case BLOCK_STAIRS_BRICK:
        case BLOCK_STAIRS_STONE_BRICK: case BLOCK_STAIRS_NETHER_BRICK:
        case BLOCK_STAIRS_SANDSTONE: case BLOCK_STAIRS_QUARTZ:
            return true;
        default:
            return false;
    }
}

static CraftItem* currentItem() {
    if (s_cursor < 0 || s_cursor >= (int)s_cat[s_curCat].size()) return nullptr;
    return &s_items[s_cat[s_curCat][s_cursor]];
}

static void recheckRecipes() {
    ItemPack ip;
    for (int i = 0; i < g_level.player->inventory->getContainerSize(); ++i) {
        ItemInstance* it = g_level.player->inventory->getItem(i);
        if (it && !it->isNull())
            ip.add(ItemPack::getIdForItemInstance(it), it->count);
    }

    for (unsigned int c = 0; c < s_items.size(); ++c) {
        CraftItem& ci = s_items[c];
        ci.nNeeded = 0;
        ci.canCraft = true;
        ci.inventoryCount = ip.getCount(ItemPack::getIdForItemInstance(&ci.item));

        std::vector<ItemInstance> pack = ci.recipe->getItemPack().getItemInstances();
        for (unsigned int j = 0; j < pack.size() && ci.nNeeded < 4; ++j) {
            ItemInstance& jtem = pack[j];
            int has;
            if (!Recipe::isAnyAuxValue(&jtem) && jtem.data == Recipe::ANY_AUX_VALUE) {

                has = 0;
                for (short aux = 0; aux < 16; ++aux) {
                    ItemInstance t(jtem.id, 1, aux);
                    has += ip.getCount(ItemPack::getIdForItemInstance(&t));
                }
            } else {
                has = ip.getCount(ItemPack::getIdForItemInstance(&jtem));
            }
            ReqItem& req = ci.needed[ci.nNeeded++];
            req.item = jtem;
            req.has  = has;
            if (!req.enough()) ci.canCraft = false;
        }
    }
}

static void craftSelectedItem() {
    CraftItem* ci = currentItem();
    if (!ci || !ci->canCraft) return;

    ItemInstance result = ci->item;
    for (int i = 0; i < ci->nNeeded; ++i) {
        ItemInstance toRemove = ci->needed[i].item;
        if (toRemove.id == BLOCK_SANDSTONE && toRemove.data == Recipe::ANY_AUX_VALUE) {
            toRemove.data = 0;
            toRemove.count = (short)g_level.player->inventory->removeResource(toRemove, true);
            toRemove.data = Recipe::ANY_AUX_VALUE;
        }
        if (toRemove.count > 0) g_level.player->inventory->removeResource(toRemove);
    }

    ItemInstance* res = new ItemInstance(result.id, result.count, result.data);
    if (!g_level.player->inventory->add(res)) {
        LocalPlayer* p = g_level.player;
        if (p) g_level.addEntity(new ItemEntity(&g_level, p->x, p->y, p->z, *res));
        delete res;
    }
    soundPlay("random.click", 1.0f, 1.0f);
    s_pressAnim = 8;
    recheckRecipes();
}

static int s_craftSize   = Recipe::SIZE_2X2;
static int s_filterMode  = CRAFT_WORKBENCH;
static int numCategories() { return (s_filterMode == CRAFT_STONECUTTER) ? 1 : 4; }

bool craftHasCategories() { return numCategories() > 1; }

void craftOpen(int craftingSize, int filterMode) {
    s_craftSize = craftingSize;
    s_filterMode = filterMode;
    s_items.clear();
    for (int i = 0; i < 4; ++i) s_cat[i].clear();

    const std::vector<Recipe*>& rs = Recipes::getInstance()->getRecipes();
    for (unsigned int i = 0; i < rs.size(); ++i) {
        Recipe* r = rs[i];
        if (r->getCraftingSize() > s_craftSize) continue;
        ItemInstance res = r->getResultItem();
        if (s_filterMode == CRAFT_STONECUTTER ? !isStonecutterItem(res)
                                              :  isStonecutterItem(res)) continue;
        Item* item = res.getItem();
        if (!item || item->category < 0) continue;

        CraftItem ci;
        ci.item = res;
        ci.recipe = r;
        ci.inventoryCount = 0;
        ci.nNeeded = 0;
        ci.canCraft = false;

        const char* name = getBlockName(res.id, (unsigned char)res.data);
        if (res.id == BLOCK_WOOL)          ci.sortText = std::string("Wool ") + name;
        else if (res.id == ITEM_BONEMEAL)  ci.sortText = std::string("ZDye ") + name;
        else                               ci.sortText = name;

        s_items.push_back(ci);
        int idx = (int)s_items.size() - 1;
        for (int c = 0; c < 4; ++c)
            if (item->category & kCatBits[c]) s_cat[c].push_back(idx);
    }

    for (int c = 0; c < 4; ++c) {
        std::vector<int>& v = s_cat[c];
        std::stable_sort(v.begin(), v.end(),
            [](int a, int b) { return s_items[a].sortText < s_items[b].sortText; });
    }

    recheckRecipes();
    s_curCat = 0; s_cursor = 0; s_focus = 1; s_scrollY = 0.0f;
    g_craftOpen = true;
    soundPlay("random.click", 1.0f, 1.0f);
}

void craftHandleInput(MenuState& s, unsigned int pressed) {
    (void)s;

    if (g_level.player && g_level.player->hurtTime > 0) { g_craftOpen = false; return; }

    int before = s_focus * 1000 + s_curCat * 100 + s_cursor;

    s_focus = 1;
    if ((pressed & PSP_CTRL_LTRIGGER) && s_curCat > 0)                   { s_curCat--; s_cursor = 0; s_scrollY = 0; }
    if ((pressed & PSP_CTRL_RTRIGGER) && s_curCat < numCategories() - 1) { s_curCat++; s_cursor = 0; s_scrollY = 0; }
    const int rows = (int)s_cat[s_curCat].size();
    if ((pressed & PSP_CTRL_UP)   && s_cursor > 0)        s_cursor--;
    if ((pressed & PSP_CTRL_DOWN) && s_cursor + 1 < rows) s_cursor++;
    if (pressed & PSP_CTRL_CROSS) craftSelectedItem();

    if (before != s_focus * 1000 + s_curCat * 100 + s_cursor)
        soundPlay("random.click", 1.0f, 1.0f);

    if (pressed & PSP_CTRL_CIRCLE) {
        g_craftOpen = false;
        soundPlay("random.click", 1.0f, 1.0f);
    }
}

void craftRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    #define G(v) ((v) * UI_SCALE)

    sceGuDisable(GU_DEPTH_TEST);

    drawNinePatch(s, GA_SS_WINDOW_X, GA_SS_WINDOW_Y, 16, 16, 4, 0, 0, 240, 136);

    const float catH = (136.0f - 16.0f) / 4.0f;
    for (int i = 0; i < numCategories(); ++i) {
        float bx = 6, by = 6 + i * (1 + catH);
        bool sel = (s_curCat == i);
        int icon = (s_filterMode == CRAFT_STONECUTTER) ? 5 : i;

        drawNinePatch(s, GA_SS_SLOT_X + (sel ? 0.0f : 8.0f), GA_SS_SLOT_Y, 8, 8, 2, bx, by, catH, catH);

        if (s.haveGui) {
            unsigned int tint = WHITE;
            textureBind(&s.guiAtlas);
            spriteDraw(&s.guiAtlas, G(bx + 3), G(by + 3), G(catH - 6), G(catH - 6),
                       GA_SS_CRAFTICONS_X + 32.0f * (icon / 2), GA_SS_CRAFTICONS_Y + (icon & 1) * 32.0f, 32, 32, tint);
        }
    }

    const float paneX = catH + 12.0f;
    const float paneY = 8.0f;
    const float paneW = 240.0f - paneX - 100.0f;
    const float paneH = 136.0f - 16.0f;
    drawNinePatch(s, GA_SS_PANE_X, GA_SS_PANE_Y, 8, 8, 2, paneX - 2, paneY - 2, paneW + 4, paneH + 4);

    const float RowH = 22.0f;
    const int   n = (int)s_cat[s_curCat].size();
    const int   visRows = (int)(paneH / RowH);

    {
        int maxScroll = n - visRows; if (maxScroll < 0) maxScroll = 0;
        int scrollRow = s_cursor - visRows / 2; if (scrollRow < 0) scrollRow = 0;
        if (scrollRow > maxScroll) scrollRow = maxScroll;
        float target = scrollRow * RowH;
        s_scrollY += (target - s_scrollY) * 0.35f;
        if (s_scrollY < 0.3f && target == 0.0f) s_scrollY = 0.0f;
    }

    sceGuScissor((int)G(paneX), (int)G(paneY), (int)G(paneW), (int)G(paneH));
    for (int i = 0; i < n; ++i) {
        float ry = paneY + i * RowH - s_scrollY;
        if (ry < paneY - RowH || ry > paneY + paneH) continue;
        CraftItem& ci = s_items[s_cat[s_curCat][i]];
        bool sel = (i == s_cursor);

        drawNinePatch(s, GA_SS_ROW_X + (sel ? 8.0f : 0.0f), GA_SS_ROW_Y, 8, 8, 2, paneX, ry, paneW, RowH);

        if (haveFont) {
            const char* name = getBlockName(ci.item.id, (unsigned char)(ci.item.data < 0 ? 0 : ci.item.data));
            if (ci.canCraft)
                fontDrawTextClipped(&font, G(paneX + 2), G(ry + 7), name, rgbActive, UI_SCALE, paneW - 22.0f);
            else
                fontDrawTextClipped(&font, G(paneX + 2), G(ry + 7), name, rgbInactive, UI_SCALE, paneW - 22.0f);
        }

        short aux = ci.item.data < 0 ? 0 : ci.item.data;
        unsigned int tint = ci.canCraft ? WHITE : 0xFF808080u;
        drawBlockIcon(ci.item.id, (unsigned char)aux, G(paneX + paneW - 18.0f), G(ry + 3), G(16), tint);

        if (haveFont) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", ci.inventoryCount);
            float ts = UI_SCALE * 2.0f / 3.0f;
            float tw = fontTextWidth(&font, buf) * ts;
            fontDrawTextShadow(&font, G(paneX + paneW - 2.0f) - tw, G(ry + RowH - 7.0f), buf,
                               ci.canCraft ? rgbActive : rgbInactive, ts);
        }
    }

    guiFillGradient(G(paneX), G(paneY), G(paneW), G(14), 0xBB000000u, 0x00000000u);
    guiFillGradient(G(paneX), G(paneY + paneH - 14.0f), G(paneW), G(14), 0x00000000u, 0xBB000000u);
    sceGuScissor(0, 0, 480, 272);

    if (n > visRows) {
        const float sbX = paneX + paneW + 2.0f, sbW = 2.0f;
        float contentH = n * RowH;
        float thumbH = paneH * (paneH / contentH); if (thumbH < 8.0f) thumbH = 8.0f;
        float maxScrollPx = contentH - paneH;
        float frac = maxScrollPx > 0.0f ? s_scrollY / maxScrollPx : 0.0f;
        if (frac < 0.0f) frac = 0.0f; else if (frac > 1.0f) frac = 1.0f;
        float thumbY = paneY + frac * (paneH - thumbH);
        guiFill(G(sbX), G(paneY), G(sbW), G(paneH), 0x66000000u);
        guiFill(G(sbX), G(thumbY), G(sbW), G(thumbH), 0xFFC0C0C0u);
    }

    const float craftW = 88.0f, craftH = 62.0f;
    const float craftX = 240.0f - 100.0f + (100.0f - craftW) / 2.0f - 1.0f;
    const float craftY = 20.0f;
    CraftItem* cur = currentItem();

    drawNinePatch(s, GA_SS_CRAFTBTN_X + (s_pressAnim > 0 ? 8.0f : 0.0f), GA_SS_CRAFTBTN_Y, 8, 67, 2, craftX, craftY, craftW, craftH);
    if (s_pressAnim > 0) s_pressAnim--;

    if (cur && haveFont) {

        const float slotW = craftW / 2.0f, slotH = craftH / 2.0f;
        const float slotBx = craftX + slotW / 2.0f - 8.0f;
        const float slotBy = craftY + slotH / 2.0f - 9.0f;
        for (int i = 0; i < cur->nNeeded; ++i) {
            ReqItem& req = cur->needed[i];
            float xx = slotBx + slotW * (i % 2);
            float yy = slotBy + slotH * (i / 2);
            short aux = req.item.data < 0 ? 0 : req.item.data;
            drawBlockIcon(req.item.id, (unsigned char)aux, G(xx), G(yy), G(16),
                          req.enough() ? WHITE : 0xFF707070u);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d/%d", req.has, (int)req.item.count);
            float ts = UI_SCALE * 2.0f / 3.0f;
            fontDrawTextShadow(&font, G(xx), G(yy + 17), buf,
                               req.enough() ? rgbActive : rgbInactive, ts);
        }

        const char* desc = getBlockDescription(cur->item.id, (unsigned char)(cur->item.data < 0 ? 0 : cur->item.data));

        const float descScale = UI_SCALE * 2.0f / 3.0f;
        fontDrawTextWrapped(&font, G(craftX), G(craftY + craftH + 4.0f), desc,
                            rgbActive, descScale, craftW * 1.5f);
        (void)rgbInactiveShadow;
    }

    if (!cur && haveFont) {
        const char* lbl = "Craft";
        float lw = fontTextWidth(&font, lbl) * UI_SCALE;
        fontDrawTextShadow(&font, G(craftX) + (G(craftW) - lw) / 2.0f,
                           G(craftY + craftH / 2.0f - 4.0f), lbl, rgbInactive, UI_SCALE);
    }

    sceGuEnable(GU_DEPTH_TEST);
    #undef G
}
