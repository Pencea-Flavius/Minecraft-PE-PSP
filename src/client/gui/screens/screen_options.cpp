
#include <pspctrl.h>
#include <pspgu.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "client/gui/screens/menu.h"
#include "platform/audio/sound.h"
#include "gpu/sprite.h"
#include "platform/path.h"
#include "gpu/gui_atlas.h"

struct OptionRowDef {

    const char* group;
    const char* label;
    const char* values[4];
    int numValues;
    int def;
    bool percent;
    int percentMin;
    int percentStep;
    bool cycle;
};

#define OPT_CATEGORIES 4
#define OPT_MAX_ROWS   5

static const OptionRowDef g_optionRows[OPT_CATEGORIES][OPT_MAX_ROWS] = {
    {

        { "Game", "Difficulty",   {"Peaceful", "Easy", "Normal", "Hard"}, 4, 2 },
        { 0,      "Third Person", {"Off", "On", 0, 0}, 2, 0 },

        { 0,      "Autosave",     {"Off", "15 min", "20 min", "30 min"}, 4, 1, false, 0, 0, true },
        { "Interface", "Bar On Top", {"Off", "On", 0, 0}, 2, 0 },
        { 0,           "Show FPS",   {"Off", "On", 0, 0}, 2, 0 },
    },
    {

        { "Input", "Sensitivity",  {0, 0, 0, 0}, 21, 10, true, 0, 10 },
        { 0,       "Auto Jump",     {"Off", "On", 0, 0}, 2, 1 },
        { 0,       "Block Outline", {"Off", "On", 0, 0}, 2, 1 },
        { 0,       "Show Coordinates", {"Off", "On", 0, 0}, 2, 0 },
    },
    {

        { "Graphics", "Render Distance", {"Tiny", "Short", "Normal", 0}, 3, 2, false, 0, 0, true },
        { 0,          "Fancy Graphics",  {"Off", "On", 0, 0}, 2, 0 },
        { 0,          "View Bobbing",    {"Off", "On", 0, 0}, 2, 1 },
        { "Experimental", "Mipmapping",  {"Off", "On", 0, 0}, 2, 1 },

    },
    {

        { "Audio", "Sound Volume", {0, 0, 0, 0}, 11, 10, true, 0, 10 },
    },
};
static const int g_optionRowCount[OPT_CATEGORIES] = { 5, 4, 4, 1 };
static const char* g_optionCategoryNames[OPT_CATEGORIES] = { "Game", "Controls", "Graphics", "Audio" };
static int g_optionValueIdx[OPT_CATEGORIES][OPT_MAX_ROWS];

extern float g_viewDist;
extern int   g_viewBobbing;
extern int   g_fancyGraphics;
extern int   g_noMipmap;
extern int   g_showFps;
extern int   g_showCoords;
extern int   g_difficulty;
extern int   g_autosave;
extern int   g_blockOutline;
extern int   g_autoJump;
extern int   g_barOnTop;
extern float g_sensitivity;
extern bool  g_thirdPerson;

#define CAT_GRAPHICS   2
#define ROW_RENDERDIST 0
#define ROW_FANCY      1
#define ROW_BOBBING    2
#define ROW_MIPMAP     3

static const float kRenderDist[3] = { 16.0f, 32.0f, 48.0f };

extern int g_lowMemPsp;
static int renderDistChoices() { return g_lowMemPsp ? 2 : 3; }

#define CAT_CONTROLS     1
#define ROW_SENS         0
#define ROW_AUTOJUMP     1
#define ROW_BLOCKOUTLINE 2
#define ROW_SHOWCOORDS   3

#define CAT_GAME        0
#define ROW_DIFFICULTY  0
#define ROW_THIRDPERSON 1
#define ROW_AUTOSAVE    2
#define ROW_BARONTOP    3
#define ROW_SHOWFPS     4

#define CAT_AUDIO       3
#define ROW_SOUNDVOL    0
static const int kAutosaveTicks[4] = { 0, 18000, 24000, 36000 };

