
#include <pspctrl.h>
#include <pspgu.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "client/gui/screens/menu.h"
#include "client/gui/screens/screen.h"
#include "platform/audio/sound.h"
#include "gpu/sprite.h"
#include "client/gui/hud.h"
#include "platform/path.h"
#include "gpu/gui_atlas.h"
#include "world/level/world.h"
#include "client/renderer/particle.h"
#include "client/gui/screens/control_scheme.h"

struct OptionRowDef {

    const char* group;
    const char* label;

    const char* values[5];
    int numValues;
    int def;
    bool percent;
    int percentMin;
    int percentStep;

    bool button;
};

#define OPT_CATEGORIES 4
#define OPT_MAX_ROWS   12

static const OptionRowDef g_optionRows[OPT_CATEGORIES][OPT_MAX_ROWS] = {
    {

        { "Game", "Difficulty",   {"Peaceful", "Easy", "Normal", "Hard"}, 4, 2 },

        { 0,      "Third Person", {"Off", "Behind", "In Front", 0}, 3, 0 },

        { 0,      "Auto Save",    {"Off", "5 min", "15 min", "20 min", "30 min"}, 5, 2 },
        { "Interface", "Bar On Top", {"Off", "On", 0, 0}, 2, 0 },
        { 0,           "Show FPS",   {"Off", "On", 0, 0}, 2, 0 },

        { 0,           "Show Coordinates", {"Off", "On", 0, 0}, 2, 0 },
        { 0,           "Block Outline",    {"Off", "On", 0, 0}, 2, 1 },

        { 0,           "Hide GUI",         {"Off", "On", 0, 0}, 2, 0 },

        { 0,           "Interface Opacity", {0, 0, 0, 0}, 11, 8, true, 0, 10 },
    },
    {

        { "Input", "Sensitivity",  {0, 0, 0, 0}, 21, 10, true, 0, 10 },

        { 0,       "Dead Zone",     {0, 0, 0, 0}, 11, 4, true, 0, 5 },
        { 0,       "Auto Jump",     {"Off", "On", 0, 0}, 2, 1 },

        { 0,       "Japanese Layout", {"Off", "On", 0, 0}, 2, 0 },

        { 0,       "Classic Item Pick", {"Off", "On", 0, 0}, 2, 0 },

        { 0,       "Control Scheme", {"Layout 1", "Layout 2", "Layout 3", "Layout 4"}, 4, 0,
          false, 0, 0, true },
    },
    {

        { "Graphics", "View Distance",   {"Tiny", "Short", "Normal", "Far"}, 4, 2 },

        { 0,          "Clouds",          {"Off", "Fast", "Fancy"}, 3, 1 },

        { 0,          "Leaves",          {"Off", "Fast", "Fancy"}, 3, 0 },
        { 0,          "View Bobbing",    {"Off", "On", 0, 0}, 2, 1 },

        { 0,          "Beautiful Skies", {"Off", "On", 0, 0}, 2, 1 },
        { 0,          "Animate Textures",{"Off", "On", 0, 0}, 2, 1 },

        { 0,          "Particles",       {"Off", "On", 0, 0}, 2, 1 },
        { 0,          "Smooth Lighting", {"Off", "On", 0, 0}, 2, 1 },

        { 0,          "Brightness",      {0, 0, 0, 0}, 11, 0, true, 0, 10 },
        { "Experimental", "Mipmapping",  {"Off", "On", 0, 0}, 2, 1 },

        { 0,              "Dithering",   {"Off", "On", 0, 0}, 2, 0 },

    },
    {

        { "Audio", "Master",        {0, 0, 0, 0}, 11, 10, true, 0, 10 },
        { 0,       "Music",         {0, 0, 0, 0}, 11, 10, true, 0, 10 },
        { 0,       "Blocks",        {0, 0, 0, 0}, 11, 10, true, 0, 10 },
        { 0,       "Hostile Mobs",  {0, 0, 0, 0}, 11, 10, true, 0, 10 },
        { 0,       "Friendly Mobs", {0, 0, 0, 0}, 11, 10, true, 0, 10 },
        { 0,       "Players",       {0, 0, 0, 0}, 11, 10, true, 0, 10 },
        { 0,       "Ambient",       {0, 0, 0, 0}, 11, 10, true, 0, 10 },
        { 0,       "UI",            {0, 0, 0, 0}, 11, 10, true, 0, 10 },
    },
};

