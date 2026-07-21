
#ifndef MCPSP_WORLD_LEVEL_TILE_ENTITY_CHEST_TILE_ENTITY_H
#define MCPSP_WORLD_LEVEL_TILE_ENTITY_CHEST_TILE_ENTITY_H

#include "world/level/tile/entity/tile_entity.h"
#include "world/inventory/filling_container.h"

class CompoundTag;

class ChestTileEntity : public TileEntity {
    typedef TileEntity super;
public:
    static const int CONTAINER_SIZE = 27;

    ChestTileEntity();

    virtual bool save(CompoundTag* tag);
    virtual void load(CompoundTag* tag);

    FillingContainer container;
};

#endif
