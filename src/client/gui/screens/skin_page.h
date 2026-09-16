
#ifndef MCPSP_CLIENT_GUI_SCREENS_SKIN_PAGE_H
#define MCPSP_CLIENT_GUI_SCREENS_SKIN_PAGE_H

#include "client/gui/screens/menu.h"

bool skinPageIsOpen();
void skinPageOpen();
void skinPageRender(MenuState& s);
void skinPageInput(MenuState& s, unsigned int pressed);

unsigned int skinPageSig();

const char* skinPageTriangleLabel();

#endif
