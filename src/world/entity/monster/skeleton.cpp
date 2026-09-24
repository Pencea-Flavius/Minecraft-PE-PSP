#include "world/entity/monster/skeleton.h"
#include "world/entity/entity_types.h"
#include "world/entity/arrow.h"
#include "world/level/level.h"
#include "world/item/item.h"
#include "world/entity/ai/goals/float_goal.h"
#include "world/entity/ai/goals/restrict_sun_goal.h"
#include "world/entity/ai/goals/flee_sun_goal.h"
#include "world/entity/ai/goals/arrow_attack_goal.h"
#include "world/entity/ai/goals/random_stroll_goal.h"
#include "world/entity/ai/goals/look_at_player_goal.h"
#include "world/entity/ai/goals/random_look_around_goal.h"
#include "world/entity/ai/goals/hurt_by_target_goal.h"
#include "world/entity/ai/goals/nearest_attackable_target_goal.h"
#include <cmath>

static const float SK_RADDEG = 180.0f / 3.14159265f;

Skeleton::Skeleton(Level* level) : Monster(level) {
    setSize(0.6f, 1.8f);
    entityRendererId = ER_SKELETON_RENDERER;
    runSpeed = 0.25f;
    attackDamage = 2;
    health = getMaxHealth();

    goalSelector.addGoal(1, new FloatGoal(this));
    goalSelector.addGoal(2, new RestrictSunGoal(this));

    goalSelector.addGoal(3, new FleeSunGoal(this, 0.25f));
    goalSelector.addGoal(4, new ArrowAttackGoal(this, 0.25f, ArrowAttackGoal::ATTACK_ARROW, 60));
    goalSelector.addGoal(5, new RandomStrollGoal(this, 0.25f));
    goalSelector.addGoal(6, new LookAtPlayerGoal(this, 8.0f));
    goalSelector.addGoal(6, new RandomLookAroundGoal(this));
    goalSelector2.addGoal(1, new HurtByTargetGoal(this, 16.0f));
    goalSelector2.addGoal(2, new NearestAttackableTargetGoal(this, 16.0f));
}

void Skeleton::aiStep() {
    updateSunburn();
    Monster::aiStep();
}

int Skeleton::getEntityTypeId() const { return EntityTypes::IdSkeleton; }

void Skeleton::checkHurtTarget(Entity* target, float d) {
    if (d >= 10.0f) return;

    if (attackTime == 0) performRangedAttack(target, 32.0f);
    else                 aimAt(target);
    holdGround = true;
}

void Skeleton::aimAt(Entity* target) {

    float myEyeY = y + getHeadHeight();
    float ex = target->x;
    float ey = target->y + target->getHeadHeight() - 0.7f;
    float ez = target->z;
    float dx = ex - x, dz = ez - z;
    float horiz = sqrtf(dx * dx + dz * dz);

    float dy = (ey - (myEyeY - 0.1f)) + horiz * 0.2f;
    float yaw   = atan2f(dz, dx) * SK_RADDEG - 90.0f;
    float pitch = atan2f(dy, horiz) * SK_RADDEG;

    yRot = yaw;
    xRot = pitch;

}

void Skeleton::performRangedAttack(Entity* target, float uncertainty) {
    aimAt(target);
    float myEyeY = y + getHeadHeight();

    Arrow* a = new Arrow(level, x, myEyeY, z, yRot, xRot, 1.6f / 1.5f, false,
                          false, uncertainty);
    a->ownerId = entityId;
    level->addEntity(a);
    level->playSound(this, "random.bow", 1.0f, 1.0f);
    attackTime = TicksPerSecond * 3;
}

void Skeleton::dropDeathLoot() {
    int arrows = sharedRandom.nextInt(3);
    for (int i = 0; i < arrows; i++) spawnAtLocation(ITEM_ARROW, 1);
    int bones = sharedRandom.nextInt(3);
    for (int i = 0; i < bones; i++) spawnAtLocation(ITEM_BONE, 1);
}
