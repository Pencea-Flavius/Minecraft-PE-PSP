
#include <pspgu.h>
#include <pspctrl.h>
#include <psputility.h>
#include <cstdio>
#include <cstring>

#include "client/gui/screens/menu.h"
#include "gpu/gu.h"
#include "gpu/button_icons.h"
#include "world/level/tile/entity/sign_tile_entity.h"

unsigned int g_heldButtons = 0;

unsigned int menuSelectionSig(const MenuState& s) {
    unsigned int h = 2166136261u;
    const int fields[] = {
        (int)s.screen, s.worldSelected, s.deleteSelected, s.createSelected,
        s.newWorldGamemode, s.uiRow, s.topSelected, s.selected,
        s.joinListSelected, s.joinIpSelected,
        s.optFocus, s.optCategory, s.optTabHighlight, s.optItemHighlight,
        s.optEditingRow,
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); i++)
        h = (h ^ (unsigned int)fields[i]) * 16777619u;
    return (h ^ optionsValueSig()) * 16777619u;
}

struct ColorVertex {
    unsigned int color;
    float x, y, z;
};

void drawRect(float x, float y, float w, float h, unsigned int color) {
    sceGuDisable(GU_TEXTURE_2D);
    ColorVertex* vertices = (ColorVertex*)sceGuGetMemory(2 * sizeof(ColorVertex));
    vertices[0].color = color;
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0.0f;
    vertices[1].color = color;
    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].z = 0.0f;

    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, vertices);
    sceGuEnable(GU_TEXTURE_2D);
}

#include "gpu/sprite.h"
#include "gpu/gui_atlas.h"

void drawNinePatch(MenuState& s, float sx, float sy, float sw, float sh, float corner,
                   float gx, float gy, float gw, float gh) {
    if (!s.haveGui) {
        drawRect(gx * UI_SCALE, gy * UI_SCALE, gw * UI_SCALE, gh * UI_SCALE, 0xFF444444u);
        return;
    }
    const float c = corner, S = UI_SCALE;
    const float xs[4] = { 0, c, gw - c, gw };
    const float us[4] = { 0, c, sw - c, sw };
    const float ys[4] = { 0, c, gh - c, gh };
    const float vs[4] = { 0, c, sh - c, sh };
    textureBind(&s.guiAtlas);
    for (int j = 0; j < 3; ++j)
        for (int i = 0; i < 3; ++i) {
            float dw = xs[i + 1] - xs[i], dh = ys[j + 1] - ys[j];
            if (dw <= 0 || dh <= 0) continue;
            spriteDraw(&s.guiAtlas, (gx + xs[i]) * S, (gy + ys[j]) * S, dw * S, dh * S,
                       sx + us[i], sy + vs[j], us[i + 1] - us[i], vs[j + 1] - vs[j], WHITE);
        }
}

void fontDrawTextClipped(const Font* font, float x, float y, const char* text,
                         unsigned int color, float scale, float maxWidthRaw) {
    if (fontTextWidth(font, text) <= maxWidthRaw) {
        fontDrawTextShadow(font, x, y, text, color, scale);
        return;
    }
    char base[68];
    int len = (int)strlen(text);
    if (len > 64) len = 64;
    memcpy(base, text, len);
    base[len] = '\0';

    char out[72];
    while (len > 0) {
        base[len] = '\0';
        snprintf(out, sizeof(out), "%s...", base);
        if (fontTextWidth(font, out) <= maxWidthRaw) {
            fontDrawTextShadow(font, x, y, out, color, scale);
            return;
        }
        len--;
    }
    fontDrawTextShadow(font, x, y, "...", color, scale);
}

void fontDrawTextWrapped(const Font* font, float x, float y, const char* text,
                         unsigned int color, float scale, float maxWidthRaw) {
    float lineH = font->lineHeight * scale;
    char line[96]; line[0] = '\0';
    char word[64];
    const char* p = text;
    while (*p) {
        while (*p == ' ') p++;
        int wl = 0;
        while (*p && *p != ' ' && wl < 63) word[wl++] = *p++;
        word[wl] = '\0';
        if (wl == 0) break;

        char trial[96];
        if (line[0]) snprintf(trial, sizeof(trial), "%s %s", line, word);
        else         snprintf(trial, sizeof(trial), "%s", word);

        if (!line[0] || fontTextWidth(font, trial) <= maxWidthRaw) {
            snprintf(line, sizeof(line), "%s", trial);
        } else {
            fontDrawTextShadow(font, x, y, line, color, scale);
            y += lineH;
            snprintf(line, sizeof(line), "%s", word);
        }
    }
    if (line[0]) fontDrawTextShadow(font, x, y, line, color, scale);
}

static bool oskActive = false;
static int oskTarget = 0;

static SignTileEntity* g_oskSign = 0;
static int g_oskSignLine = 0;
static const int OSK_TARGET_SIGN = 4;
static unsigned short oskDesc[128];
static unsigned short oskInText[128];
static unsigned short oskOutText[128];
static SceUtilityOskData oskData;
static SceUtilityOskParams oskParams;

