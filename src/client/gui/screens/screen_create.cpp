
#include <pspctrl.h>
#include <pspgu.h>
#include <cstdio>
#include <cstring>

#include "client/gui/screens/menu.h"
#include "client/gui/screens/screen.h"
#include "client/gui/hud.h"
#include "gpu/sprite.h"
#include "gpu/gui_atlas.h"
#include "world/level/levelgen/level_source.h"
#include "world/level/levelgen/gen_features.h"
#include "world/level/storage/worldlist.h"

struct CreateScreen : Screen {
    void renderContent(MenuState& s);
    void handleInput(MenuState& s, unsigned int pressed, unsigned int held);
};

namespace {

const float PX     = 0.625f;
const float TEXT_S = 1.0f;

const float BEVEL = 2.0f;

struct CreateRowDef {
    const char* label;
    const char* placeholder;
    int         oskTarget;
    const char* oskPrompt;
    bool        advancedOnly;
};

const CreateRowDef kFieldRows[] = {
    { "Name", "New world", 1, "Enter World Name:", false },

    { "Seed", 0,           2, "Enter World Seed:", true  },
};
const int FIELD_COUNT = (int)(sizeof(kFieldRows) / sizeof(kFieldRows[0]));
const int ROW_NAME = 0, ROW_SEED = 1;

const int ROW_COUNT = FIELD_COUNT + GEN_FEATURE_COUNT;

bool rowIsToggle(int row)   { return row >= FIELD_COUNT; }
int  rowFeature(int row)    { return row - FIELD_COUNT; }
const char* rowLabel(int row) {
    return rowIsToggle(row) ? kGenFeatures[rowFeature(row)].label : kFieldRows[row].label;
}

enum { FOCUS_TYPE_OLD = ROW_COUNT, FOCUS_TYPE_SKY, FOCUS_TYPE_FLAT,
       FOCUS_SURVIVAL, FOCUS_CREATIVE, FOCUS_CREATE,
       FOCUS_BACK, FOCUS_ADVANCED, FOCUS_COUNT };

const float ROW_LABEL_H = 16.0f * PX;
const float ROW_BOX_H   = 18.0f * PX;

const float LABEL_GAP = 5.0f;

float rowHeight() {
    return ROW_LABEL_H + ROW_BOX_H + 13.0f * PX;
}

bool s_advanced = false;

int  s_lastHeader = FOCUS_BACK;

char* rowText(MenuState& s, int row) {
    return (row == ROW_NAME) ? s.newWorldName : s.newWorldSeed;
}

bool rowVisible(int row) { return s_advanced || (!rowIsToggle(row) && !kFieldRows[row].advancedOnly); }
bool rowInLeftColumn(int row) { return !rowIsToggle(row) && row != ROW_SEED; }

bool genFeaturesUsable(const MenuState& s) {
    return levelSourceFor(s.newWorldType).supportsGenFeatures();
}

bool genFeatureUsable(const MenuState& s, int feature) {
    return genFeaturesUsable(s) && levelSourceFor(s.newWorldType).genFeatureAllowed(feature);
}
bool rowFocusable(const MenuState& s, int row) {
    if (!rowVisible(row)) return false;
    return !rowIsToggle(row) || genFeatureUsable(s, rowFeature(row));
}

const int kTypeFocus[] = { FOCUS_TYPE_OLD, FOCUS_TYPE_SKY, FOCUS_TYPE_FLAT };
const int kTypeWorld[] = { WORLD_TYPE_OLD, WORLD_TYPE_SKY, WORLD_TYPE_FLAT };
const int TYPE_PILLS   = 3;
bool isTypeFocus(int sel) { return sel >= FOCUS_TYPE_OLD && sel < FOCUS_TYPE_OLD + TYPE_PILLS; }
int  typeFocusFor(int worldType) {
    for (int i = 0; i < TYPE_PILLS; i++) if (kTypeWorld[i] == worldType) return kTypeFocus[i];
    return FOCUS_TYPE_OLD;
}

int toggleStep(const MenuState& s, int from, int dir) {
    for (int r = from + dir; r >= FIELD_COUNT && r < ROW_COUNT; r += dir)
        if (rowFocusable(s, r)) return r;
    return -1;
}

const float TOG_S   = 1.0f;
const float TOG_W   = TOGGLE_CELL_W * TOG_S / UI_SCALE;
const float TOG_H   = TOGGLE_CELL_H * TOG_S / UI_SCALE;
const float TOG_ROW = ROW_BOX_H + 5.0f * PX;

const char* modeDescription(int gamemode) {
    return gamemode == 1
        ? "Easily destroy and place blocks. No damage, flying and other cool stuff."
        : "Limited resources, you'll need tools. You may get hurt. Watch out for Monsters.";
}

void drawFieldLabel(Font& font, float x, float widgetY, const char* text) {
    fontDrawTextShadow(&font, x * UI_SCALE,
                       widgetY * UI_SCALE - 8.0f * TEXT_S - LABEL_GAP,
                       text, 0xFFFFFFFFu, TEXT_S);
}

struct Layout {
    float headerH, btnH, hdrBtnY, backX, backW, advX, advW;
    float panelX, panelY, panelW, panelH;
    float formX, formY, formW, formH, boxW, contentH;
    float typeY, modeY, pillW, pillH, pill0X, pill1X;
    float typePillW, typeGap;
    float descX, descY, descW;
    float createX, createY, createW, createH;
};

Layout layout(MenuState& s) {
    Layout L;

    L.btnH    = 18.0f * MENU_PX;
    L.headerH = L.btnH + 8.0f * MENU_PX;

    L.hdrBtnY = MENU_BAR_Y + MENU_BAR_BTNY;
    L.backW   = menuBarButtonW(s, "Back");
    L.backX   = 4.0f * MENU_PX;
    L.advW    = menuBarButtonW(s, "Advanced");
    L.advX    = VW - L.advW - 4.0f * MENU_PX;

    L.panelX = 5.0f * PX;
    L.panelY = L.headerH + 8.0f * PX;
    L.panelW = VW - 10.0f * PX;

    L.panelH = (UI_HINTS_Y / UI_SCALE - 1.0f) - L.panelY;

    L.formX = L.panelX + 5.0f * PX;
    L.formY = L.panelY + 6.0f * PX;
    L.boxW  = VW * 0.28f;
    L.formW = VW * 0.5f - L.formX;

    L.pillH  = 26.0f * PX;
    L.pillW  = VW * 0.205f;
    L.pill0X = L.formX;
    L.pill1X = L.formX + L.pillW + 6.0f * PX;
    L.typeGap   = 4.0f * PX;
    L.typePillW = (2.0f * L.pillW + 6.0f * PX - 2.0f * L.typeGap) / 3.0f;

    L.descX = VW * 0.52f;
    L.descW = VW * 0.44f;

    L.createW = L.descW;
    L.createH = 26.0f * PX;
    L.createX = L.descX;

    if (s_advanced) {

        L.typeY   = L.panelY + L.panelH * 0.42f;
        L.modeY   = L.panelY + L.panelH * 0.72f;
        L.createY = L.modeY;
        L.descY   = 0.0f;
        L.formH   = L.typeY - LABEL_GAP / UI_SCALE - 4.0f * PX - L.formY;
    } else {
        L.typeY   = 0.0f;
        L.modeY   = L.panelY + L.panelH * 0.52f;
        L.createY = L.panelY + L.panelH - L.createH - 6.0f * PX;
        L.descY   = L.modeY;
        L.formH   = L.modeY - LABEL_GAP / UI_SCALE - 4.0f * PX - L.formY;
    }

    L.contentH = 0.0f;
    for (int i = 0; i < ROW_COUNT; i++)
        if (rowVisible(i) && rowInLeftColumn(i)) L.contentH += rowHeight();
    return L;
}

int effectiveGameMode(const MenuState& s) {
    int forced = levelSourceFor(s.newWorldType).forcedGameType();
    return (forced >= 0) ? forced : s.newWorldGamemode;
}
bool gameModeLocked(const MenuState& s) {
    return levelSourceFor(s.newWorldType).forcedGameType() >= 0;
}

}

