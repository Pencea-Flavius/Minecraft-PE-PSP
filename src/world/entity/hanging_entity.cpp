
#include "world/entity/hanging_entity.h"
#include "world/direction.h"
#include "world/level/level.h"
#include "nbt/compound_tag.h"
#include "util/mth.h"

HangingEntity::HangingEntity(Level* level) : super(level) { init(); }

HangingEntity::HangingEntity(Level* level, int xTile, int yTile, int zTile, int )
    : super(level), dir(0), xTile(xTile), yTile(yTile), zTile(zTile) {
    init();
}

void HangingEntity::setPosition(int x, int y, int z) { xTile = x; yTile = y; zTile = z; }

void HangingEntity::init() {
    heightOffset = 0;
    setSize(0.5f, 0.5f);
    dir = 0;
    checkInterval = 0;
}

void HangingEntity::setDir(int d) {
    this->dir = d;
    yRotO = yRot = float(d * 90);

    float w = float(getWidth());
    float h = float(getHeight());
    float depth = float(getWidth());

    if (d == Direction::NORTH || d == Direction::SOUTH) {
        depth = 2.0f;
        yRot = yRotO = float(Direction::DIRECTION_OPPOSITE[d] * 90);
    } else {
        w = 2.0f;
    }

    w /= 32.0f;
    h /= 32.0f;
    depth /= 32.0f;

    float px = xTile + 0.5f;
    float py = yTile + 0.5f;
    float pz = zTile + 0.5f;

    float offset = 0.5f + 1.0f / 16.0f;
    if (d == Direction::NORTH) pz -= offset;
    if (d == Direction::WEST)  px -= offset;
    if (d == Direction::SOUTH) pz += offset;
    if (d == Direction::EAST)  px += offset;

    if (d == Direction::NORTH) px -= offs(getWidth());
    if (d == Direction::WEST)  pz += offs(getWidth());
    if (d == Direction::SOUTH) px += offs(getWidth());
    if (d == Direction::EAST)  pz -= offs(getWidth());
    py += offs(getHeight());

    setPos(px, py, pz);

    xOld = xo = x;
    yOld = yo = y;
    zOld = zo = z;

    float ss = -(0.5f / 16.0f);
    bb.set(px - w - ss, py - h - ss, pz - depth - ss,
           px + w + ss, py + h + ss, pz + depth + ss);
}

float HangingEntity::offs(int w) {
    if (w == 32) return 0.5f;
    if (w == 64) return 0.5f;
    return 0;
}

void HangingEntity::tick() {
    if (checkInterval++ == 20 * 5 && !level->isClientSide) {
        checkInterval = 0;
        if (!removed && !survives()) {
            remove();
            dropItem();
        }
    }
}

bool HangingEntity::survives() {
    if (!level->getCubes(this, bb).empty()) {
        return false;
    } else {
        int ws = Mth::Max(1, getWidth() / 16);
        int hs = Mth::Max(1, getHeight() / 16);

        int xt = xTile, yt = yTile, zt = zTile;
        if (dir == Direction::NORTH) xt = Mth::floor(x - getWidth() / 32.0f);
        if (dir == Direction::WEST)  zt = Mth::floor(z - getWidth() / 32.0f);
        if (dir == Direction::SOUTH) xt = Mth::floor(x - getWidth() / 32.0f);
        if (dir == Direction::EAST)  zt = Mth::floor(z - getWidth() / 32.0f);
        yt = Mth::floor(y - getHeight() / 32.0f);

        for (int ss = 0; ss < ws; ++ss) {
            for (int yy = 0; yy < hs; ++yy) {
                bool solid;
                if (dir == Direction::NORTH || dir == Direction::SOUTH) {
                    solid = level->isSolidTile(xt + ss, yt + yy, zTile);
                } else {
                    solid = level->isSolidTile(xTile, yt + yy, zt + ss);
                }
                if (!solid) return false;
            }
            std::vector<Entity*> es = level->getEntities(this, bb);
            for (size_t ei = 0; ei < es.size(); ++ei) {
                if (es[ei]->isHangingEntity()) return false;
            }
        }
    }
    return true;
}

bool HangingEntity::isHangingEntity() { return true; }
bool HangingEntity::isPickable() { return true; }

bool HangingEntity::interact() {

    if (!removed && !level->isClientSide) {
        remove();
        markHurt();
        dropItem();
        return true;
    }
    return !removed;
}

void HangingEntity::move(float xa, float ya, float za) {
    if (!level->isClientSide && !removed && (xa * xa + ya * ya + za * za) > 0) {
        dropItem();
        remove();
    }
}

void HangingEntity::push(float xa, float ya, float za) {
    if (!level->isClientSide && !removed && (xa * xa + ya * ya + za * za) > 0) {
        dropItem();
        remove();
    }
}

void HangingEntity::addAdditonalSaveData(CompoundTag* tag) {
    tag->putByte("Direction", (char)dir);
    tag->putInt("TileX", xTile);
    tag->putInt("TileY", yTile);
    tag->putInt("TileZ", zTile);
}

void HangingEntity::readAdditionalSaveData(CompoundTag* tag) {
    if (tag->contains("Direction")) {
        dir = tag->getByte("Direction");
    } else {
        switch (tag->getByte("Dir")) {
            case 0: dir = Direction::NORTH; break;
            case 1: dir = Direction::WEST;  break;
            case 2: dir = Direction::SOUTH; break;
            case 3: dir = Direction::EAST;  break;
        }
    }
    xTile = tag->getInt("TileX");
    yTile = tag->getInt("TileY");
    zTile = tag->getInt("TileZ");
    setDir(dir);
}

int HangingEntity::getRawBrightness() {
    int xt = Mth::floor(x);
    int zt = Mth::floor(z);
    if (level->hasChunksAt(xt, 0, zt, xt, 0, zt))
        return level->getRawBrightness(xt, Mth::floor(y), zt);
    return 0;
}

float HangingEntity::getBrightness(float ) {
    int xt = Mth::floor(x);
    int zt = Mth::floor(z);
    if (level->hasChunksAt(xt, 0, zt, xt, 0, zt)) {
        int yt = Mth::floor(y);
        return level->getBrightness(xt, yt, zt);
    }
    return 0;
}

bool HangingEntity::hurt(Entity* , int ) {
    if (!removed && !level->isClientSide) {
        remove();
        markHurt();
        dropItem();
    }
    return true;
}
