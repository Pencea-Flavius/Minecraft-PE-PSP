
#ifndef MCPSP_MENU_MENU_H
#define MCPSP_MENU_MENU_H

#include "gpu/texture.h"
#include "gpu/font.h"
#include "world/level/storage/worldlist.h"
#include "world/level/storage/external_servers.h"

enum AppScreen { SCREEN_TITLE, SCREEN_WORLDS, SCREEN_DELETE, SCREEN_CREATE, SCREEN_JOIN, SCREEN_ADD_SERVER, SCREEN_OPTIONS, SCREEN_GAME };

static const float UI_SCALE = 2.0f;

inline float G(float v) { return v * UI_SCALE; }
static const float VW = 480.0f / UI_SCALE;
static const float VH = 272.0f / UI_SCALE;
static const unsigned int WHITE = 0xFFFFFFFFu;
static const unsigned int DIRT_TINT = 0xFF404040u;

struct MenuState {
    AppScreen screen;

    Font    font;         bool haveFont;
    Texture guiAtlas;     bool haveGui;
    Texture icons;        bool haveIcons;
    Texture logo;         bool haveLogo;
    Texture dirtBg;       bool haveBg;
    Texture touchGui;     bool haveTouch;
    Texture defaultWorld; bool haveDefaultWorld;

    WorldList worlds;
    int  worldSelected;
    int  deleteSelected;
    int  createSelected;
    int  newWorldGamemode;
    int  newWorldType;
    int  newWorldGenMask;
    char newWorldName[64];
    char newWorldSeed[64];
    int  uiRow;
    int  topSelected;
    float listScrollX;
    int  selected;

    ExternalServerList servers;
    int  joinUiRow;
    int  joinBarSel;
    int  joinRow;
    float joinScroll;
    int  joinEditMode;

    int  addSelected;
    char addName[32];
    char addAddr[64];
    char addPort[8];

    int  optFocus;
    int  optCategory;
    int  optTabHighlight;
    int  optItemHighlight;
    float optScroll;
    float createScroll;

    char statusMsg[128];
};

void drawRect(float x, float y, float w, float h, unsigned int color);

void drawDirtBackground(MenuState& s, float y = 0.0f, float h = -1.0f,
                        float uOffset = 0.0f, unsigned int tint = DIRT_TINT);

void drawWindowFrame(MenuState& s);

void drawHeaderBar(MenuState& s, bool shadow = false);

static const float MENU_PX       = 0.75f;
static const float MENU_BAR_H    = 26.0f * MENU_PX;
static const float MENU_BAR_BTNH = 18.0f * MENU_PX;
static const float MENU_BAR_BTNY = (MENU_BAR_H - MENU_BAR_BTNH) / 2.0f;

static const float MENU_BAR_Y    = -1.0f / UI_SCALE;

static const float MENU_BAR_TEXT = 2.0f;

static const float MENU_BEVEL    = 2.0f;

float menuBarButtonW(MenuState& s, const char* label);
void  menuBarButton(MenuState& s, float x, float w, const char* label, bool hovered);

void drawMenuHeader(MenuState& s, const char* title, float x, float w,
                    float h, float textScale, float titleX = 0.0f, float titleW = 0.0f);

void drawTextField(MenuState& s, float x, float y, float w, float h,
                   const char* text, const char* placeholder, bool focused,
                   float scale = UI_SCALE);

void uiDraw(MenuState& s, float x, float y, float w, float h,
            float tsx, float tsy, float asx, float asy,
            float sw, float sh, unsigned int tint);

void drawHeaderTitle(MenuState& s, const char* title, unsigned int color = 0xFFFFFFFFu,
                     float centreX = 0.0f, float width = VW);

static const float HEADER_H = 23.0f;

struct HoldCharge {
    int          key;
    unsigned int start;
    float        share;
    HoldCharge() : key(-1), start(0), share(0.0f) {}
    void reset() { key = -1; share = 0.0f; }
};

int holdChargeUpdate(HoldCharge& c, int slotKey, int count, bool crossHeld);