void CreateScreen::handleInput(MenuState& s, unsigned int pressed, unsigned int ) {
    int& sel = s.createSelected;
    if (sel < 0 || sel >= FOCUS_COUNT) sel = 0;

    if (pressed & PSP_CTRL_TRIANGLE) {
        s_advanced = !s_advanced;
        if (!s_advanced && (sel == ROW_SEED || rowIsToggle(sel) ||
                            isTypeFocus(sel)))
            sel = ROW_NAME;
    }

    const bool locked = gameModeLocked(s);
    const int  modePill = effectiveGameMode(s) ? FOCUS_CREATIVE : FOCUS_SURVIVAL;
    const int  typePill = typeFocusFor(s.newWorldType);
    const bool onHeader = (sel == FOCUS_BACK || sel == FOCUS_ADVANCED);
    const bool onType   = isTypeFocus(sel);
    const bool onMode   = (sel == FOCUS_SURVIVAL || sel == FOCUS_CREATIVE);

    const int belowType  = locked ? FOCUS_CREATE : modePill;
    const int aboveCreate = locked ? (s_advanced ? typePill : ROW_NAME) : modePill;

    const int firstToggle = toggleStep(s, FIELD_COUNT - 1, +1);
    const bool onToggle   = (sel >= FIELD_COUNT && sel < ROW_COUNT);

    if (pressed & PSP_CTRL_DOWN) {
        if (onHeader)                sel = ROW_NAME;
        else if (sel == ROW_NAME)    sel = s_advanced ? typePill : belowType;
        else if (sel == ROW_SEED)    sel = (firstToggle >= 0) ? firstToggle : typePill;
        else if (onToggle)           { int n = toggleStep(s, sel, +1);
                                       sel = (n >= 0) ? n : typePill; }
        else if (onType)             sel = belowType;
        else if (onMode)             sel = FOCUS_CREATE;
    }
    if (pressed & PSP_CTRL_UP) {
        if (sel == FOCUS_CREATE)  sel = aboveCreate;
        else if (onMode)          sel = s_advanced ? typePill : ROW_NAME;
        else if (onType)          sel = ROW_NAME;
        else if (onToggle)        { int p = toggleStep(s, sel, -1);
                                    sel = (p >= 0) ? p : ROW_SEED; }
        else if (sel == ROW_SEED) sel = ROW_NAME;
        else if (sel == ROW_NAME) sel = s_lastHeader;
    }
    if (pressed & PSP_CTRL_RIGHT) {
        if (sel == FOCUS_BACK)             sel = FOCUS_ADVANCED;
        else if (sel == ROW_NAME && s_advanced) sel = ROW_SEED;
        else if (onType && sel < FOCUS_TYPE_OLD + TYPE_PILLS - 1) {
            sel++; s.newWorldType = kTypeWorld[sel - FOCUS_TYPE_OLD];
        }
        else if (sel == FOCUS_SURVIVAL && !locked) { sel = FOCUS_CREATIVE; s.newWorldGamemode = 1; }
        else if (sel == FOCUS_CREATIVE || (sel == FOCUS_SURVIVAL && locked)) sel = FOCUS_CREATE;
    }
    if (pressed & PSP_CTRL_LEFT) {
        if (sel == FOCUS_ADVANCED)       sel = FOCUS_BACK;
        else if (sel == ROW_SEED)        sel = ROW_NAME;
        else if (onToggle)               sel = ROW_NAME;
        else if (onType && sel > FOCUS_TYPE_OLD) {
            sel--; s.newWorldType = kTypeWorld[sel - FOCUS_TYPE_OLD];
        }
        else if (sel == FOCUS_CREATIVE && !locked) { sel = FOCUS_SURVIVAL; s.newWorldGamemode = 0; }
        else if (sel == FOCUS_CREATE)    sel = aboveCreate;
    }
    if (sel == FOCUS_BACK || sel == FOCUS_ADVANCED) s_lastHeader = sel;

    if (gameModeLocked(s) && (sel == FOCUS_SURVIVAL || sel == FOCUS_CREATIVE))
        sel = FOCUS_CREATE;

    if (pressed & PSP_CTRL_CIRCLE) s.screen = SCREEN_WORLDS;

    if (pressed & PSP_CTRL_CROSS) {
        if (sel < ROW_COUNT && rowIsToggle(sel)) {
            if (genFeatureUsable(s, rowFeature(sel)))
                s.newWorldGenMask = genFeatureToggled(s.newWorldGenMask, rowFeature(sel));
        } else if (sel < ROW_COUNT) {
            const CreateRowDef& row = kFieldRows[sel];
            startOsk(row.oskTarget, row.oskPrompt, rowText(s, sel));
        } else if (isTypeFocus(sel))       { s.newWorldType = kTypeWorld[sel - FOCUS_TYPE_OLD];
        } else if (sel == FOCUS_SURVIVAL)  { if (!locked) s.newWorldGamemode = 0;
        } else if (sel == FOCUS_CREATIVE)  { if (!locked) s.newWorldGamemode = 1;
        } else if (sel == FOCUS_BACK)      { s.screen = SCREEN_WORLDS;
        } else if (sel == FOCUS_ADVANCED)  { s_advanced = !s_advanced;
        } else if (sel == FOCUS_CREATE) {
            char created[64];

            long seed = worldSeedFromString(s.newWorldSeed);
            if (worldListCreate(&s.worlds, s.newWorldName, created,
                                effectiveGameMode(s), seed, s.newWorldType,
                                s.newWorldGenMask)) {
                snprintf(s.statusMsg, sizeof(s.statusMsg), "Loading: %s", created);
                s.worldSelected = s.worlds.count - 1;
                s.screen = SCREEN_GAME;
            } else {
                s.screen = SCREEN_WORLDS;
            }
            s.uiRow = 1;
        }
    }
}