unsigned int optionsValueSig() {
    unsigned int h = 2166136261u;
    for (int c = 0; c < OPT_CATEGORIES; c++)
        for (int r = 0; r < g_optionRowCount[c]; r++)
            h = (h ^ (unsigned int)g_optionValueIdx[c][r]) * 16777619u;
    return h;
}

static void optionsApply() {
    g_fancyGraphics = g_optionValueIdx[CAT_GRAPHICS][ROW_FANCY];
    g_viewBobbing = g_optionValueIdx[CAT_GRAPHICS][ROW_BOBBING];
    int rd = g_optionValueIdx[CAT_GRAPHICS][ROW_RENDERDIST];
    if (rd < 0) rd = 0; else if (rd > 2) rd = 2;
    if (rd >= renderDistChoices()) rd = renderDistChoices() - 1;
    g_optionValueIdx[CAT_GRAPHICS][ROW_RENDERDIST] = rd;
    g_viewDist    = kRenderDist[rd];
    g_noMipmap    = (g_optionValueIdx[CAT_GRAPHICS][ROW_MIPMAP] == 1) ? 0 : 1;
    g_showFps     = g_optionValueIdx[CAT_GAME][ROW_SHOWFPS];
    g_showCoords  = g_optionValueIdx[CAT_CONTROLS][ROW_SHOWCOORDS];
    g_barOnTop    = g_optionValueIdx[CAT_GAME][ROW_BARONTOP];
    int ai = g_optionValueIdx[CAT_GAME][ROW_AUTOSAVE];
    if (ai < 0) ai = 0; else if (ai > 3) ai = 3;
    g_autosave    = kAutosaveTicks[ai];
    g_blockOutline = g_optionValueIdx[CAT_CONTROLS][ROW_BLOCKOUTLINE];
    g_autoJump     = g_optionValueIdx[CAT_CONTROLS][ROW_AUTOJUMP];
    g_difficulty  = g_optionValueIdx[CAT_GAME][ROW_DIFFICULTY];
    soundSetVolume(g_optionValueIdx[CAT_AUDIO][ROW_SOUNDVOL] / 10.0f);

    g_sensitivity = g_optionValueIdx[CAT_CONTROLS][ROW_SENS] / 10.0f;
    g_thirdPerson = g_optionValueIdx[CAT_GAME][ROW_THIRDPERSON] != 0;
}

void optionsSetThirdPerson(bool on) {
    g_optionValueIdx[CAT_GAME][ROW_THIRDPERSON] = on ? 1 : 0;
    optionsSave();
}

static void optionsSetDefaults() {
    for (int c = 0; c < OPT_CATEGORIES; c++)
        for (int r = 0; r < g_optionRowCount[c]; r++)
            g_optionValueIdx[c][r] = g_optionRows[c][r].def;
}

void optionsInitDefaults() {
    optionsSetDefaults();
    optionsApply();
}

void optionsSave() {
    FILE* f = fopen(assetPath("options.txt"), "w");
    if (!f) return;
    for (int c = 0; c < OPT_CATEGORIES; c++)
        for (int r = 0; r < g_optionRowCount[c]; r++)
            fprintf(f, "%s=%d\n", g_optionRows[c][r].label, g_optionValueIdx[c][r]);
    fclose(f);
}

void optionsLoad() {
    optionsSetDefaults();
    FILE* f = fopen(assetPath("options.txt"), "r");
    if (f) {
        char line[128];
        while (fgets(line, sizeof(line), f)) {
            char* eq = strrchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            int val = atoi(eq + 1);
            for (int c = 0; c < OPT_CATEGORIES; c++)
                for (int r = 0; r < g_optionRowCount[c]; r++)
                    if (strcmp(line, g_optionRows[c][r].label) == 0) {
                        int nv = g_optionRows[c][r].numValues;
                        if (val < 0) val = 0; else if (val >= nv) val = nv - 1;
                        g_optionValueIdx[c][r] = val;
                    }
        }
        fclose(f);
    }
    optionsApply();
}

static const float kOptRowH    = 14.0f;
static const float kOptHeaderH = 9.0f;
static float optionRowY(int category, int row, float y0) {
    float y = y0;
    for (int r = 0; r < row; r++) {
        if (g_optionRows[category][r].group) y += kOptHeaderH;
        y += kOptRowH;
    }
    if (g_optionRows[category][row].group) y += kOptHeaderH;
    return y;
}

