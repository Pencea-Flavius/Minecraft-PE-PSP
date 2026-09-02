
#ifndef MCPSP_CLIENT_RENDERER_TILEENTITY_TILE_ENTITY_RENDERER_H
#define MCPSP_CLIENT_RENDERER_TILEENTITY_TILE_ENTITY_RENDERER_H

class Level;

void renderAllTileEntities(Level* level, float a);

unsigned int tileEntityLightColor(Level* level, int x, int y, int z);

class ChestTileEntity;

void renderChestTile(ChestTileEntity* chest, float a);

const struct Texture* chestModelTexture();

struct ChunkVertex;
int chestBuildHeldMesh(ChunkVertex* out);

struct Texture;

Texture* signBoardTexture();

#endif
