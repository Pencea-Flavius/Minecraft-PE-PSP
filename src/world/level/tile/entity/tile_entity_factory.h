
#ifndef MCPSP_WORLD_LEVEL_TILE_ENTITY_FACTORY_H
#define MCPSP_WORLD_LEVEL_TILE_ENTITY_FACTORY_H

class TileEntity;
class Level;
class CompoundTag;

namespace TileEntityFactory {
    TileEntity* createTileEntity(int type);
    TileEntity* loadTileEntity(CompoundTag* tag, Level* level);
}

#endif
