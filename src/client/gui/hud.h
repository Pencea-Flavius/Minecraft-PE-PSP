#pragma once
#include "client/gui/screens/menu.h"

void hotbarDraw(MenuState& s);
void guiFill(float x, float y, float w, float h, unsigned int color);
void guiFillGradient(float x, float y, float w, float h, unsigned int topColor, unsigned int botColor);
void drawBlockIcon(short id, unsigned char data, float x, float y, float sizePx, unsigned int colorTint = 0xFFFFFFFFu);
void drawStackCount(Font& font, int count, float slotX, float slotY, float size);
int getGuiBlockIcon(short id, unsigned char data);
const char* getBlockName(short id, unsigned char data);
const char* getBlockDescription(short id, unsigned char data);

int  itemFlatIcon(short id, unsigned char data);
void drawFlatIcon(int icon, float x, float y, float sizePx, unsigned int tint);
void drawArmorSlotGhost(int slot, float x, float y, float sizePx, unsigned int tint);

void hudChatMessage(const char* msg);
