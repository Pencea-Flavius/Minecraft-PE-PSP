#include "world/entity/animal/chicken.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/item/item.h"

Chicken::Chicken(Level* level) : Animal(level) {
    setSize(0.4f, 0.7f);
    heightOffset = 0.0f;
    walkingSpeed = 0.1f;
    entityRendererId = ER_CHICKEN_RENDERER;
    health = getMaxHealth();
    eggTime = sharedRandom.nextInt(6000) + 6000;

    flap = oFlap = flapSpeed = oFlapSpeed = 0.0f;
    flapping = 1.0f;
}

int Chicken::getEntityTypeId() const { return EntityTypes::IdChicken; }

void Chicken::aiStep() {
    PathfinderMob::aiStep();

    oFlap      = flap;
    oFlapSpeed = flapSpeed;

    flapSpeed += (onGround ? -1.0f : 4.0f) * 0.3f;
    if (flapSpeed < 0.0f) flapSpeed = 0.0f;
    if (flapSpeed > 1.0f) flapSpeed = 1.0f;

    if (!onGround && flapping < 1.0f) flapping = 1.0f;
    flapping *= 0.9f;

    if (!onGround && yd < 0.0f) yd *= 0.6f;

    flap += flapping * 2.0f;

    if (!level->isClientSide && !isBaby() && --eggTime <= 0) {
        spawnAtLocation(ITEM_EGG, 1);
        eggTime = sharedRandom.nextInt(6000) + 6000;
    }
}

void Chicken::dropDeathLoot() {
    spawnAtLocation(onFire > 0 ? ITEM_CHICKEN_COOKED : ITEM_CHICKEN_RAW, 1);
    int feathers = sharedRandom.nextInt(3);
    for (int i = 0; i < feathers; i++) spawnAtLocation(ITEM_FEATHER, 1);
}
