#include "world/entity/animal/pig.h"
#include "world/entity/entity_types.h"
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
#include "world/level/pathfinder/path_navigation.h"

static const int PIG_FOODS[] = { ITEM_POTATO, ITEM_CARROT, ITEM_BEETROOT };

Pig::Pig(Level* level) : Animal(level) {
    setSize(0.9f, 0.9f);
    heightOffset = 0.0f;
    runSpeed = 0.25f;

    getNavigation()->setAvoidWater(true);
    entityRendererId = ER_PIG_RENDERER;
    health = getMaxHealth();

    goalSelector.addGoal(0, new FloatGoal(this));

    goalSelector.addGoal(1, new PanicGoal(this, 0.38f));
    goalSelector.addGoal(2, new BreedGoal(this, 0.25f));
    goalSelector.addGoal(3, new TemptGoal(this, 0.25f, PIG_FOODS, 3));
    goalSelector.addGoal(4, new FollowParentGoal(this, 0.28f));
    goalSelector.addGoal(5, new RandomStrollGoal(this, 0.25f));
    goalSelector.addGoal(6, new LookAtPlayerGoal(this, 6.0f));
    goalSelector.addGoal(7, new RandomLookAroundGoal(this));
}

bool Pig::isFood(ItemInstance* item) {
    if (!item) return false;
    return item->id == ITEM_POTATO || item->id == ITEM_CARROT ||
           item->id == ITEM_BEETROOT;
}

Animal* Pig::getBreedOffspring(Animal*) { return new Pig(level); }

int Pig::getEntityTypeId() const { return EntityTypes::IdPig; }

int Pig::getDeathLoot()          { return onFire > 0 ? ITEM_PORKCHOP_COOKED : ITEM_PORKCHOP_RAW; }
