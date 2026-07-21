#include "world/entity/animal/animal.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/entity/entity_types.h"
#include "nbt/compound_tag.h"
#include <cmath>

Animal::Animal(Level* level) : PathfinderMob(level) { setDespawnProtected(); }

void Animal::setDespawnProtected() {
    int xt = (int)floorf(x), zt = (int)floorf(z);
    minWanderX = maxWanderX = xt;
    minWanderZ = maxWanderZ = zt;
    despawnProtected = true;
}

void Animal::updateDespawnProtectedState() {
    if (!despawnProtected) return;
    int xt = (int)floorf(x), zt = (int)floorf(z);
    if (xt > maxWanderX) maxWanderX = xt;
    if (xt < minWanderX) minWanderX = xt;
    if (zt > maxWanderZ) maxWanderZ = zt;
    if (zt < minWanderZ) minWanderZ = zt;
    if ((maxWanderX - minWanderX) > MAX_WANDER_DISTANCE ||
        (maxWanderZ - minWanderZ) > MAX_WANDER_DISTANCE)
        despawnProtected = false;
}

void Animal::aiStep() {
    updateDespawnProtectedState();
    PathfinderMob::aiStep();
}

void Animal::baseTick() {
    if (age < 0) age++;
    Mob::baseTick();
}

void Animal::addAdditonalSaveData(CompoundTag* tag) {
    Mob::addAdditonalSaveData(tag);
    tag->putInt("Age", age);

    tag->putByte("DespawnProtected", despawnProtected ? 1 : 0);
    tag->putInt("WanderMinX", minWanderX); tag->putInt("WanderMaxX", maxWanderX);
    tag->putInt("WanderMinZ", minWanderZ); tag->putInt("WanderMaxZ", maxWanderZ);
}

void Animal::readAdditionalSaveData(CompoundTag* tag) {
    Mob::readAdditionalSaveData(tag);
    age = tag->getInt("Age");
    if (tag->contains("WanderMinX")) {
        despawnProtected = tag->getByte("DespawnProtected") != 0;
        minWanderX = tag->getInt("WanderMinX"); maxWanderX = tag->getInt("WanderMaxX");
        minWanderZ = tag->getInt("WanderMinZ"); maxWanderZ = tag->getInt("WanderMaxZ");
    } else {
        setDespawnProtected();
    }
}

bool Animal::hurt(Entity* source, int damage) {
    fleeTime = 3 * TicksPerSecond;
    attackTargetId = 0;
    return Mob::hurt(source, damage);
}

float Animal::getWalkTargetValue(int x, int y, int z) {
    if (level->getTile(x, y - 1, z) == BLOCK_GRASS) return 10.0f;
    return level->getBrightness(x, y, z) - 0.5f;
}

int Animal::getCreatureBaseType() const { return EntityTypes::BaseCreature; }