static bool optionRowIsBoolean(const OptionRowDef& row) {
    return row.numValues == 2 && strcmp(row.values[0], "Off") == 0 && strcmp(row.values[1], "On") == 0;
}

void optionsHandleInput(MenuState& s, unsigned int pressed) {
    int& optFocus = s.optFocus;
    int& optCategory = s.optCategory;
    int& optTabHighlight = s.optTabHighlight;
    int& optItemHighlight = s.optItemHighlight;
    int& optEditingRow = s.optEditingRow;
    AppScreen& screen = s.screen;

    int rowCount = g_optionRowCount[optCategory];

    if (optEditingRow >= 0) {

        const OptionRowDef& row = g_optionRows[optCategory][optEditingRow];
        int idx = g_optionValueIdx[optCategory][optEditingRow];
        if ((pressed & PSP_CTRL_LEFT) && idx > 0) {
            g_optionValueIdx[optCategory][optEditingRow] = idx - 1;
            optionsApply();
        }
        if ((pressed & PSP_CTRL_RIGHT) && idx < row.numValues - 1) {
            g_optionValueIdx[optCategory][optEditingRow] = idx + 1;
            optionsApply();
        }
        if (pressed & PSP_CTRL_CIRCLE)
            optEditingRow = -1;
    } else {

        optFocus = 1;
        if ((pressed & PSP_CTRL_LTRIGGER) && optCategory > 0) { optCategory--; optItemHighlight = 0; }
        if ((pressed & PSP_CTRL_RTRIGGER) && optCategory < OPT_CATEGORIES - 1) { optCategory++; optItemHighlight = 0; }
        optTabHighlight = optCategory;
        rowCount = g_optionRowCount[optCategory];

        if ((pressed & PSP_CTRL_UP)   && optItemHighlight > 0)            optItemHighlight--;
        if ((pressed & PSP_CTRL_DOWN) && optItemHighlight < rowCount - 1) optItemHighlight++;

        if (pressed & PSP_CTRL_CIRCLE) {
            optionsSave();
            screen = SCREEN_TITLE;
        }
        if (pressed & PSP_CTRL_CROSS) {
            const OptionRowDef& row = g_optionRows[optCategory][optItemHighlight];
            if (optionRowIsBoolean(row) || row.cycle) {

                int nVals = row.numValues;
                if (optCategory == CAT_GRAPHICS && optItemHighlight == ROW_RENDERDIST)
                    nVals = renderDistChoices();
                g_optionValueIdx[optCategory][optItemHighlight] =
                    (g_optionValueIdx[optCategory][optItemHighlight] + 1) % nVals;
                optionsApply();
            } else {

                optEditingRow = optItemHighlight;
            }
        }
    }
}

