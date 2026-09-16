
#include <pspctrl.h>
#include <pspgu.h>
#include <pspiofilemgr.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "client/gui/screens/skin_page.h"
#include "client/gui/screens/screen.h"
#include "client/renderer/entity/mob_model.h"
#include "client/renderer/entity/player_model.h"
#include "client/skin/skin_pack.h"
#include "gpu/font.h"
#include "gpu/gui_atlas.h"
#include "gpu/sprite.h"
#include "gpu/texture.h"
#include "platform/path.h"

#define NSLOT 7

#define SK_TOP_SHIFT   23

#define SK_TINT_Y      (52.0f + SK_TOP_SHIFT)
#define SK_TINT_H     167.0f
#define SK_TINT     0x60000000u

#define SK_BAR_Y       (30 + SK_TOP_SHIFT)
#define SK_BAR_BAND_Y  (45 + SK_TOP_SHIFT)
#define SK_STUB_W      26

#define SK_BASE_BAND_Y (212 + SK_TOP_SHIFT)

#define SK_TAB_W      143
#define SK_TAB_Y       (25 + SK_TOP_SHIFT)
#define SK_TAB_BOT     (45 + SK_TOP_SHIFT)
#define SK_TAB_C_DY     4
#define SK_TAB_C_LAP    2
#define SK_TAB_C_GROW   2
static const int kTabX[3] = { 168 - SK_TAB_W, 168, 168 + SK_TAB_W };

struct SlotRect { float x, y, w, h; };
static const SlotRect kSlot[NSLOT] = {
    {  -3.0f,  98.6f, 45.8f, 42.5f },
    {  50.3f,  92.9f, 57.8f, 53.3f },
    { 115.5f,  86.1f, 72.0f, 66.9f },
    { 195.0f,  78.2f, 90.0f, 83.3f },
    { 292.5f,  86.1f, 72.0f, 66.9f },
    { 372.0f,  92.9f, 57.8f, 53.3f },
    { 437.3f,  98.6f, 45.8f, 42.5f },
};

#define SK_SEL_X      156
#define SK_SEL_Y      (163 + SK_TOP_SHIFT)
#define SK_SEL_W      168
#define SK_SEL_H       14

#define SK_NAME_X      98
#define SK_NAME_Y     (178 + SK_TOP_SHIFT)
#define SK_NAME_W     285
#define SK_NAME_H      31

#define LOOK_LEFT_EXTENT      45.0f
#define LOOK_RIGHT_EXTENT    -45.0f
#define CHANGING_SKIN_FRAMES  15

#define MAX_PACKS 32

enum { FOCUS_SKINS, FOCUS_PACKS, FOCUS_BAR };

enum { PACK_DEFAULT = 0, PACK_FAVORITES = 1, PACK_FIRST_FILE = 2 };
#define MAX_OPEN_PACKS (SKIN_MAX_FAVORITES + 1)

struct Entry { short pack; short idx; };

struct Slot {
    int  idx;
    Texture tex;  bool haveTex;
    Texture cape; bool haveCape;
    int  boxCount;
    float rot, rotFrom, rotTo;
    int  frame;
};

static bool s_open = false;
static char s_packFile[MAX_PACKS][64];
static char s_packName[MAX_PACKS][48];
static int  s_packCount = PACK_FIRST_FILE;
static int  s_pack = PACK_DEFAULT;
static int  s_center = 0;
static int  s_focus = FOCUS_SKINS;
static SkinPack s_opened[MAX_OPEN_PACKS];
static char     s_openedFile[MAX_OPEN_PACKS][64];
static int      s_openedCount = 0;
static Entry s_entries[SKIN_MAX_SKINS];
static int   s_entryCount = 0;
static Slot s_store[NSLOT];
static MobVertex (*s_mesh)[36] = 0;
static unsigned char s_part[NSLOT][SKIN_MAX_BOXES];
static float s_ws = 0.0f, s_wp = 0.0f;

bool skinPageIsOpen() { return s_open; }

static int skinCount() { return s_entryCount; }
static int wrap(int i, int n) { return n > 0 ? ((i % n) + n) % n : 0; }

