
#ifndef MCPSP_CLIENT_PLAYER_H
#define MCPSP_CLIENT_PLAYER_H

#include "client/gui/screens/menu.h"
#include <pspctrl.h>

void gameUpdate(MenuState& s, unsigned int pressed, const SceCtrlData& pad);

void playerSpawnAt(float eyeY);

void playerSpawnEnsure();

#define HOTBAR_SLOTS 8
#define INV_COLS     6
#include "world/item/item.h"
#include "world/inventory/inventory.h"

extern bool g_invOpen;
extern int  g_invCursor;
extern int  g_invHeaderSel;
extern float g_flashSlotStartTime;
extern int   g_flashSlotIndex;
extern int g_invFlashCursor;
extern int g_invFlashTicks;

extern bool g_paused;
extern int  g_pauseSel;
extern int  g_thirdPerson;
extern bool g_optionsOpen;
extern bool g_quitConfirm;
extern int  g_quitConfirmSel;

extern bool g_saveRequested;
extern bool g_isoMapRequested;

extern bool g_quitAfterSave;

void playerRespawn();

void releaseWorldAndPlayer();
void quitToMenuNoSave(MenuState& s);

#endif
