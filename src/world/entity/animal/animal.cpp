#include "world/entity/animal/animal.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/entity/entity_types.h"
#include "nbt/compound_tag.h"
#include "world/entity/local_player.h"
#include "world/inventory/inventory.h"
#include "world/item/item_instance.h"
#include "client/renderer/particle.h"
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

    if (age != 0) inLove = 0;
    if (inLove > 0) {
        if ((--inLove & 0xF) == 0) {
            float px = x + (sharedRandom.nextFloat() * bbWidth * 2.0f) - bbWidth;
            float py = (y - heightOffset) + 0.5f + sharedRandom.nextFloat() * bbHeight;
            float pz = z + (sharedRandom.nextFloat() * bbWidth * 2.0f) - bbWidth;
            particlesHeart(px, py, pz,
                           sharedRandom.nextGaussian() * 0.02f,
                           sharedRandom.nextGaussian() * 0.02f,
                           sharedRandom.nextGaussian() * 0.02f);
        }
    }
}

bool Animal::canMate(Animal* other) {
    if (other == this) return false;
    if (other->getEntityTypeId() != getEntityTypeId()) return false;
    return isInLove() && other->isInLove();
}

bool Animal::isFood(ItemInstance* ) { return false; }

bool Animal::playerInteract() {
    Player* p = (Player*)g_level.player;
    if (!p) return false;
    ItemInstance* sel = p->inventory->getSelected();
    if (!sel || !isFood(sel) || isBaby() || getAge() != 0) return false;

    p->inventory->consumeSelected();
    inLove = 600;
    attackTargetId = 0;
    setDespawnProtected();
    particlesHeartBurst(x, y - heightOffset, z, bbWidth, bbHeight);
    return true;
}

void Animal::baseTick() {

    if      (age < 0) age++;
    else if (age > 0) age--;
    Mob::baseTick();
}

void Animal::addAdditonalSaveData(CompoundTag* tag) {
    Mob::addAdditonalSaveData(tag);
    tag->putInt("Age", age);
    tag->putInt("InLove", inLove);

    tag->putByte("DespawnProtected", despawnProtected ? 1 : 0);
    tag->putInt("WanderMinX", minWanderX); tag->putInt("WanderMaxX", maxWanderX);
    tag->putInt("WanderMinZ", minWanderZ); tag->putInt("WanderMaxZ", maxWanderZ);
}

void Animal::readAdditionalSaveData(CompoundTag* tag) {
    Mob::readAdditionalSaveData(tag);
    age = tag->getInt("Age");
    inLove = tag->getInt("InLove");
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
    inLove = 0;
    return Mob::hurt(source, damage);
}

float Animal::getWalkTargetValue(int x, int y, int z) {
    if (level->getTile(x, y - 1, z) == BLOCK_GRASS) return 10.0f;
    return level->getBrightness(x, y, z) - 0.5f;
}

int Animal::getCreatureBaseType() const { return EntityTypes::BaseCreature; }
