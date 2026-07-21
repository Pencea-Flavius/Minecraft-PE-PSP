#include "world/entity/monster/skeleton.h"
#include "world/entity/entity_types.h"
#include "world/entity/arrow.h"
#include "world/level/level.h"
#include "world/item/item.h"
#include <cmath>

static const float SK_RADDEG = 180.0f / 3.14159265f;

Skeleton::Skeleton(Level* level) : Monster(level) {
    setSize(0.6f, 1.8f);
    entityRendererId = ER_SKELETON_RENDERER;
    attackDamage = 2;
    health = getMaxHealth();
}

void Skeleton::aiStep() {
    updateSunburn();
    Mob::aiStep();
}

int Skeleton::getEntityTypeId() const { return EntityTypes::IdSkeleton; }

void Skeleton::checkHurtTarget(Entity* target, float d) {
    if (d >= 10.0f) return;

    float myEyeY = (y - heightOffset) + bbHeight * 0.85f;
    float ex = target->x;
    float ey = (target->y - target->heightOffset) + target->bbHeight * 0.5f;
    float ez = target->z;
    float dx = ex - x, dz = ez - z;
    float horiz = sqrtf(dx * dx + dz * dz);
    float dy = (ey - myEyeY) + horiz * 0.2f;
    float yaw   = atan2f(dx, dz) * SK_RADDEG;
    float pitch = atan2f(dy, horiz) * SK_RADDEG;

    xRot = pitch;

    if (attackTime == 0) {
        Arrow* a = new Arrow(level, x, myEyeY, z, yaw, pitch, 1.1f, false);
        a->ownerId = entityId;
        level->addEntity(a);
        attackTime = TicksPerSecond * 3;
    }
    holdGround = true;
}

void Skeleton::dropDeathLoot() {
    int arrows = sharedRandom.nextInt(3);
    for (int i = 0; i < arrows; i++) spawnAtLocation(ITEM_ARROW, 1);
    int bones = sharedRandom.nextInt(3);
    for (int i = 0; i < bones; i++) spawnAtLocation(ITEM_BONE, 1);
}
