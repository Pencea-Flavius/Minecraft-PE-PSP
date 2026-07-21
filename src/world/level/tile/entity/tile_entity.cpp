#include "world/level/tile/entity/tile_entity.h"
#include "world/level/level.h"
#include "nbt/compound_tag.h"

TileEntity::TileEntity(int type)
    : type(type), x(0), y(0), z(0), level(0), rendererId(TR_NO_RENDER), removed(false) {}

void TileEntity::setLevelAndPos(Level* lvl, int px, int py, int pz) {
    level = lvl;
    x = px; y = py; z = pz;
}

bool TileEntity::save(CompoundTag* tag) {
    tag->putInt("x", x);
    tag->putInt("y", y);
    tag->putInt("z", z);
    return true;
}

void TileEntity::load(CompoundTag* tag) {
    x = tag->getInt("x");
    y = tag->getInt("y");
    z = tag->getInt("z");
}

int TileEntity::getTile() const { return level ? level->getTile(x, y, z) : 0; }
int TileEntity::getData() const { return level ? level->getData(x, y, z) : 0; }