void CreateScreen::renderContent(MenuState& s) {
    if (!s.haveFont || !s.haveGui) return;
    Font& font = s.font;
    const int sel = s.createSelected;
    const Layout L = layout(s);
    const bool locked = gameModeLocked(s);
    const int  mode   = effectiveGameMode(s);

    sceGuDisable(GU_DEPTH_TEST);

    {
        float lb = L.backX + L.backW, rb = L.advX;
        drawMenuHeader(s, "Create a World", 0.0f, VW, L.headerH, MENU_BAR_TEXT, lb, rb - lb);
    }
    menuBarButton(s, L.backX, L.backW, "Back", sel == FOCUS_BACK);

    guiTButton(s, L.advX, L.hdrBtnY, L.advW, L.btnH, s_advanced, MENU_BEVEL);

    guiTButtonLabel(s, L.advX, L.hdrBtnY, L.advW, L.btnH, "Advanced",
                    sel == FOCUS_ADVANCED, true, MENU_BAR_TEXT);

    drawNinePatch(s, GA_SS_PANEL, 3.0f, L.panelX, L.panelY, L.panelW, L.panelH, 3.0f * PX);

    float scroll = s.createScroll;

    if (sel < ROW_COUNT && rowInLeftColumn(sel) && rowVisible(sel)) {
        float selY = 0.0f;
        for (int i = 0; i < sel; i++)
            if (rowVisible(i) && rowInLeftColumn(i)) selY += rowHeight();
        if (selY < scroll) scroll = selY;
        if (selY + rowHeight() > scroll + L.formH) scroll = selY + rowHeight() - L.formH;
    }
    float maxScroll = L.contentH - L.formH;
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    if (scroll < 0.0f) scroll = 0.0f; else if (scroll > maxScroll) scroll = maxScroll;
    s.createScroll = scroll;

    sceGuScissor((int)(L.panelX * UI_SCALE), (int)(L.formY * UI_SCALE),
                 (int)(L.formW * UI_SCALE), (int)(L.formH * UI_SCALE));
    {
        float rowY = L.formY - scroll;
        for (int i = 0; i < ROW_COUNT; i++) {
            if (!rowVisible(i) || !rowInLeftColumn(i)) continue;
            float boxY = rowY + ROW_LABEL_H;
            drawFieldLabel(font, L.formX, boxY, rowLabel(i));
            drawTextField(s, L.formX, boxY, L.boxW, ROW_BOX_H,
                          rowText(s, i), kFieldRows[i].placeholder, sel == i, TEXT_S);
            rowY += rowHeight();
        }
    }
    sceGuScissor(0, 0, 480, 272);

    guiScrollbar((L.formX + L.boxW + 2.0f) * UI_SCALE, L.formY * UI_SCALE,
                 2.0f * PX * UI_SCALE, L.formH * UI_SCALE,
                 L.contentH * UI_SCALE, scroll * UI_SCALE);

    if (s_advanced) {

        drawFieldLabel(font, L.descX, L.formY + ROW_LABEL_H, rowLabel(ROW_SEED));
        drawTextField(s, L.descX, L.formY + ROW_LABEL_H, L.boxW, ROW_BOX_H,
                      s.newWorldSeed, kFieldRows[ROW_SEED].placeholder, sel == ROW_SEED, TEXT_S);

        {
            float togRowY = L.formY + ROW_LABEL_H + ROW_BOX_H + 6.0f * PX;

            const float togRight = L.panelX + L.panelW - 5.0f * PX;
            float widestLabel = 0.0f;
            for (int i = FIELD_COUNT; i < ROW_COUNT; i++) {
                float w = (float)fontTextWidth(&font, rowLabel(i)) * TEXT_S / UI_SCALE;
                if (w > widestLabel) widestLabel = w;
            }
            float togX = L.descX + widestLabel + 6.0f * PX;
            if (togX + TOG_W > togRight) togX = togRight - TOG_W;
            for (int i = FIELD_COUNT; i < ROW_COUNT; i++, togRowY += TOG_ROW) {
                const bool usable = genFeatureUsable(s, rowFeature(i));
                const bool on = usable && genFeatureEnabled(s.newWorldGenMask, rowFeature(i));

                const float labelY = (float)(int)(togRowY * UI_SCALE
                                     + (ROW_BOX_H * UI_SCALE - 8.0f * TEXT_S) / 2.0f);
                fontDrawTextClipped(&font, L.descX * UI_SCALE, labelY,
                                    rowLabel(i),
                                    !usable ? GUI_DISABLED
                                               : (sel == i ? 0xFFFFFFFFu : 0xFFE0E0E0u), TEXT_S,
                                    (togX - 2.0f - L.descX) * UI_SCALE / TEXT_S);
                guiOptionSwitch(s, togX,
                                togRowY + (ROW_BOX_H - TOG_H) / 2.0f, TOG_W, TOG_H,
                                on, sel == i, !usable ? GUI_DISABLED : 0xFFFFFFFFu,
                                TOG_S);
            }
        }

        drawFieldLabel(font, L.formX, L.typeY, "World Type");
        {
            for (int i = 0; i < TYPE_PILLS; i++) {
                const float x = L.pill0X + i * (L.typePillW + L.typeGap);
                guiTButton(s, x, L.typeY, L.typePillW, L.pillH, s.newWorldType == kTypeWorld[i], BEVEL);
                guiTButtonLabel(s, x, L.typeY, L.typePillW, L.pillH,
                                levelSourceFor(kTypeWorld[i]).label(),
                                sel == kTypeFocus[i], true, TEXT_S);
            }
        }
    }

    drawFieldLabel(font, L.formX, L.modeY, "Game Mode");
    {
        const bool creative = (mode == 1);

        guiTButton(s, L.pill0X, L.modeY, L.pillW, L.pillH, !creative, BEVEL);
        guiTButtonLabel(s, L.pill0X, L.modeY, L.pillW, L.pillH, "Survival",
                        sel == FOCUS_SURVIVAL, !locked, TEXT_S);
        guiTButton(s, L.pill1X, L.modeY, L.pillW, L.pillH, creative, BEVEL);
        guiTButtonLabel(s, L.pill1X, L.modeY, L.pillW, L.pillH, "Creative",
                        sel == FOCUS_CREATIVE, !locked, TEXT_S);
    }

    if (!s_advanced)
        fontDrawTextWrapped(&font, L.descX * UI_SCALE, L.descY * UI_SCALE,
                            modeDescription(mode), 0xFFFFFFFFu, TEXT_S,
                            L.descW * UI_SCALE / TEXT_S);

    guiTButton(s, L.createX, L.createY, L.createW, L.createH, sel == FOCUS_CREATE, BEVEL);
    guiTButtonLabel(s, L.createX, L.createY, L.createW, L.createH, "Create World!",
                    sel == FOCUS_CREATE, true, TEXT_S);
}

void createFormReset(MenuState& s) {
    s.createSelected = 0;

    strcpy(s.newWorldName, "My World");
    s.newWorldSeed[0] = '\0';
    s.newWorldGamemode = 0;
    s.newWorldType = WORLD_TYPE_OLD;
    s.newWorldGenMask = genFeaturesDefaultMask();
    s.createScroll = 0.0f;
}

static CreateScreen s_createScreen;
Screen& createScreen() { return s_createScreen; }
