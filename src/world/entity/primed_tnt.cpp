
#include "world/entity/primed_tnt.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "client/renderer/particle.h"
#include "nbt/compound_tag.h"
#include <cmath>
#include <cstdlib>

PrimedTnt::PrimedTnt(Level* level)
    : super(level), life(80) {
    init();
}

PrimedTnt::PrimedTnt(Level* level, float x, float y, float z, int fuse)
    : super(level), life(fuse) {
    init();
    setPos(x, y, z);
    xo = xOld = x; yo = yOld = y; zo = zOld = z;

    float rot = ((float)rand() / RAND_MAX) * 6.2831853f;
    xd = -sinf(rot) * 0.02f;
    yd = 0.2f;
    zd = -cosf(rot) * 0.02f;
}

void PrimedTnt::init() {
    entityRendererId = ER_TNT_RENDERER;
    blocksBuilding = true;
    setSize(0.98f, 0.98f);
    heightOffset = bbHeight / 2.0f;
    makeStepSound = false;
    xd = yd = zd = 0.0f;
}

void PrimedTnt::tick() {

    xOld = x; yOld = y; zOld = z;
    xo = x; yo = y; zo = z;

    yd -= 0.04f;
    move(xd, yd, zd);
    xd *= 0.98f;
    yd *= 0.98f;
    zd *= 0.98f;
    if (onGround) {
        xd *= 0.7f;
        zd *= 0.7f;
        yd *= -0.5f;
    }

    if (--life <= 0) {
        remove();
        worldExplode(level->w, x, y, z, 3.1f);
    } else {
        particlesSmoke(x, y + 0.5f, z);
    }
}

int PrimedTnt::getEntityTypeId() const { return EntityTypes::IdPrimedTnt; }

void PrimedTnt::addAdditonalSaveData(CompoundTag* tag) { tag->putByte("Fuse", (char)life); }
void PrimedTnt::readAdditionalSaveData(CompoundTag* tag) { life = (unsigned char)tag->getByte("Fuse"); }