static void freeSlot(Slot& sl) {
    if (sl.haveTex)  textureFree(&sl.tex);
    if (sl.haveCape) textureFree(&sl.cape);
    sl.haveTex = sl.haveCape = false;
    sl.idx = -1;
    sl.boxCount = 0;
}

static void closeOpened() {
    for (int i = 0; i < s_openedCount; i++) skinPackClose(&s_opened[i]);
    s_openedCount = 0;
}

static void freeAll() {
    for (int i = 0; i < NSLOT; i++) freeSlot(s_store[i]);
    closeOpened();
}

static int openedPack(const char* file) {
    for (int i = 0; i < s_openedCount; i++)
        if (strcmp(s_openedFile[i], file) == 0) return i;
    if (s_openedCount >= MAX_OPEN_PACKS) return -1;
    char path[160];
    snprintf(path, sizeof(path), "data/skinpacks/%s", file);
    int i = s_openedCount;
    if (!skinPackOpen(path, &s_opened[i])) return -1;
    snprintf(s_openedFile[i], sizeof(s_openedFile[0]), "%s", file);
    s_openedCount++;
    return i;
}

static void scanPacks() {
    snprintf(s_packName[PACK_DEFAULT], sizeof(s_packName[0]), "No Pack: Default Skins");
    snprintf(s_packName[PACK_FAVORITES], sizeof(s_packName[0]), "Favorite Skins");
    s_packCount = PACK_FIRST_FILE;
    SceUID d = sceIoDopen(assetPath("data/skinpacks"));
    if (d < 0) return;
    SceIoDirent e;
    memset(&e, 0, sizeof(e));
    while (sceIoDread(d, &e) > 0 && s_packCount < MAX_PACKS) {
        size_t n = strlen(e.d_name);
        if (!FIO_S_ISDIR(e.d_stat.st_mode) && n > 4 && n < sizeof(s_packFile[0]) &&
            strcasecmp(e.d_name + n - 4, ".pck") == 0) {
            char path[160], name[48];
            snprintf(path, sizeof(path), "data/skinpacks/%s", e.d_name);
            SkinPack p;
            bool ok = skinPackOpen(path, &p);
            if (ok && p.name[0]) {
                snprintf(name, sizeof(name), "%s", p.name);
            } else {
                snprintf(name, sizeof(name), "%.*s", (int)n - 4, e.d_name);
                for (char* c = name; *c; c++) if (*c == '_') *c = ' ';
            }
            skinPackClose(&p);
            if (ok) {
                int i = s_packCount++;
                while (i > PACK_FIRST_FILE && strcasecmp(s_packName[i - 1], name) > 0) {
                    strcpy(s_packName[i], s_packName[i - 1]);
                    strcpy(s_packFile[i], s_packFile[i - 1]);
                    i--;
                }
                strcpy(s_packName[i], name);
                snprintf(s_packFile[i], sizeof(s_packFile[0]), "%s", e.d_name);
            }
        }
        memset(&e, 0, sizeof(e));
    }
    sceIoDclose(d);
}

static float restRot(int k) {
    return k < 3 ? LOOK_RIGHT_EXTENT : k > 3 ? LOOK_LEFT_EXTENT : 0.0f;
}

static int positionIdx(int k) {
    int n = skinCount();
    static const int order[NSLOT] = { 3, 4, 2, 5, 1, 6, 0 };
    int shown[NSLOT], ns = 0;
    for (int o = 0; o < NSLOT; o++) {
        int pos = order[o];
        int idx = wrap(s_center + pos - 3, n);
        bool dup = false;
        for (int j = 0; j < ns; j++) if (shown[j] == idx) dup = true;
        if (pos == k) return dup || n == 0 ? -1 : idx;
        if (!dup) shown[ns++] = idx;
    }
    return -1;
}

static Slot* findSlot(int idx) {
    for (int i = 0; i < NSLOT; i++) if (s_store[i].idx == idx) return &s_store[i];
    return 0;
}