void optionsRender(MenuState& s) {
    Font& font = s.font; bool haveFont = s.haveFont;
    Texture& dirtBg = s.dirtBg; bool haveBg = s.haveBg;

    Texture& touchGui = s.touchGui;
    bool haveArt = s.haveTouch;
    int& optFocus = s.optFocus;
    int& optCategory = s.optCategory;
    int& optTabHighlight = s.optTabHighlight;
    int& optItemHighlight = s.optItemHighlight;
    int& optEditingRow = s.optEditingRow;

    if (haveBg) {
        textureBind(&dirtBg);
        sceGuDisable(GU_DEPTH_TEST);
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        float tileScale = (float)dirtBg.realW / (32.0f * UI_SCALE);
        float bgW = (VW * UI_SCALE) * tileScale;
        float bgH = (VH * UI_SCALE) * tileScale;
        spriteDraw(&dirtBg, 0.0f, 0.0f, VW * UI_SCALE, VH * UI_SCALE,
                   0.0f, 0.0f, bgW, bgH, DIRT_TINT);
        sceGuTexWrap(GU_CLAMP, GU_CLAMP);
        sceGuEnable(GU_DEPTH_TEST);
    }

    if (haveArt && haveFont) {
        sceGuDisable(GU_DEPTH_TEST);
        float barBtnH = 26.0f;
        float closeBtnW = 34.0f;
        float tabW = 66.0f;

        float hW = VW - closeBtnW;
        textureBind(&touchGui);
        spriteDraw(&touchGui, 0.0f, 0.0f, 2.0f * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE, 150.0f, 26.0f, 2.0f, 25.0f, WHITE);
        textureBind(&touchGui);
        spriteDraw(&touchGui, 2.0f * UI_SCALE, 0.0f, (hW - 4.0f) * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE, 153.0f, 26.0f, 8.0f, 25.0f, WHITE);
        textureBind(&touchGui);
        spriteDraw(&touchGui, (hW - 2.0f) * UI_SCALE, 0.0f, 2.0f * UI_SCALE, (barBtnH - 1.0f) * UI_SCALE, 162.0f, 26.0f, 2.0f, 25.0f, WHITE);
        textureBind(&touchGui);
        spriteDraw(&touchGui, 0.0f, (barBtnH - 1.0f) * UI_SCALE, hW * UI_SCALE, 3.0f * UI_SCALE, 153.0f, 52.0f, 8.0f, 3.0f, WHITE);

        float headerW = fontTextWidth(&font, "Options") * UI_SCALE;
        fontDrawTextShadow(&font, (hW * UI_SCALE - headerW) / 2.0f, (barBtnH - 8.0f) / 2.0f * UI_SCALE, "Options", 0xFFE0E0E0u, UI_SCALE);

        bool closeHovered = (optFocus == 2);
        {
            float scale = closeHovered ? 0.95f : 1.0f;
            float drawW = closeBtnW * scale, drawH = barBtnH * scale;
            float drawX = hW + (closeBtnW - drawW) / 2.0f, drawY = (barBtnH - drawH) / 2.0f;
            textureBind(&touchGui);
            spriteDraw(&touchGui, drawX * UI_SCALE, drawY * UI_SCALE, drawW * UI_SCALE, drawH * UI_SCALE, closeHovered ? 184.0f : 150.0f, 0.0f, 34.0f, 26.0f, WHITE);
        }

        float tabY0 = barBtnH;

        float tabH = 22.0f;
        for (int i = 0; i < OPT_CATEGORIES; i++) {
            bool tabHovered = (optFocus == 0 && optTabHighlight == i);
            bool tabActive = (optCategory == i);
            bool tabPressedVisual = tabActive || tabHovered;
            float tY = tabY0 + i * tabH;
            textureBind(&touchGui);
            spriteDraw(&touchGui, 0.0f, tY * UI_SCALE, tabW * UI_SCALE, tabH * UI_SCALE, tabPressedVisual ? 66.0f : 0.0f, 0.0f, 66.0f, 26.0f, WHITE);

            unsigned int tabCol = tabActive ? 0xFFA0FFFFu : 0xFFE0E0E0u;
            float tw = fontTextWidth(&font, g_optionCategoryNames[i]) * UI_SCALE;
            fontDrawTextShadow(&font, (tabW * UI_SCALE - tw) / 2.0f, (tY + (tabH - 8.0f) / 2.0f) * UI_SCALE, g_optionCategoryNames[i], tabCol, UI_SCALE);
        }

        int rowCount = g_optionRowCount[optCategory];
        float itemsX = tabW + 4.0f;
        float itemsW = VW - tabW - 8.0f;
        float rowH = kOptRowH;
        float rowY0 = barBtnH + 2.0f;
        for (int r = 0; r < rowCount; r++) {
            const OptionRowDef& row = g_optionRows[optCategory][r];
            int valIdx = g_optionValueIdx[optCategory][r];
            float rY = optionRowY(optCategory, r, rowY0);

            if (row.group)
                fontDrawTextShadow(&font, itemsX * UI_SCALE, (rY - kOptHeaderH + 2.0f) * UI_SCALE,
                                   row.group, 0xFFA0FFFFu, UI_SCALE);
            bool rowGrabbed = (optEditingRow == r);
            bool rowHovered = (optFocus == 1 && optItemHighlight == r) || rowGrabbed;

            if (rowHovered)
                drawRect(itemsX * UI_SCALE, rY * UI_SCALE, itemsW * UI_SCALE, (rowH - 1.0f) * UI_SCALE, 0x40FFFFFFu);

            unsigned int labelCol = rowHovered ? 0xFFA0FFFFu : 0xFFE0E0E0u;
            fontDrawTextShadow(&font, itemsX * UI_SCALE, (rY + (rowH - 8.0f) / 2.0f) * UI_SCALE, row.label, labelCol, UI_SCALE);

            if (optionRowIsBoolean(row)) {

                const float togW = 27.0f, togH = 14.0f;
                float togX = itemsX + itemsW - togW - 8.0f;
                float togY = rY + (rowH - togH) / 2.0f;
                float scale = rowHovered ? 0.95f : 1.0f;
                float drawW = togW * scale, drawH = togH * scale;
                float drawX = togX + (togW - drawW) / 2.0f, drawY = togY + (togH - drawH) / 2.0f;
                textureBind(&touchGui);
                spriteDraw(&touchGui, drawX * UI_SCALE, drawY * UI_SCALE, drawW * UI_SCALE, drawH * UI_SCALE, valIdx == 1 ? 199.0f : 160.0f, 206.0f, 39.0f, 20.0f, WHITE);
            } else if (row.cycle) {

                const char* valTxt = row.values[valIdx] ? row.values[valIdx] : "";
                const float btnW = 56.0f, btnH = 13.0f;
                float btnX = itemsX + itemsW - btnW - 8.0f;
                float btnY = rY + (rowH - btnH) / 2.0f;
                textureBind(&touchGui);
                spriteDraw(&touchGui, btnX * UI_SCALE, btnY * UI_SCALE, btnW * UI_SCALE, btnH * UI_SCALE, rowHovered ? 66.0f : 0.0f, 0.0f, 66.0f, 26.0f, WHITE);
                unsigned int vcol = rowHovered ? 0xFFA0FFFFu : 0xFFE0E0E0u;
                float vw = fontTextWidth(&font, valTxt) * UI_SCALE;
                fontDrawTextShadow(&font, btnX * UI_SCALE + (btnW * UI_SCALE - vw) / 2.0f,
                                   (btnY + (btnH - 8.0f) / 2.0f) * UI_SCALE, valTxt, vcol, UI_SCALE);
            } else {

                const float sliderW = 80.0f;
                float sliderX = itemsX + itemsW - sliderW - 8.0f;
                float trackX0 = sliderX + 5.0f, trackX1 = sliderX + sliderW - 5.0f;
                float barWidth = trackX1 - trackX0;
                float trackY = rY + rowH / 2.0f - 1.5f;

                drawRect(trackX0 * UI_SCALE, trackY * UI_SCALE, barWidth * UI_SCALE, 3.0f * UI_SCALE, 0xFF606060u);

                if (!row.percent && row.numValues > 2) {
                    float stepDist = barWidth / (float)(row.numValues - 1);
                    for (int sIdx = 0; sIdx < row.numValues; sIdx++) {
                        float tickX = trackX0 + sIdx * stepDist;
                        drawRect((tickX - 1.0f) * UI_SCALE, (trackY - 2.0f) * UI_SCALE, 2.0f * UI_SCALE, 7.0f * UI_SCALE, 0xFF606060u);
                    }
                }

                float percentage = (float)valIdx / (float)(row.numValues - 1);
                const float thumbW = 9.0f, thumbH = 15.0f;
                float thumbX = trackX0 + percentage * barWidth - thumbW / 2.0f;
                float thumbY = rY + (rowH - thumbH) / 2.0f;

                unsigned int thumbTint = rowGrabbed ? 0xFFA0FFFFu : WHITE;
                textureBind(&touchGui);
                spriteDraw(&touchGui, thumbX * UI_SCALE, thumbY * UI_SCALE, thumbW * UI_SCALE, thumbH * UI_SCALE, 226.0f, 126.0f, 9.0f, 15.0f, thumbTint);
            }
        }

        sceGuEnable(GU_DEPTH_TEST);
    }
}
