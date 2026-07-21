#include "world/level/tile/entity/tile_entity_factory.h"
#include "world/level/tile/entity/tile_entity.h"
#include "world/level/tile/entity/sign_tile_entity.h"
#include "world/level/tile/entity/chest_tile_entity.h"
#include "world/level/tile/entity/furnace_tile_entity.h"
#include "world/level/tile/entity/reactor_tile_entity.h"
#include "nbt/compound_tag.h"

namespace TileEntityFactory {

TileEntity* createTileEntity(int type) {
    switch (type) {
        case TE_SIGN:    return new SignTileEntity();
        case TE_CHEST:   return new ChestTileEntity();
        case TE_FURNACE: return new FurnaceTileEntity();
        case TE_REACTOR: return new ReactorTileEntity();
    }
    return 0;
}

TileEntity* loadTileEntity(CompoundTag* tag, Level* level) {
    if (!tag || !tag->contains("id")) return 0;
    TileEntity* te = createTileEntity(tag->getInt("id"));
    if (te) {
        te->level = level;
        te->load(tag);
    }
    return te;
}

}