static void loadInto(int si, int idx, float rot) {
    Slot& sl = s_store[si];
    freeSlot(sl);
    sl.idx = idx;
    sl.rot = sl.rotFrom = sl.rotTo = rot;
    sl.frame = CHANGING_SKIN_FRAMES;
    const Entry& en = s_entries[idx];
    if (en.pack < 0) {
        sl.haveTex = skinDefaultLoadTexture(en.idx, &sl.tex);
        return;
    }
    const SkinPack& pk = s_opened[en.pack];
    sl.haveTex  = skinPackLoadTexture(pk, en.idx, &sl.tex);
    sl.haveCape = skinPackLoadCape(pk, en.idx, &sl.cape);
    const SkinEntry& e = pk.skins[en.idx];
    if (s_mesh)
        sl.boxCount = playerModelBuildSkinBoxes(pk.boxes ? pk.boxes + e.boxFirst : 0,
                                                e.boxCount, s_mesh + si * SKIN_MAX_BOXES,
                                                s_part[si], SKIN_MAX_BOXES, (float)e.texH);
}

static void refreshSlots(bool animate) {
    int want[NSLOT];
    for (int k = 0; k < NSLOT; k++) want[k] = positionIdx(k);
    for (int i = 0; i < NSLOT; i++) {
        bool used = false;
        for (int k = 0; k < NSLOT; k++) if (want[k] >= 0 && want[k] == s_store[i].idx) used = true;
        if (!used) freeSlot(s_store[i]);
    }
    for (int k = 0; k < NSLOT; k++) {
        if (want[k] < 0) continue;
        if (Slot* sl = findSlot(want[k])) {
            float to = restRot(k);
            if (sl->rotTo != to) {
                sl->rotFrom = sl->rot;
                sl->rotTo = to;
                sl->frame = animate ? 0 : CHANGING_SKIN_FRAMES;
                if (!animate) sl->rot = to;
            }
            continue;
        }
        for (int i = 0; i < NSLOT; i++)
            if (s_store[i].idx < 0) { loadInto(i, want[k], restRot(k)); break; }
    }
}

static void buildEntries() {
    s_entryCount = 0;
    if (s_pack == PACK_DEFAULT) {
        for (int i = 0; i < SKIN_DEFAULT_COUNT; i++) s_entries[s_entryCount++] = { -1, (short)i };
    } else if (s_pack == PACK_FAVORITES) {

        for (int f = 0; f < skinFavoriteCount(); f++) {
            char file[96];
            snprintf(file, sizeof(file), "%s", skinFavorite(f));
            char* slash = strchr(file, '/');
            if (!slash) continue;
            *slash = 0;
            int pk = openedPack(file);
            if (pk < 0) continue;
            for (int i = 0; i < s_opened[pk].skinCount; i++)
                if (strcmp(s_opened[pk].skins[i].file, slash + 1) == 0) {
                    s_entries[s_entryCount++] = { (short)pk, (short)i };
                    break;
                }
        }
    } else {
        int pk = openedPack(s_packFile[s_pack]);
        for (int i = 0; pk >= 0 && i < s_opened[pk].skinCount && s_entryCount < SKIN_MAX_SKINS; i++)
            s_entries[s_entryCount++] = { (short)pk, (short)i };
    }
}

static void openPack(int pack, int center) {
    for (int i = 0; i < NSLOT; i++) freeSlot(s_store[i]);
    closeOpened();
    s_pack = pack;
    buildEntries();
    s_center = wrap(center, skinCount());
    refreshSlots(false);
}

static void entryOption(int i, char* out, size_t cap) {
    out[0] = 0;
    if (i < 0 || i >= s_entryCount) return;
    const Entry& en = s_entries[i];
    if (en.pack < 0) snprintf(out, cap, "default:%d", en.idx);
    else snprintf(out, cap, "%s:%d", s_openedFile[en.pack], en.idx);
}

static void entryFavorite(int i, char* out, size_t cap) {
    out[0] = 0;
    if (i < 0 || i >= s_entryCount || s_entries[i].pack < 0) return;
    const Entry& en = s_entries[i];
    snprintf(out, cap, "%s/%s", s_openedFile[en.pack], s_opened[en.pack].skins[en.idx].file);
}

unsigned int skinPageSig() {
    if (!s_open) return 0;
    return (unsigned int)(s_pack * 131071 + s_center * 257 + s_focus + 1);
}

