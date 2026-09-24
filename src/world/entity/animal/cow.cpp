#include "world/entity/animal/cow.h"
#include "world/entity/entity_types.h"
#include "world/item/item.h"
#include "world/item/bucket_item.h"
#include "world/item/item_instance.h"
#include "world/entity/player.h"
#include "world/inventory/inventory.h"
#include "world/level/level.h"
#include "world/entity/ai/goals/float_goal.h"
#include "world/entity/ai/goals/panic_goal.h"
#include "world/entity/ai/goals/breed_goal.h"
#include "world/entity/ai/goals/tempt_goal.h"
#include "world/entity/ai/goals/follow_parent_goal.h"
#include "world/entity/ai/goals/random_stroll_goal.h"
#include "world/entity/ai/goals/look_at_player_goal.h"
#include "world/entity/ai/goals/random_look_around_goal.h"
#include "world/level/pathfinder/path_navigation.h"

static const int COW_FOODS[] = { ITEM_WHEAT };

Cow::Cow(Level* level) : Animal(level) {
    setSize(0.9f, 1.3f);
    heightOffset = 0.0f;
    runSpeed = 0.2f;

    getNavigation()->setAvoidWater(true);
    entityRendererId = ER_COW_RENDERER;
    health = getMaxHealth();
    milkedTicks = 0;

    goalSelector.addGoal(0, new FloatGoal(this));

    goalSelector.addGoal(1, new PanicGoal(this, 0.38f));
    goalSelector.addGoal(2, new BreedGoal(this, 0.2f));
    goalSelector.addGoal(3, new TemptGoal(this, 0.25f, COW_FOODS, 1));
    goalSelector.addGoal(4, new FollowParentGoal(this, 0.25f));
    goalSelector.addGoal(5, new RandomStrollGoal(this, 0.2f));
    goalSelector.addGoal(6, new LookAtPlayerGoal(this, 6.0f));
    goalSelector.addGoal(7, new RandomLookAroundGoal(this));
}

int Cow::getEntityTypeId() const { return EntityTypes::IdCow; }

bool Cow::isFood(ItemInstance* item) { return item && item->id == ITEM_WHEAT; }

Animal* Cow::getBreedOffspring(Animal*) { return new Cow(level); }

void Cow::dropDeathLoot() {
    int beef = 1 + sharedRandom.nextInt(3);
    short meat = onFire > 0 ? ITEM_BEEF_COOKED : ITEM_BEEF_RAW;
    for (int i = 0; i < beef; i++) spawnAtLocation(meat, 1);
    int leather = sharedRandom.nextInt(3);
    for (int i = 0; i < leather; i++) spawnAtLocation(ITEM_LEATHER, 1);
}

void Cow::aiStep() {
    Animal::aiStep();
    ++milkedTicks;
}

bool Cow::playerInteract() {
    Player* p = (Player*)g_level.player;
    if (!p) return false;
    ItemInstance* sel = p->inventory->getSelected();
    if (milkedTicks <= 20 || !sel || sel->id != ITEM_BUCKET || sel->data != BUCKET_EMPTY ||
        p->inventory->isCreative())
        return Animal::playerInteract();
    milkedTicks = 0;
    if (--sel->count <= 0) {
        sel->id = ITEM_BUCKET; sel->count = 1; sel->data = BUCKET_MILK;
    } else {
        ItemInstance milk(ITEM_BUCKET, 1, BUCKET_MILK);
        if (p->inventory->add(milk)) p->inventory->ensureHotbar(ITEM_BUCKET, BUCKET_MILK);
    }
    return true;
}
