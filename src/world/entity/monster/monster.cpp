
#include "world/entity/monster/monster.h"
#include "world/entity/local_player.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/difficulty.h"
#include "world/inventory/inventory.h"
#include "client/renderer/particle.h"
#include <cmath>

Monster::Monster(Level* level)
:   PathfinderMob(level),
    attackDamage(2),
    sunburnCounter(0),
    frozenTicks(0)
{
    entityRendererId = ER_HUMANOID_RENDERER;
    heightOffset = 0.0f;
    walkingSpeed = 0.1f;
}

void Monster::updateSunburn() {
    if (fireImmune) return;

    if (((++sunburnCounter) & 1) == 0) return;
    if (level->isClientSide) return;
    float br = getBrightness(1.0f);
    if (br <= 0.5f) return;
    int xt = (int)floorf(x);
    int yt = (int)floorf(y - heightOffset);
    int zt = (int)floorf(z);
    if (!worldCanSeeSky(level->w, xt, yt, zt)) return;
    if (sharedRandom.nextFloat() * 3.5f >= (br - 0.4f)) return;

    int burn = 8 * TicksPerSecond;
    if (onFire <= 0) onFire = burn;
    for (int i = 0; i < 2; i++) {
        float ox = x + sharedRandom.nextFloat() * bbWidth * 2.0f - bbWidth;
        float oy = (y - heightOffset) + sharedRandom.nextFloat() * bbHeight;
        float oz = z + sharedRandom.nextFloat() * bbWidth * 2.0f - bbWidth;
        particlesSmoke(ox, oy, oz);
    }
}

static const int DESPAWN_FROZEN = 5 * 60 * 20;

void Monster::tick() {
    PathfinderMob::tick();

    if (!level->isClientSide && level->getDifficulty() == Difficulty::PEACEFUL) { remove(); return; }

    if (level->player) {
        float dx = x - level->player->x, dy = y - level->player->y, dz = z - level->player->z;
        if (dx * dx + dy * dy + dz * dz > mobAiRange() * mobAiRange()) {
            if (++frozenTicks >= DESPAWN_FROZEN) remove();
        } else {
            frozenTicks = 0;
        }
    }
}

bool Monster::hurt(Entity* source, int damage) {
    if (!PathfinderMob::hurt(source, damage)) return false;

    if (source && source != this) attackTargetId = source->entityId;
    return true;
}

int Monster::getCreatureBaseType() const { return EntityTypes::BaseEnemy; }

Entity* Monster::findAttackTarget() {
    LocalPlayer* p = level->player;
    if (!p || !p->isAlive()) return 0;

    if (g_inv.isCreative()) return 0;
    if (distanceTo((Entity*)p) > 16.0f) return 0;
    if (!canSee((Entity*)p)) return 0;
    return (Entity*)p;
}

bool Monster::canSpawn() {
    return isDarkEnoughToSpawn();
}

bool Monster::isDarkEnoughToSpawn() {
    int xt = (int)floorf(x);
    int yt = (int)floorf(bb.y0);
    int zt = (int)floorf(z);

    int sky = lightSkyGet(level->w, xt, yt, zt) - g_skyDarken;
    if (sky < 0) sky = 0;
    if (sky > sharedRandom.nextInt(32)) return false;

    return lightRawAt(level->w, xt, yt, zt) <= sharedRandom.nextInt(8);
}

bool Monster::doHurtTarget(Entity* target) {
    swing();
    return target->hurt(this, attackDamage);
}

void Monster::checkHurtTarget(Entity* target, float distance) {

    if (attackTime <= 0 && distance < 2.0f &&
        target->bb.y1 > bb.y0 && target->bb.y0 < bb.y1) {
        attackTime = getAttackTime();
        doHurtTarget(target);
    }
}

float Monster::getWalkTargetValue(int x, int y, int z) {
    return 0.5f - level->getBrightness(x, y, z);
}