const char* skinPageTriangleLabel() {
    if (!s_open || s_focus != FOCUS_SKINS) return 0;
    char key[96];
    entryFavorite(s_center, key, sizeof(key));
    if (!key[0]) return 0;
    return skinFavoriteFind(key) >= 0 ? "Unfavourite" : "Favourite";
}

void skinPageOpen() {
    s_open = true;
    s_focus = FOCUS_SKINS;
    for (int i = 0; i < NSLOT; i++) { memset(&s_store[i], 0, sizeof(Slot)); s_store[i].idx = -1; }
    if (!s_mesh) s_mesh = (MobVertex (*)[36])malloc(sizeof(*s_mesh) * NSLOT * SKIN_MAX_BOXES);
    scanPacks();

    const char* worn = skinOptionGet();
    int pack = PACK_DEFAULT, center = 0;
    const char* colon = strrchr(worn, ':');
    if (strncmp(worn, "default:", 8) == 0) {
        center = atoi(worn + 8);
    } else if (colon) {
        size_t len = (size_t)(colon - worn);
        for (int i = PACK_FIRST_FILE; i < s_packCount; i++)
            if (strlen(s_packFile[i]) == len && strncmp(s_packFile[i], worn, len) == 0) {
                pack = i;
                center = atoi(colon + 1);
            }
    }
    openPack(pack, center);
}

static void closePage() {
    freeAll();
    free(s_mesh);
    s_mesh = 0;
    s_open = false;
}

void skinPageInput(MenuState& , unsigned int pressed) {
    if (pressed & PSP_CTRL_CIRCLE) { closePage(); return; }

    if (pressed & PSP_CTRL_UP)   s_focus = (s_focus == FOCUS_SKINS) ? FOCUS_PACKS : FOCUS_BAR;
    if (pressed & PSP_CTRL_DOWN) s_focus = (s_focus == FOCUS_BAR) ? FOCUS_PACKS : FOCUS_SKINS;
    if (s_focus == FOCUS_BAR) {

        if (pressed & PSP_CTRL_CROSS) closePage();
        return;
    }

    int packStep = 0, skinStep = 0;
    if (pressed & PSP_CTRL_LTRIGGER) packStep = -1;
    if (pressed & PSP_CTRL_RTRIGGER) packStep = 1;
    if (pressed & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT)) {
        int d = (pressed & PSP_CTRL_LEFT) ? -1 : 1;
        if (s_focus == FOCUS_PACKS) packStep = d; else skinStep = d;
    }
    if (packStep)
        openPack(wrap(s_pack + packStep, s_packCount), 0);
    if (skinStep && skinCount() > 1) {
        s_center = wrap(s_center + skinStep, skinCount());
        refreshSlots(true);
    }

    if (s_focus != FOCUS_SKINS || skinCount() == 0) return;
    char key[96];
    entryFavorite(s_center, key, sizeof(key));
    if (pressed & PSP_CTRL_CROSS) {
        char v[96];
        entryOption(s_center, v, sizeof(v));
        skinOptionSet(v);

    }
    if ((pressed & PSP_CTRL_TRIANGLE) && key[0]) {
        int at = skinFavoriteFind(key);
        if (at < 0) {
            skinFavoriteAdd(key);
        } else {
            skinFavoriteRemove(at);

            if (s_pack == PACK_FAVORITES) openPack(PACK_FAVORITES, s_center);
        }
    }
}

static void slice9(MenuState& s, float sx, float sy, float sw, float sh, int l, int t, int r, int b,
                   int x, int y, int w, int h) {
    const float us[4] = { 0.0f, (float)l, sw - r, sw };
    const float vs[4] = { 0.0f, (float)t, sh - b, sh };
    const int   dx[4] = { x, x + l, x + w - r, x + w };
    const int   dy[4] = { y, y + t, y + h - b, y + h };
    for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++) {
            const float su = us[i + 1] - us[i], sv = vs[j + 1] - vs[j];
            const int   dw = dx[i + 1] - dx[i], dh = dy[j + 1] - dy[j];
            if (su > 0.0f && sv > 0.0f && dw > 0 && dh > 0)
                spriteDraw(&s.guiAtlas, (float)dx[i], (float)dy[j], (float)dw, (float)dh,
                           sx + us[i], sy + vs[j], su, sv, 0xFFFFFFFFu);
        }
}

