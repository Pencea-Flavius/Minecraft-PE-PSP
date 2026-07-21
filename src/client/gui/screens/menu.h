
#ifndef MCPSP_MENU_MENU_H
#define MCPSP_MENU_MENU_H

#include "gpu/texture.h"
#include "gpu/font.h"
#include "world/level/storage/worldlist.h"

enum AppScreen { SCREEN_TITLE, SCREEN_WORLDS, SCREEN_DELETE, SCREEN_CREATE, SCREEN_JOIN, SCREEN_JOIN_IP, SCREEN_OPTIONS, SCREEN_GAME };

static const float UI_SCALE = 2.0f;
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
    Texture spriteSheet;  bool haveSprite;
    Texture defaultWorld; bool haveDefaultWorld;

    WorldList worlds;
    int  worldSelected;
    int  deleteSelected;
    int  createSelected;
    int  newWorldGamemode;
    char newWorldName[64];
    char newWorldSeed[64];
    int  uiRow;
    int  topSelected;
    float listScrollX;
    int  selected;

    int  joinListSelected;
    int  joinIpSelected;
    char joinIp[64];

    int  optFocus;
    int  optCategory;
    int  optTabHighlight;
    int  optItemHighlight;
    int  optEditingRow;

    char statusMsg[128];
};

void drawRect(float x, float y, float w, float h, unsigned int color);

void drawNinePatch(MenuState& s, float sx, float sy, float sw, float sh, float corner,
                   float gx, float gy, float gw, float gh);
void fontDrawTextClipped(const Font* font, float x, float y, const char* text,
                         unsigned int color, float scale, float maxWidthRaw);
void fontDrawTextWrapped(const Font* font, float x, float y, const char* text,
                         unsigned int color, float scale, float maxWidthRaw);
void startOsk(int target, const char* desc, const char* intext, int maxLen = 64);

bool menuOskUpdate(MenuState& s);

#include "gpu/button_icons.h"
struct ButtonHint { ButtonIcon icon; unsigned int btn; const char* label; };

#define UI_HINT_S   1.0f
#define UI_HINTS_Y  (272.0f - 15.0f * UI_HINT_S)
void buttonHintsDraw(MenuState& s, const ButtonHint* hints, int n, float y = UI_HINTS_Y,
                     float scale = UI_HINT_S);
void menuHintsDraw(MenuState& s);

extern unsigned int g_heldButtons;

void titleHandleInput(MenuState& s, unsigned int pressed);   void titleRender(MenuState& s);
void worldsHandleInput(MenuState& s, unsigned int pressed);  void worldsRender(MenuState& s);
void deleteHandleInput(MenuState& s, unsigned int pressed);  void deleteRender(MenuState& s);
void createHandleInput(MenuState& s, unsigned int pressed);  void createRender(MenuState& s);
void joinHandleInput(MenuState& s, unsigned int pressed);    void joinRender(MenuState& s);
void joinIpHandleInput(MenuState& s, unsigned int pressed);  void joinIpRender(MenuState& s);
void optionsHandleInput(MenuState& s, unsigned int pressed); void optionsRender(MenuState& s);
void optionsInitDefaults();
void optionsLoad();
void optionsSave();
void optionsSetThirdPerson(bool on);

unsigned int menuSelectionSig(const MenuState& s);

unsigned int optionsValueSig();

void pauseHandleInput(MenuState& s, unsigned int pressed);   void pauseRender(MenuState& s);

extern bool g_craftOpen;

enum { CRAFT_WORKBENCH = 0, CRAFT_STONECUTTER = 1 };
void craftOpen(int craftingSize, int filterMode);
void craftHandleInput(MenuState& s, unsigned int pressed);   void craftRender(MenuState& s);
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
void furnaceHandleInput(MenuState& s, unsigned int pressed, unsigned int held); void furnaceRender(MenuState& s);
extern bool g_chestOpen;
void chestOpen(ChestTileEntity* ce);
void chestHandleInput(MenuState& s, unsigned int pressed, unsigned int held);   void chestRender(MenuState& s);

extern bool g_armorOpen;
void armorOpen();
void armorHandleInput(MenuState& s, unsigned int pressed);   void armorRender(MenuState& s);

void inBedHandleInput(MenuState& s, unsigned int pressed);  void inBedRender(MenuState& s);
void inBedRenderFade(MenuState& s);

extern bool g_deadScreen;
void deadScreenOpen();
void deadHandleInput(MenuState& s, unsigned int pressed);    void deadRender(MenuState& s);

class SignTileEntity;
extern SignTileEntity* g_signEditing;
void signHandleInput(MenuState& s, unsigned int pressed);    void signRender(MenuState& s);
void signStartEdit(SignTileEntity* ste);
void signEditLine(int line);

#endif
