
#ifndef MCPSP_CLIENT_RENDER_H
#define MCPSP_CLIENT_RENDER_H

#include "client/gui/screens/menu.h"

#define SKY_COLOR 0xFFFFD8C0u

extern unsigned int g_clearColorNow;
extern unsigned int g_skyColorNow;

extern float g_camX, g_camY, g_camZ;

extern float g_camYawNow;

extern float g_nearZPlane;

void gameRender(MenuState& s);

bool gameProgressScreenUp();

#endif