static const int g_optionRowCount[OPT_CATEGORIES] = { 9, 6, 11, 8 };
static const char* g_optionCategoryNames[OPT_CATEGORIES] = { "Game", "Controls", "Graphics", "Audio" };
static int g_optionValueIdx[OPT_CATEGORIES][OPT_MAX_ROWS];

extern float g_viewDist;
extern int   g_viewBobbing;
extern int   g_fancyGraphics;
extern int   g_fancyLeaves;
extern int   g_cloudMode;
extern int   g_noMipmap;
extern int   g_showFps;
extern int   g_showCoords;
extern int   g_difficulty;
extern int   g_autosave;
extern int   g_blockOutline;
extern int   g_autoJump;
extern int   g_classicPick;
extern int   g_dither;
extern int   g_barOnTop;
extern float g_sensitivity;
extern int   g_fineAim;
extern int   g_thirdPerson;
extern int   g_invertY;
extern int   g_southpaw;
extern int   g_beautifulSkies;
extern int   g_animateTextures;
extern int   g_hideGui;
extern float g_hudOpacity;
extern int   g_particles;
extern float g_analogDeadzone;
extern bool  g_worldBuilt;
extern bool  g_paused;
extern bool  g_optionsOpen;
extern World g_world;

#define CAT_GRAPHICS    2
#define ROW_RENDERDIST  0
#define ROW_CLOUDS      1
#define ROW_LEAVES      2
#define ROW_BOBBING     3
#define ROW_SKIES       4
#define ROW_ANIMTEX     5
#define ROW_PARTICLES   6
#define ROW_SMOOTHLIGHT 7
#define ROW_BRIGHTNESS  8
#define ROW_MIPMAP      9
#define ROW_DITHER      10

static const float kRenderDist[4] = { 16.0f, 32.0f, 48.0f, 64.0f };
extern int g_lowMemPsp;

static int renderDistChoices() { return g_lowMemPsp ? 2 : 4; }

#define CAT_CONTROLS     1
#define ROW_SENS         0
#define ROW_DEADZONE     1
#define ROW_AUTOJUMP     2
#define ROW_JPLAYOUT     3
#define ROW_CLASSICPICK  4
#define ROW_SCHEME       5

#define CAT_GAME        0
#define ROW_DIFFICULTY  0
#define ROW_THIRDPERSON 1
#define ROW_AUTOSAVE    2
#define ROW_BARONTOP    3
#define ROW_SHOWFPS     4
#define ROW_SHOWCOORDS  5
#define ROW_BLOCKOUTLINE 6
#define ROW_HIDEGUI      7
#define ROW_HUDOPACITY   8

#define CAT_AUDIO       3
#define ROW_SOUNDVOL    0
#define ROW_CATVOL0     1
static const int kAutosaveTicks[5] = { 0, 6000, 18000, 24000, 36000 };

static const struct { const char* label; int* value; int def; } kPageSettings[] = {
    { "Invert Look", &g_invertY,  0 },
    { "Southpaw",    &g_southpaw, 0 },
    { "Fine Aim",    &g_fineAim,  1 },
};
#define PAGE_SETTINGS (int)(sizeof(kPageSettings) / sizeof(kPageSettings[0]))