static void tabBar(MenuState& s, bool focus) {
    const int y0 = SK_BAR_Y, W = SK_STUB_W, H = SK_BAR_BAND_Y + 7 - SK_BAR_Y;
    if (focus) {
        slice9(s, GA_SKIN_BAR_L_F, 0, 2, 2, 7, 0, y0, W, H);
        slice9(s, GA_SKIN_BAR_R_F, 2, 2, 0, 7, 480 - W, y0, W, H);
        slice9(s, GA_SKIN_BAND_F, 0, 0, 0, 0, W, SK_BAR_BAND_Y, 480 - 2 * W, 7);
    } else {
        slice9(s, GA_SKIN_BAR_L, 0, 2, 2, 7, 0, y0, W, H);
        slice9(s, GA_SKIN_BAR_R, 2, 2, 0, 7, 480 - W, y0, W, H);
        slice9(s, GA_SKIN_BAND, 0, 0, 0, 0, W, SK_BAR_BAND_Y, 480 - 2 * W, 7);
    }
}

static void shadowClipped(MenuState& s, float x, float y, const char* t, unsigned int col, float maxW) {
    fontDrawTextClipped(&s.font, x, y, t, col, 1.0f, maxW);
}

static const char* packName(int i) {
    return s_packName[wrap(i, s_packCount)];
}

static void label(MenuState& s, float cx, float y, const char* t, unsigned int col, float maxW,
                  bool shadow) {
    float w = (float)fontTextWidth(&s.font, t);
    if (w > maxW) {
        fontDrawTextClipped(&s.font, cx - maxW * 0.5f, y, t, col, 1.0f, maxW);
    } else if (shadow) {
        fontDrawTextShadow(&s.font, cx - w * 0.5f, y, t, col, 1.0f);
    } else {
        fontDrawText(&s.font, cx - w * 0.5f, y, t, col, 1.0f);
    }
}

