
#ifndef MCPSP_CLIENT_PLAYER_H
#define MCPSP_CLIENT_PLAYER_H

#include "client/gui/screens/menu.h"
#include <pspctrl.h>

void gameUpdate(MenuState& s, unsigned int pressed, const SceCtrlData& pad);

void playerSpawnAt(float eyeY);

void playerSpawnEnsure();

#define HOTBAR_SLOTS 7
#define INV_COLS     6
#include "world/item/item.h"
#include "world/inventory/inventory.h"

extern bool g_invOpen;
extern int  g_invCursor;
extern float g_flashSlotStartTime;
extern int g_invFlashCursor;
extern int g_invFlashTicks;

extern bool g_paused;
extern int  g_pauseSel;
extern bool g_muted;
extern bool g_serverVisible;
extern bool g_thirdPerson;
extern bool g_quitConfirm;
extern int  g_quitConfirmSel;

extern bool g_saveRequested;

extern bool g_quitAfterSave;

void playerRespawn();

void quitToMenuNoSave(MenuState& s);

#endif