void optionsSetInvertLook(int on) { g_invertY  = on ? 1 : 0; }
void optionsSetSouthpaw(int on)   { g_southpaw = on ? 1 : 0; }
void optionsSetFineAim(int on)    { g_fineAim  = on ? 1 : 0; }

unsigned int optionsValueSig() {
    unsigned int h = 2166136261u;
    for (int c = 0; c < OPT_CATEGORIES; c++)
        for (int r = 0; r < g_optionRowCount[c]; r++)
            h = (h ^ (unsigned int)g_optionValueIdx[c][r]) * 16777619u;
    return h;
}

static float s_meshedGamma = 0.0f;

static void optionsApply() {

    int leaves = g_optionValueIdx[CAT_GRAPHICS][ROW_LEAVES];
    g_fancyGraphics = (leaves > 0);
    g_fancyLeaves   = (leaves == 2);
    g_cloudMode     = g_optionValueIdx[CAT_GRAPHICS][ROW_CLOUDS];
    g_viewBobbing = g_optionValueIdx[CAT_GRAPHICS][ROW_BOBBING];
    int rd = g_optionValueIdx[CAT_GRAPHICS][ROW_RENDERDIST];
    if (rd < 0) rd = 0; else if (rd > 3) rd = 3;

    if (rd >= renderDistChoices()) rd = 0;
    g_optionValueIdx[CAT_GRAPHICS][ROW_RENDERDIST] = rd;
    g_viewDist    = kRenderDist[rd];
    g_noMipmap    = (g_optionValueIdx[CAT_GRAPHICS][ROW_MIPMAP] == 1) ? 0 : 1;
    g_showFps     = g_optionValueIdx[CAT_GAME][ROW_SHOWFPS];
    g_showCoords  = g_optionValueIdx[CAT_GAME][ROW_SHOWCOORDS];
    g_barOnTop    = g_optionValueIdx[CAT_GAME][ROW_BARONTOP];
    int ai = g_optionValueIdx[CAT_GAME][ROW_AUTOSAVE];
    if (ai < 0) ai = 0; else if (ai > 4) ai = 4;
    g_autosave    = kAutosaveTicks[ai];
    g_blockOutline = g_optionValueIdx[CAT_GAME][ROW_BLOCKOUTLINE];
    g_autoJump     = g_optionValueIdx[CAT_CONTROLS][ROW_AUTOJUMP];
    g_japaneseLayout = g_optionValueIdx[CAT_CONTROLS][ROW_JPLAYOUT];
    g_classicPick    = g_optionValueIdx[CAT_CONTROLS][ROW_CLASSICPICK];
    g_dither       = g_optionValueIdx[CAT_GRAPHICS][ROW_DITHER];
    g_difficulty  = g_optionValueIdx[CAT_GAME][ROW_DIFFICULTY];
    soundSetVolume(g_optionValueIdx[CAT_AUDIO][ROW_SOUNDVOL] / 10.0f);
    for (int c = 0; c < SND_CAT_COUNT; c++)
        soundSetCategoryVolume(c, g_optionValueIdx[CAT_AUDIO][ROW_CATVOL0 + c] / 10.0f);

    g_sensitivity = g_optionValueIdx[CAT_CONTROLS][ROW_SENS] / 10.0f;
    g_controlScheme = g_optionValueIdx[CAT_CONTROLS][ROW_SCHEME];

    g_analogDeadzone = g_optionValueIdx[CAT_CONTROLS][ROW_DEADZONE] * 0.05f;
    int tp = g_optionValueIdx[CAT_GAME][ROW_THIRDPERSON];
    g_thirdPerson = (tp < 0 || tp > 2) ? 0 : tp;
    g_beautifulSkies = g_optionValueIdx[CAT_GRAPHICS][ROW_SKIES];
    g_animateTextures= g_optionValueIdx[CAT_GRAPHICS][ROW_ANIMTEX];
    g_hideGui        = g_optionValueIdx[CAT_GAME][ROW_HIDEGUI];

    g_hudOpacity     = g_optionValueIdx[CAT_GAME][ROW_HUDOPACITY] / 10.0f;

    int wantParticles = g_optionValueIdx[CAT_GRAPHICS][ROW_PARTICLES];
    if (!wantParticles && g_particles) particlesReset();
    g_particles = wantParticles;

    bool wantSmooth = g_optionValueIdx[CAT_GRAPHICS][ROW_SMOOTHLIGHT] != 0;
    if (wantSmooth != g_smoothLighting) {
        g_smoothLighting = wantSmooth;
        if (g_worldBuilt) worldMarkAllDirty(&g_world);
    }

    float wantGamma = (g_optionValueIdx[CAT_GRAPHICS][ROW_BRIGHTNESS] / 10.0f) * BRIGHT_GAMMA_MAX;
    if (wantGamma != g_brightGamma) {
        g_brightGamma = wantGamma;
        chunkInitBrightRamp();
    }

    if (!g_worldBuilt) s_meshedGamma = g_brightGamma;
}

