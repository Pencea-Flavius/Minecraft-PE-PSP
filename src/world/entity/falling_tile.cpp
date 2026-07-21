
#include "world/entity/falling_tile.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "nbt/compound_tag.h"
#include "util/mth.h"

FallingTile::FallingTile(Level* level)
    : super(level), tile(0), data(0), time(0) {
    init();
}

FallingTile::FallingTile(Level* level, float px, float py, float pz, int tile, int data)
    : super(level), tile(tile), data(data), time(0) {
    init();
    setPos(px, py, pz);
    xo = xOld = px;
    yo = yOld = py;
    zo = zOld = pz;
}

void FallingTile::init() {
    entityRendererId = ER_FALLINGTILE_RENDERER;
    time = 0;
    blocksBuilding = true;
    setSize(0.98f, 0.98f);
    heightOffset = bbHeight / 2.0f;
    makeStepSound = false;
    xd = yd = zd = 0.0f;
}

void FallingTile::tick() {
    if (tile == 0) { remove(); return; }

    xOld = x; yOld = y; zOld = z;
    xo = x; yo = y; zo = z;
    time++;

    yd -= 0.04f;
    move(xd, yd, zd);
    xd *= 0.98f;
    yd *= 0.98f;
    zd *= 0.98f;

    int xt = Mth::floor(x);
    int yt = Mth::floor(y);
    int zt = Mth::floor(z);

    if (time == 1) {

        if (worldBlock(level->w, xt, yt, zt) == tile) {
            worldSetBlockAndData(level->w, xt, yt, zt, BLOCK_AIR, 0);
            worldNotifyNeighborsChanged(level->w, xt, yt, zt);
        } else {
            remove();
            return;
        }
    }

    if (onGround) {
        xd *= 0.7f;
        zd *= 0.7f;
        yd *= -0.5f;

        remove();

        if (isReplaceable(worldBlock(level->w, xt, yt, zt))) {
            worldSetBlockAndData(level->w, xt, yt, zt, (unsigned char)tile, (unsigned char)data);
            worldNotifyNeighborsChanged(level->w, xt, yt, zt);
        } else {
            worldSpawnResources(level->w, xt, yt, zt, (unsigned char)tile, data);
        }
    } else if (time > TicksPerSecond * 5) {
        worldSpawnResources(level->w, xt, yt, zt, (unsigned char)tile, data);
        remove();
    }
}

int FallingTile::getEntityTypeId() const { return EntityTypes::IdFallingTile; }

void FallingTile::addAdditonalSaveData(CompoundTag* tag) {
    tag->putByte("Tile", (char)tile);
    tag->putByte("Data", (char)data);
    tag->putByte("Time", (char)time);
}

void FallingTile::readAdditionalSaveData(CompoundTag* tag) {
    tile = (unsigned char)tag->getByte("Tile");
    data = (unsigned char)tag->getByte("Data");
    time = (unsigned char)tag->getByte("Time");
}
