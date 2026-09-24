#include "world/entity/animal/chicken.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/item/item.h"
#include "world/item/item_instance.h"
#include "world/entity/ai/goals/float_goal.h"
#include "world/entity/ai/goals/panic_goal.h"
#include "world/entity/ai/goals/breed_goal.h"
#include "world/entity/ai/goals/tempt_goal.h"
#include "world/entity/ai/goals/follow_parent_goal.h"
#include "world/entity/ai/goals/random_stroll_goal.h"
#include "world/entity/ai/goals/look_at_player_goal.h"
#include "world/entity/ai/goals/random_look_around_goal.h"

static const int CHICKEN_FOODS[] = { ITEM_SEEDS_WHEAT };

Chicken::Chicken(Level* level) : Animal(level) {
    setSize(0.4f, 0.7f);
    heightOffset = 0.0f;
    runSpeed = 0.25f;
    entityRendererId = ER_CHICKEN_RENDERER;
    health = getMaxHealth();
    eggTime = sharedRandom.nextInt(6000) + 6000;

    flap = oFlap = flapSpeed = oFlapSpeed = 0.0f;
    flapping = 1.0f;

    goalSelector.addGoal(0, new FloatGoal(this));

    goalSelector.addGoal(1, new PanicGoal(this, 0.38f));
    goalSelector.addGoal(2, new BreedGoal(this, 0.25f));
    goalSelector.addGoal(3, new TemptGoal(this, 0.25f, CHICKEN_FOODS, 1));
    goalSelector.addGoal(4, new FollowParentGoal(this, 0.28f));
    goalSelector.addGoal(5, new RandomStrollGoal(this, 0.25f));
    goalSelector.addGoal(6, new LookAtPlayerGoal(this, 6.0f));
    goalSelector.addGoal(7, new RandomLookAroundGoal(this));
}

bool Chicken::isFood(ItemInstance* item) {
    if (!item || item->id <= 0 || item->id >= 4096) return false;
    Item* it = Item::items[item->id];
    return it && it->isSeed();
}

Animal* Chicken::getBreedOffspring(Animal*) { return new Chicken(level); }

int Chicken::getEntityTypeId() const { return EntityTypes::IdChicken; }

void Chicken::aiStep() {
    Animal::aiStep();

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
