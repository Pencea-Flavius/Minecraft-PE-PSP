#ifndef MCPSP_WORLD_LEVEL_TILE_TILE_GUI_HOOKS_H
#define MCPSP_WORLD_LEVEL_TILE_TILE_GUI_HOOKS_H

class SignTileEntity;
class FurnaceTileEntity;
class ChestTileEntity;

void guiOpenSignEditor(SignTileEntity* te);
void guiOpenFurnace(FurnaceTileEntity* te);
void guiOpenChest(ChestTileEntity* te);
void guiOpenCrafting(bool stonecutter);
void guiChatMessage(const char* msg);

#endif
