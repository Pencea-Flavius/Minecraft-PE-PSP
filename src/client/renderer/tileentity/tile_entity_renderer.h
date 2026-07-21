
#ifndef MCPSP_CLIENT_RENDERER_TILEENTITY_TILE_ENTITY_RENDERER_H
#define MCPSP_CLIENT_RENDERER_TILEENTITY_TILE_ENTITY_RENDERER_H

class Level;

void renderAllTileEntities(Level* level, float a);

struct Texture;

Texture* signBoardTexture();

#endif