static void optionsBrightnessCommit() {
    if (s_meshedGamma == g_brightGamma) return;
    s_meshedGamma = g_brightGamma;
    if (g_worldBuilt) worldMarkAllDirty(&g_world);
}

static void optionsSetDefaults() {
    for (int c = 0; c < OPT_CATEGORIES; c++)
        for (int r = 0; r < g_optionRowCount[c]; r++)
            g_optionValueIdx[c][r] = g_optionRows[c][r].def;
    for (int i = 0; i < PAGE_SETTINGS; i++)
        *kPageSettings[i].value = kPageSettings[i].def;

    if (g_lowMemPsp) {
        g_optionValueIdx[CAT_GRAPHICS][ROW_SKIES]       = 0;
        g_optionValueIdx[CAT_GRAPHICS][ROW_CLOUDS]      = 0;
        g_optionValueIdx[CAT_GRAPHICS][ROW_SMOOTHLIGHT] = 0;
    }
}

void optionsInitDefaults() {
    optionsSetDefaults();
    optionsApply();
}

void optionsCycleCameraMode() {
    int tp = g_optionValueIdx[CAT_GAME][ROW_THIRDPERSON] + 1;
    g_optionValueIdx[CAT_GAME][ROW_THIRDPERSON] = (tp < 0 || tp > 2) ? 0 : tp;
    optionsApply();
}

void optionsSetControlScheme(int scheme) {
    if (scheme < 0 || scheme >= CONTROL_SCHEMES) return;
    g_optionValueIdx[CAT_CONTROLS][ROW_SCHEME] = scheme;
    optionsApply();
}

static const char* optionsFile() { return savePath("options.txt"); }

void optionsSave() {
    FILE* f = fopen(optionsFile(), "w");
    if (!f) return;
    for (int c = 0; c < OPT_CATEGORIES; c++)
        for (int r = 0; r < g_optionRowCount[c]; r++)
            fprintf(f, "%s=%d\n", g_optionRows[c][r].label, g_optionValueIdx[c][r]);

    for (int i = 0; i < PAGE_SETTINGS; i++)
        fprintf(f, "%s=%d\n", kPageSettings[i].label, *kPageSettings[i].value);
    fclose(f);
}