void drawNinePatch(MenuState& s, float sx, float sy, float sw, float sh, float corner,
                   float gx, float gy, float gw, float gh, float destCorner = -1.0f,
                   unsigned int tint = 0xFFFFFFFFu);
void fontDrawTextClipped(const Font* font, float x, float y, const char* text,
                         unsigned int color, float scale, float maxWidthRaw);
void fontDrawTextWrapped(const Font* font, float x, float y, const char* text,
                         unsigned int color, float scale, float maxWidthRaw);

enum OskTarget {
    OSK_TARGET_WORLD_NAME = 1,
    OSK_TARGET_WORLD_SEED = 2,
    OSK_TARGET_SIGN       = 4,
    OSK_TARGET_SRV_NAME   = 5,
    OSK_TARGET_SRV_ADDR   = 6,
    OSK_TARGET_SRV_PORT   = 7,
};
void startOsk(int target, const char* desc, const char* intext, int maxLen = 64);

bool menuOskUpdate(MenuState& s);

#include "gpu/button_icons.h"
#include "gpu/gui_atlas.h"
struct ButtonHint { ButtonIcon icon; unsigned int btn; const char* label; };

#define UI_HINT_S   1.0f

#define UI_HINT_GAP 2.0f
#define UI_HINTS_Y  (272.0f - 15.0f * UI_HINT_S - UI_HINT_GAP)

ButtonIcon menuShoulderIcon(bool right);

void drawNameTag(MenuState& s, float centreX, float y, const char* name,
                 float scale = UI_SCALE);

const char* pausePlayerName();
ButtonHint menuFaceHint(bool confirm, const char* label);
void buttonHintsDraw(MenuState& s, const ButtonHint* hints, int n, float y = UI_HINTS_Y,
                     float scale = UI_HINT_S);
void menuHintsDraw(MenuState& s);

bool optionsScreenUp(const MenuState& s);

extern unsigned int g_heldButtons;

void optionsInitDefaults();
void optionsLoad();
void optionsSave();

void optionsSetControlScheme(int scheme);

void optionsSetInvertLook(int on);
void optionsSetSouthpaw(int on);
void optionsSetFineAim(int on);

void optionsCycleCameraMode();

static const unsigned int GUI_DISABLED = 0xFF707070u;
void guiTButton(MenuState& s, float x, float y, float w, float h, bool pressed,
                float destCorner = 2.0f);
void guiTButtonLabel(MenuState& s, float x, float y, float w, float h,
                     const char* label, bool hovered, bool active, float scale = UI_SCALE);

static const float TOGGLE_CELL_W = 28.0f;
static const float TOGGLE_CELL_H = 15.0f;

static const float KNOB_CELL_W = 9.0f;
static const float KNOB_CELL_H = 14.0f;

void guiOptionSwitch(MenuState& s, float x, float y, float w, float h,
                     bool on, bool hovered, unsigned int tint = 0xFFFFFFFFu,
                     float scale = 1.0f);

unsigned int menuSelectionSig(const MenuState& s);

unsigned int optionsValueSig();

extern bool g_craftOpen;

enum { CRAFT_WORKBENCH = 0, CRAFT_STONECUTTER = 1 };
void craftOpen(int craftingSize, int filterMode);

extern bool g_uiParentIsInventory;
bool craftHasCategories();
bool armorFocusIsWornSlot();
bool chestCursorOnChest();
bool furnaceFocusIsSlots();
int  furnaceTargetSlotForCursor();

void gameHintsDraw(MenuState& s);

class FurnaceTileEntity;
class ChestTileEntity;
extern bool g_furnaceOpen;
void furnaceOpen(FurnaceTileEntity* fe);
extern bool g_chestOpen;
void chestOpen(ChestTileEntity* ce);

void chestClose();

extern bool g_armorOpen;
void armorOpen();

void inBedRenderFade(MenuState& s);

extern bool g_deadScreen;
void deadScreenOpen();

class SignTileEntity;
extern SignTileEntity* g_signEditing;

SignTileEntity* signEditLive();
void signEditRemember(int x, int y, int z);
void signStopEdit();
void signOskForget();
void signStartEdit(SignTileEntity* ste);
void signEditLine(int line);

#endif
