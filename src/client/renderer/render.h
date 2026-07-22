
#ifndef MCPSP_CLIENT_RENDER_H
#define MCPSP_CLIENT_RENDER_H

#include "client/gui/screens/menu.h"

#define SKY_COLOR 0xFFE0A860u

extern unsigned int g_skyColorNow;

extern float g_camX, g_camY, g_camZ;

void gameRender(MenuState& s);

bool gameProgressScreenUp();

#endif