void optionsLoad() {
    optionsSetDefaults();
    FILE* f = fopen(optionsFile(), "r");

    if (f) {
        char line[128];
        while (fgets(line, sizeof(line), f)) {
            char* eq = strrchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            int val = atoi(eq + 1);
            if (strcmp(line, "Sound Volume") == 0) strcpy(line, "Master");

            if (strcmp(line, "Autosave") == 0) {
                strcpy(line, "Auto Save");
                if (val > 0) val++;
            }

            {
                bool claimed = false;
                for (int i = 0; i < PAGE_SETTINGS && !claimed; i++)
                    if (strcmp(line, kPageSettings[i].label) == 0) {
                        *kPageSettings[i].value = val ? 1 : 0;
                        claimed = true;
                    }
                if (claimed) continue;
            }
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

#define OPT_CAT_BTN    26.0f
#define OPT_CAT_PITCH  27.0f
#define OPT_CAT_X       4.0f

#define OPT_CAT_ICON   24.0f
static const float kCatIconUV[OPT_CATEGORIES][2] = {
    { OPT_CAT_ICON, 0.0f }, { 0.0f, 0.0f },
    { OPT_CAT_ICON, OPT_CAT_ICON }, { 0.0f, OPT_CAT_ICON },
};

static const float kOptRowH    = 16.0f;
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

static float optionPaneHeight(int category) {
    float h = 0.0f;
    for (int r = 0; r < g_optionRowCount[category]; r++) {
        if (g_optionRows[category][r].group) h += kOptHeaderH;
        h += kOptRowH;
    }
    return h;
}

static float optionRowTop(int category, int row) {
    float y = optionRowY(category, row, 0.0f);
    if (g_optionRows[category][row].group) y -= kOptHeaderH;
    return y;
}
static float optionRowSpan(int category, int row) {
    return kOptRowH + (g_optionRows[category][row].group ? kOptHeaderH : 0.0f);
}

static int rowValueCount(int category, int row) {
    if (category == CAT_GRAPHICS && row == ROW_RENDERDIST) return renderDistChoices();
    return g_optionRows[category][row].numValues;
}

static bool optionRowIsBoolean(const OptionRowDef& row) {
    return row.numValues == 2 && strcmp(row.values[0], "Off") == 0 && strcmp(row.values[1], "On") == 0;
}

static bool optionRowDisabled(int category, int row) {
    return category == CAT_GRAPHICS && row == ROW_CLOUDS &&
           g_optionValueIdx[CAT_GRAPHICS][ROW_SKIES] == 0;
}
struct OptionsScreen : Screen {
    void renderContent(MenuState& s);
    void handleInput(MenuState& s, unsigned int pressed, unsigned int held);
};

void OptionsScreen::handleInput(MenuState& s, unsigned int pressed, unsigned int ) {

    if (controlsPageIsOpen()) { controlsPageInput(s, pressed); return; }

    int& optFocus = s.optFocus;
    int& optCategory = s.optCategory;
    int& optTabHighlight = s.optTabHighlight;
    int& optItemHighlight = s.optItemHighlight;
    AppScreen& screen = s.screen;

    int rowCount = g_optionRowCount[optCategory];

    optFocus = 1;
    if (pressed & (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER)) {
        optCategory += (pressed & PSP_CTRL_LTRIGGER) ? -1 : 1;
        if (optCategory < 0) optCategory = OPT_CATEGORIES - 1;
        else if (optCategory >= OPT_CATEGORIES) optCategory = 0;
        optItemHighlight = 0; s.optScroll = 0.0f;
    }
    optTabHighlight = optCategory;
    rowCount = g_optionRowCount[optCategory];

    if ((pressed & PSP_CTRL_UP)   && optItemHighlight > 0)            optItemHighlight--;
    if ((pressed & PSP_CTRL_DOWN) && optItemHighlight < rowCount - 1) optItemHighlight++;

    if (pressed & PSP_CTRL_CIRCLE) {
        optionsSave();
        optionsBrightnessCommit();

        if (g_optionsOpen) { g_optionsOpen = false; g_paused = true; }
        else               screen = SCREEN_TITLE;
    }

    if (!optionRowDisabled(optCategory, optItemHighlight)) {
        const OptionRowDef& row = g_optionRows[optCategory][optItemHighlight];
        int  idx  = g_optionValueIdx[optCategory][optItemHighlight];
        int  nVal = rowValueCount(optCategory, optItemHighlight);

        if ((pressed & PSP_CTRL_LEFT) && idx > 0) {
            g_optionValueIdx[optCategory][optItemHighlight] = idx - 1;
            optionsApply();
        }
        if ((pressed & PSP_CTRL_RIGHT) && idx < nVal - 1) {
            g_optionValueIdx[optCategory][optItemHighlight] = idx + 1;
            optionsApply();
        }
        if ((pressed & PSP_CTRL_CROSS) && row.button) controlsPageOpen();

        if ((pressed & PSP_CTRL_CROSS) && optionRowIsBoolean(row)) {
            g_optionValueIdx[optCategory][optItemHighlight] ^= 1;
            optionsApply();
        }
    }
}

void OptionsScreen::renderContent(MenuState& s) {
    if (controlsPageIsOpen()) { controlsPageRender(s); return; }

    Font& font = s.font; bool haveFont = s.haveFont;

    bool haveArt = s.haveGui;
    int& optFocus = s.optFocus;
    int& optCategory = s.optCategory;
    int& optItemHighlight = s.optItemHighlight;

    if (haveArt && haveFont) {
        sceGuDisable(GU_DEPTH_TEST);

        drawRect(0.0f, 0.0f, (OPT_CAT_BTN + 10.0f) * UI_SCALE, VH * UI_SCALE, 0xFF828795u);

        float barBtnH = MENU_BAR_H;
        {
            float lb = 4.0f * MENU_PX + menuBarButtonW(s, "Back");
            drawMenuHeader(s, "Options", 0.0f, VW, MENU_BAR_H, MENU_BAR_TEXT, lb, VW - lb);
        }
        {
            float bw = menuBarButtonW(s, "Back");
            menuBarButton(s, 4.0f * MENU_PX, bw, "Back", optFocus == 2);
        }

        const float catBtn = OPT_CAT_BTN, catPitch = OPT_CAT_PITCH;

        float catSpan = catBtn * OPT_CATEGORIES + (catPitch - catBtn) * (OPT_CATEGORIES - 1);

        const float hintsTop = UI_HINTS_Y / UI_SCALE - 1.0f;
        float catY0 = floorf(barBtnH + 3.0f + ((hintsTop - barBtnH - 3.0f) - catSpan) / 2.0f);
        for (int i = 0; i < OPT_CATEGORIES; i++) {
            bool tabActive = (optCategory == i);
            float cY = catY0 + i * catPitch;

            drawNinePatch(s, GA_SS_SLOT_X + (tabActive ? 0.0f : 8.0f), GA_SS_SLOT_Y, 8.0f, 8.0f, 2.0f,
                          OPT_CAT_X, cY, catBtn, catBtn);
            if (s.haveGui) {

                textureBind(&s.guiAtlas);

                const float isz = OPT_CAT_ICON * UI_SCALE;
                const float ix = floorf(OPT_CAT_X * UI_SCALE + (catBtn * UI_SCALE - isz) / 2.0f);
                float iy = floorf(cY * UI_SCALE + (catBtn * UI_SCALE - isz) / 2.0f);

                if (i == CAT_AUDIO) iy += 2.0f;
                spriteDraw(&s.guiAtlas, ix, iy, isz, isz,
                           GA_SS_OPTCAT_X + kCatIconUV[i][0],
                           GA_SS_OPTCAT_Y + kCatIconUV[i][1], OPT_CAT_ICON, OPT_CAT_ICON, WHITE);
            } else {

                float tw = fontTextWidth(&font, g_optionCategoryNames[i]) * UI_SCALE;
                fontDrawTextShadow(&font, OPT_CAT_X * UI_SCALE + (catBtn * UI_SCALE - tw) / 2.0f,
                                   (cY + (catBtn - 8.0f) / 2.0f) * UI_SCALE,
                                   g_optionCategoryNames[i], tabActive ? 0xFFA0FFFFu : 0xFFE0E0E0u, UI_SCALE);
            }
        }

        int rowCount = g_optionRowCount[optCategory];
        float itemsX = OPT_CAT_BTN + 20.0f;
        float itemsW = VW - itemsX - 6.0f;
        float rowH = kOptRowH;
        float paneY0 = barBtnH + 3.0f;

        float paneH  = (UI_HINTS_Y / UI_SCALE - 1.0f) - paneY0 + 3.0f / UI_SCALE;
        float contentH = optionPaneHeight(optCategory);

        int firstRow = (int)s.optScroll;
        if (firstRow < 0) firstRow = 0;
        if (firstRow > rowCount - 1) firstRow = rowCount - 1;

        if (firstRow > optItemHighlight) firstRow = optItemHighlight;

        {
            float selBottom = optionRowTop(optCategory, optItemHighlight) +
                              optionRowSpan(optCategory, optItemHighlight);
            while (firstRow < optItemHighlight &&
                   selBottom - optionRowTop(optCategory, firstRow) > paneH)
                firstRow++;
        }

        while (firstRow > 0) {
            float h = 0.0f;
            for (int r = firstRow - 1; r < rowCount; r++) h += optionRowSpan(optCategory, r);
            if (h > paneH) break;
            firstRow--;
        }
        float scroll = optionRowTop(optCategory, firstRow);
        s.optScroll = (float)firstRow;
        float rowY0 = paneY0 - scroll;

        float visH = 0.0f;
        for (int r = firstRow; r < rowCount; r++) {
            float span = optionRowSpan(optCategory, r);
            if (visH + span > paneH) break;
            visH += span;
        }
        if (visH <= 0.0f) visH = paneH;

        sceGuScissor((int)(itemsX * UI_SCALE), (int)(paneY0 * UI_SCALE),
                     (int)((VW - itemsX) * UI_SCALE), (int)(visH * UI_SCALE));
        for (int r = 0; r < rowCount; r++) {
            const OptionRowDef& row = g_optionRows[optCategory][r];
            int valIdx = g_optionValueIdx[optCategory][r];
            float rY = optionRowY(optCategory, r, rowY0);
            if (rY > paneY0 + visH || rY + rowH < paneY0 - kOptHeaderH) continue;

            if (row.group)
                fontDrawTextShadow(&font, (itemsX + 2.0f) * UI_SCALE, (rY - kOptHeaderH + 2.0f) * UI_SCALE,
                                   row.group, 0xFFFFFFFFu, UI_SCALE);
            bool rowHovered = (optFocus == 1 && optItemHighlight == r);
            bool rowDisabled = optionRowDisabled(optCategory, r);

            unsigned int labelCol = rowDisabled ? 0xFF707070u : (rowHovered ? 0xFFFFFFFFu : 0xFFBBBBBBu);
            unsigned int togTint  = rowDisabled ? 0xFF707070u : WHITE;

            const float kWidgetMargin = 6.0f;
            const float togW = TOGGLE_CELL_W, togH = TOGGLE_CELL_H;

            const float sliderW = 60.0f, sliderH = kOptRowH;
            bool isBool = optionRowIsBoolean(row);
            int  nVals  = rowValueCount(optCategory, r);

            char valBuf[16];
            const char* valTxt = 0;

            const int nCap   = (int)(sizeof(row.values) / sizeof(row.values[0]));
            const int nShown = (nVals < nCap) ? nVals : nCap;
            if (row.button) {
                valTxt = (valIdx >= 0 && valIdx < nShown) ? row.values[valIdx] : 0;
            } else if (!isBool) {
                if (row.percent) {
                    snprintf(valBuf, sizeof(valBuf), "%d%%", row.percentMin + valIdx * row.percentStep);
                    valTxt = valBuf;
                } else if (valIdx >= 0 && valIdx < nShown) {
                    valTxt = row.values[valIdx];
                }
            }

            const float btnW = row.button ? fontTextWidth(&font, "Open") + 10.0f : 0.0f;
            float widgetX  = itemsX + itemsW - (row.button ? btnW : isBool ? togW : sliderW) - kWidgetMargin;
            float valW     = valTxt ? fontTextWidth(&font, valTxt) : 0.0f;
            float labelMax = (widgetX - itemsX) - (valTxt ? valW + 8.0f : 4.0f);
            if (labelMax < 8.0f) labelMax = 8.0f;

            fontDrawTextClipped(&font, itemsX * UI_SCALE, (rY + (rowH - 8.0f) / 2.0f) * UI_SCALE,
                                row.label, labelCol, UI_SCALE, labelMax);

            if (row.button) {

                const float bh = kOptRowH - 2.0f;
                const float by = rY + 1.0f;
                guiTButton(s, widgetX, by, btnW, bh, rowHovered, MENU_BEVEL);
                guiTButtonLabel(s, widgetX, by, btnW, bh, "Open", rowHovered, !rowDisabled);
                if (valTxt)
                    fontDrawTextShadow(&font, (widgetX - 4.0f) * UI_SCALE - valW * UI_SCALE,
                                       (rY + (rowH - 8.0f) / 2.0f) * UI_SCALE, valTxt,
                                       rowHovered ? 0xFFFFFFFFu : 0xFFBBBBBBu, UI_SCALE);
            } else if (isBool) {

                guiOptionSwitch(s, widgetX, rY + (rowH - togH) / 2.0f, togW, togH,
                                valIdx == 1, rowHovered, togTint, 2.0f);
            } else {

                const float K = 2.0f;
                float sliderX = widgetX;
                float sliderY = rY + (rowH - sliderH) / 2.0f;

                const float cy       = floorf((sliderY + sliderH * 0.5f) * UI_SCALE);
                const float trackX0  = sliderX * UI_SCALE + 5.0f * K;
                const float barWidth = sliderW * UI_SCALE - 10.0f * K;

                drawRect(trackX0, cy - 1.5f * K, barWidth, 3.0f * K, 0xFF706C70u);

                if (!row.percent && nVals > 2) {
                    float step = barWidth / (float)(nVals - 1);
                    float tx = trackX0 - 1.0f * K;
                    for (int i = 0; i < nVals; i++, tx += step)
                        drawRect(floorf(tx), cy - 3.5f * K, 4.0f * K, 7.0f * K, 0xFF908590u);
                }

                float progress = (nVals > 1) ? (float)valIdx / (float)(nVals - 1) : 0.0f;
                float knobCx = trackX0 + barWidth * progress;
                uiDraw(s, floorf(knobCx - KNOB_CELL_W * K * 0.5f),
                       cy - KNOB_CELL_H * K * 0.5f,
                       KNOB_CELL_W * K, KNOB_CELL_H * K, 225.0f, 125.0f,
                       GA_SS_SLIDER_KNOB_X, GA_SS_SLIDER_KNOB_Y,
                       KNOB_CELL_W, KNOB_CELL_H, WHITE);

                if (valTxt)
                    fontDrawTextShadow(&font, (sliderX - 4.0f) * UI_SCALE - valW * UI_SCALE,
                                       (rY + (rowH - 8.0f) / 2.0f) * UI_SCALE, valTxt,
                                       rowHovered ? 0xFFFFFFFFu : 0xFFBBBBBBu, UI_SCALE);
            }
        }
        sceGuScissor(0, 0, 480, 272);

        guiScrollbar((VW - 3.0f) * UI_SCALE, paneY0 * UI_SCALE, 2.0f * UI_SCALE,
                     paneH * UI_SCALE, contentH * UI_SCALE, scroll * UI_SCALE);

        sceGuEnable(GU_DEPTH_TEST);
    }
}

static OptionsScreen s_optionsScreen;
Screen& optionsScreen() { return s_optionsScreen; }
