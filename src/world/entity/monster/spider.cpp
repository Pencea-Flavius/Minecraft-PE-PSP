#include "world/entity/monster/spider.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/item/item.h"
#include <cmath>

Spider::Spider(Level* level) : Monster(level), climbing(false) {
    setSize(1.4f, 0.9f);
    entityRendererId = ER_SPIDER_RENDERER;
    runSpeed = 0.5f;
    attackDamage = 2;
    health = getMaxHealth();
}

void Spider::tick() {
    Monster::tick();
    climbing = horizontalCollision;
}

bool Spider::onLadder()      { return climbing; }
void Spider::makeStuckInWeb() {  }

int Spider::getEntityTypeId() const { return EntityTypes::IdSpider; }
int Spider::getDeathLoot()          { return ITEM_STRING; }

Entity* Spider::findAttackTarget() {

    if (getBrightness(1.0f) < 0.5f) return Monster::findAttackTarget();
    return 0;
}

void Spider::checkHurtTarget(Entity* target, float d) {

    if (getBrightness(1.0f) > 0.5f && sharedRandom.nextInt(100) == 0) {
        attackTargetId = 0;
        return;
    }

    if (d > 2.0f && d < 6.0f && sharedRandom.nextInt(10) == 0 && onGround) {
        float xdd = target->x - x, zdd = target->z - z;
        float dd = sqrtf(xdd * xdd + zdd * zdd);
        if (dd > 1e-4f) {
            xd = (xdd / dd * 0.5f) * 0.8f + xd * 0.2f;
            zd = (zdd / dd * 0.5f) * 0.8f + zd * 0.2f;
            yd = 0.4f;
        }
    } else {
        Monster::checkHurtTarget(target, d);
    }
}