void skinPageRender(MenuState& s) {
    if (!s.haveFont) return;
    sceGuDisable(GU_DEPTH_TEST);

    s_ws += (0.1f - s_ws) * 0.4f;
    s_wp += s_ws;

    const bool onPacks = (s_focus == FOCUS_PACKS);
    drawRect(0.0f, SK_TINT_Y, 480.0f, SK_TINT_H, SK_TINT);

    if (!s.haveGui) { sceGuEnable(GU_DEPTH_TEST); return; }
    textureBind(&s.guiAtlas);

    tabBar(s, onPacks);

    static const int kOrder[3] = { 0, 2, 1 };
    for (int o = 0; o < 3; o++) {
        const int t = kOrder[o];
        const bool centre = (t == 1);
        const int y = centre ? SK_TAB_Y - SK_TAB_C_DY - (onPacks ? SK_TAB_C_GROW : 0) : SK_TAB_Y;
        const int h = (centre ? SK_TAB_BOT + SK_TAB_C_LAP : SK_TAB_BOT) - y;

        textureBind(&s.guiAtlas);
        if (centre && onPacks) slice9(s, GA_SKIN_TAB_C_F, 3, 3, 3, 1, kTabX[t], y, SK_TAB_W, h);
        else if (centre)       slice9(s, GA_SKIN_TAB_C,   3, 3, 3, 1, kTabX[t], y, SK_TAB_W, h);
        else if (onPacks)      slice9(s, GA_SKIN_TAB_F,   3, 3, 3, 0, kTabX[t], y, SK_TAB_W, h);
        else                   slice9(s, GA_SKIN_TAB,     3, 3, 3, 0, kTabX[t], y, SK_TAB_W, h);
        const char* name = packName(s_pack + t - 1);

        const float tw = (float)fontTextWidth(&s.font, name);
        const float tx = tw < SK_TAB_W - 8 ? kTabX[t] + (SK_TAB_W - tw) * 0.5f : kTabX[t] + 4.0f;
        const unsigned int tc = centre ? (onPacks ? 0xFFA0FFFFu : 0xFFE0E0E0u) : 0xFFB0B0B0u;
        shadowClipped(s, floorf(tx + 0.5f), floorf(y + (h - 8) * 0.5f + 0.5f), name, tc,
                      SK_TAB_W - 8.0f);
    }

    textureBind(&s.guiAtlas);
    if (onPacks) slice9(s, GA_SKIN_BAND_F, 0, 0, 0, 0, 0, SK_BASE_BAND_Y, 480, 7);
    else         slice9(s, GA_SKIN_BAND,   0, 0, 0, 0, 0, SK_BASE_BAND_Y, 480, 7);

    static const int drawOrder[NSLOT] = { 0, 6, 1, 5, 2, 4, 3 };
    for (int o = 0; o < NSLOT; o++) {
        int k = drawOrder[o];
        int idx = positionIdx(k);
        if (idx < 0) continue;
        Slot* sl = findSlot(idx);
        if (!sl) continue;
        if (sl->frame < CHANGING_SKIN_FRAMES) {
            sl->frame++;
            sl->rot = sl->rotFrom + sl->frame * ((sl->rotTo - sl->rotFrom) / CHANGING_SKIN_FRAMES);
        }
        SkinDraw d;
        memset(&d, 0, sizeof(d));
        const int si = (int)(sl - s_store);
        const Entry& en = s_entries[idx];
        d.tex = sl->haveTex ? &sl->tex : 0;
        if (en.pack >= 0) {
            d.cape = sl->haveCape ? &sl->cape : 0;
            d.boxMesh = s_mesh ? s_mesh + si * SKIN_MAX_BOXES : 0;
            d.boxPart = s_part[si];
            d.boxCount = s_mesh ? sl->boxCount : 0;
            d.anim = s_opened[en.pack].skins[en.idx].anim;
        }
        const SlotRect& r = kSlot[k];
        playerModelRenderSkinPreview(d, r.x, r.y + SK_TOP_SHIFT, r.w, r.h, sl->rot, s_wp, s_ws);
    }
    sceGuDisable(GU_DEPTH_TEST);

    char v[96];
    entryOption(s_center, v, sizeof(v));
    textureBind(&s.guiAtlas);
    if (skinCount() > 0 && strcmp(v, skinOptionGet()) == 0) {
        slice9(s, GA_SKIN_BADGE, 2, 0, 2, 0, SK_SEL_X, SK_SEL_Y + 1, SK_SEL_W, SK_SEL_H - 2);
        label(s, SK_SEL_X + SK_SEL_W * 0.5f, SK_SEL_Y + 3.0f, "Selected", 0xFFE0E0E0u,
              SK_SEL_W - 8.0f, true);
        textureBind(&s.guiAtlas);
    }

    slice9(s, GA_SKIN_RECESS, 2, 2, 2, 2, SK_NAME_X, SK_NAME_Y, SK_NAME_W, SK_NAME_H);
    const char* name = "";
    char origin[64] = "";
    if (skinCount() == 0) {
        name = s_pack == PACK_FAVORITES ? "No favourite skins" : "No skins";
    } else if (s_entries[s_center].pack < 0) {
        name = skinDefaultName(s_entries[s_center].idx);
    } else {
        const Entry& en = s_entries[s_center];
        const SkinEntry& e = s_opened[en.pack].skins[en.idx];
        name = e.name[0] ? e.name : e.file;
        if (e.theme[0]) snprintf(origin, sizeof(origin), "%s", e.theme);
        else snprintf(origin, sizeof(origin), "%s", s_opened[en.pack].name);
    }
    const float cx = SK_NAME_X + SK_NAME_W * 0.5f;
    label(s, cx, SK_NAME_Y + 4.0f,  name,   0xFFFFFFFFu, SK_NAME_W - 12.0f, true);
    label(s, cx, SK_NAME_Y + 17.0f, origin, 0xFFFFFFFFu, SK_NAME_W - 12.0f, true);

    {
        const float bw = menuBarButtonW(s, "Back");
        const float lb = 4.0f * MENU_PX + bw;
        drawMenuHeader(s, "Skins", 0.0f, VW, MENU_BAR_H, MENU_BAR_TEXT, lb, VW - lb * 2.0f);
        menuBarButton(s, 4.0f * MENU_PX, bw, "Back", s_focus == FOCUS_BAR);
    }
    sceGuEnable(GU_DEPTH_TEST);
}
