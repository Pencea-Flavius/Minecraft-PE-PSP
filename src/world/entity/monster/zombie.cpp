#include "world/entity/monster/zombie.h"
#include "world/entity/entity_types.h"
#include "world/item/item.h"
#include "world/level/pathfinder/path_navigation.h"
#include "world/entity/ai/goals/float_goal.h"
#include "world/entity/ai/goals/break_door_goal.h"
#include "world/entity/ai/goals/melee_attack_goal.h"
#include "world/entity/ai/goals/random_stroll_goal.h"
#include "world/entity/ai/goals/look_at_player_goal.h"
#include "world/entity/ai/goals/random_look_around_goal.h"
#include "world/entity/ai/goals/hurt_by_target_goal.h"
#include "world/entity/ai/goals/nearest_attackable_target_goal.h"

void Zombie::addZombieGoals(float speed) {
    goalSelector.addGoal(0, new FloatGoal(this));

    goalSelector.addGoal(2, new MeleeAttackGoal(this, speed, false));
    goalSelector.addGoal(6, new RandomStrollGoal(this, speed));
    goalSelector.addGoal(7, new LookAtPlayerGoal(this, 8.0f));
    goalSelector.addGoal(7, new RandomLookAroundGoal(this));
    goalSelector2.addGoal(1, new HurtByTargetGoal(this, 16.0f));
    goalSelector2.addGoal(2, new NearestAttackableTargetGoal(this, 16.0f));
}

Zombie::Zombie(Level* level) : Monster(level) {
    setSize(0.6f, 1.8f);
    entityRendererId = ER_ZOMBIE_RENDERER;
    runSpeed = 0.23f;
    attackDamage = 4;
    health = getMaxHealth();
    addZombieGoals(0.23f);

    getNavigation()->setCanOpenDoors(true);
    goalSelector.addGoal(1, new BreakDoorGoal(this));
}

Zombie::Zombie(Level* level, int rendererId) : Monster(level) {
    setSize(0.6f, 1.8f);
    entityRendererId = (EntityRendererId)rendererId;
    runSpeed = 0.23f;
    attackDamage = 4;
    health = getMaxHealth();

    addZombieGoals(0.21f);
}

void Zombie::aiStep() {
    updateSunburn();
    Monster::aiStep();
}

int Zombie::getEntityTypeId() const { return EntityTypes::IdZombie; }

void Zombie::dropDeathLoot() {
    if (sharedRandom.nextInt(4) == 0) spawnAtLocation(ITEM_FEATHER, 1);

    if (sharedRandom.nextInt(40) == 0) spawnAtLocation(ITEM_CARROT, 1);
    if (sharedRandom.nextInt(40) == 0) spawnAtLocation(ITEM_POTATO, 1);
}