void startOsk(int target, const char* desc, const char* intext, int maxLen) {
    oskTarget = target;
    memset(&oskData, 0, sizeof(oskData));
    memset(&oskParams, 0, sizeof(oskParams));
    memset(oskDesc, 0, sizeof(oskDesc));
    memset(oskInText, 0, sizeof(oskInText));
    memset(oskOutText, 0, sizeof(oskOutText));

    for (int i = 0; desc[i] && i < 127; i++) oskDesc[i] = desc[i];
    for (int i = 0; intext[i] && i < 127; i++) oskInText[i] = intext[i];

    oskData.language = PSP_UTILITY_OSK_LANGUAGE_DEFAULT;
    oskData.lines = 1;
    oskData.unk_24 = 1;

    oskData.inputtype = PSP_UTILITY_OSK_INPUTTYPE_ALL;
    oskData.desc = oskDesc;
    oskData.intext = oskInText;
    if (maxLen > 64) maxLen = 64;
    oskData.outtextlength = maxLen;
    oskData.outtextlimit = maxLen;
    oskData.outtext = oskOutText;

    oskParams.base.size = sizeof(oskParams);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &oskParams.base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &oskParams.base.buttonSwap);
    oskParams.base.graphicsThread = 17;
    oskParams.base.accessThread = 19;
    oskParams.base.fontThread = 18;
    oskParams.base.soundThread = 16;
    oskParams.datacount = 1;
    oskParams.data = &oskData;

    sceUtilityOskInitStart(&oskParams);
    oskActive = true;
}

void signStartEdit(SignTileEntity* ste) {
    if (!ste) return;
    g_signEditing = ste;
    ste->selectedLine = 0;
}

void signEditLine(int line) {
    if (!g_signEditing || line < 0 || line >= SignTileEntity::NUM_LINES) return;
    g_oskSign = g_signEditing;
    g_oskSignLine = line;
    char desc[16];
    snprintf(desc, sizeof(desc), "Sign line %d:", line + 1);
    startOsk(OSK_TARGET_SIGN, desc, g_oskSign->messages[line].c_str(),
             SignTileEntity::MAX_LINE_LENGTH);
}

bool menuOskUpdate(MenuState& s) {
    if (!oskActive) return false;

    int status = sceUtilityOskGetStatus();

    if (status == PSP_UTILITY_DIALOG_QUIT ||
        status == PSP_UTILITY_DIALOG_FINISHED) {
        sceUtilityOskShutdownStart();
    } else if (status == PSP_UTILITY_DIALOG_NONE) {

        oskActive = false;

        if (oskTarget == OSK_TARGET_SIGN) {
            char line[SignTileEntity::MAX_LINE_LENGTH + 1];
            int n = 0;
            for (int i = 0; i < SignTileEntity::MAX_LINE_LENGTH && oskOutText[i]; i++)
                line[n++] = (char)oskOutText[i];
            line[n] = 0;
            if (g_oskSign) g_oskSign->messages[g_oskSignLine] = line;
            g_oskSign = 0;
            return true;
        }

        char* targetStr = (oskTarget == 1) ? s.newWorldName : (oskTarget == 2) ? s.newWorldSeed : s.joinIp;

        for (int i = 0; i < 63 && oskOutText[i]; i++) {
            targetStr[i] = (char)oskOutText[i];
            targetStr[i+1] = 0;
        }
        if (oskOutText[0] == 0) targetStr[0] = 0;
        return true;
    }

    guStartFrame(0xFF000000u);
    guFinishFrame();
    if (status == PSP_UTILITY_DIALOG_VISIBLE)
        sceUtilityOskUpdate(1);

    guPresent();
    return true;
}

void buttonHintsDraw(MenuState& s, const ButtonHint* hints, int n, float y, float scale) {
    if (!s.haveGui || !s.haveFont) return;

    float x = 6.0f;
    for (int i = 0; i < n; i++) {
        const ButtonIconRect& r = buttonIconRect(hints[i].icon);
        bool held = (g_heldButtons & hints[i].btn) != 0;
        buttonIconDraw(&s.guiAtlas, hints[i].icon, x, y, scale,
                       held ? 0xC8C0C0C0u : 0xC8FFFFFFu);
        x += r.w * scale + 3.0f * scale;
        if (hints[i].label[0]) {
            fontDrawTextShadow(&s.font, x, y + (r.h * scale - 8.0f * scale) * 0.5f, hints[i].label,
                               0xFFE0E0E0u, scale);
            x += fontTextWidth(&s.font, hints[i].label) * scale + 10.0f * scale;
        }
    }
}

void menuHintsDraw(MenuState& s) {
    ButtonHint hints[4];
    int n = 0;
    hints[n++] = (ButtonHint){ BTN_ICON_CROSS,  PSP_CTRL_CROSS,  "Select" };
    if (s.screen != SCREEN_TITLE)
        hints[n++] = (ButtonHint){ BTN_ICON_CIRCLE, PSP_CTRL_CIRCLE, "Back" };
    if (s.screen == SCREEN_OPTIONS) {
        hints[n++] = (ButtonHint){ BTN_ICON_L, PSP_CTRL_LTRIGGER, "" };
        hints[n++] = (ButtonHint){ BTN_ICON_R, PSP_CTRL_RTRIGGER, "Change Group" };
    }
    buttonHintsDraw(s, hints, n);
}

#include "world/level/tile/tile_gui_hooks.h"
#include "world/item/crafting/recipe.h"
#include "client/gui/hud.h"

void guiOpenSignEditor(SignTileEntity* te) { if (te) signStartEdit(te); }
void guiOpenFurnace(FurnaceTileEntity* te) { if (te) furnaceOpen(te); }
void guiOpenChest(ChestTileEntity* te)     { if (te) chestOpen(te); }
void guiOpenCrafting(bool stonecutter) {
    craftOpen(Recipe::SIZE_3X3, stonecutter ? CRAFT_STONECUTTER : CRAFT_WORKBENCH);
}
void guiChatMessage(const char* msg) { hudChatMessage(msg); }
