#include "world/entity/monster/pig_zombie.h"
#include "world/entity/entity_types.h"
#include "world/entity/arrow.h"
#include "world/level/level.h"
#include "world/item/item.h"
#include "nbt/compound_tag.h"
#include <vector>

PigZombie::PigZombie(Level* level)
:   Zombie(level, ER_PIGZOMBIE_RENDERER),
    angerTime(0),
    playAngrySoundIn(0),
    stunedTime(TicksPerSecond * 3)
{
    runSpeed = 0.7f;
    attackDamage = 5;
    fireImmune = true;
    health = getMaxHealth();
}

int PigZombie::getEntityTypeId() const { return EntityTypes::IdPigZombie; }

void PigZombie::tick() {
    if (stunedTime > 0) stunedTime--;
    if (playAngrySoundIn > 0) {
        if (--playAngrySoundIn == 0)

            level->playSound(this, "mob.zombiepig.zpigangry",
                             getSoundVolume() * 2.0f, getVoicePitch() * 1.8f);
    }
    Zombie::tick();
}

Entity* PigZombie::findAttackTarget() {
    if (stunedTime != 0) return 0;
    Entity* t = Monster::findAttackTarget();
    if (angerTime == 0) {

        if (t && t->distanceTo(x, y, z) < 5.0f) return t;
        return 0;
    }
    return t;
}

void PigZombie::alert(Entity* target) {
    if (!target) return;
    attackTargetId = target->entityId;

    angerTime = 400 + sharedRandom.nextInt(400);
    playAngrySoundIn = sharedRandom.nextInt(40);
}

bool PigZombie::hurt(Entity* source, int damage) {

    Entity* attacker = 0;
    if (source) {
        if (source->isPlayer()) {
            attacker = source;
        } else if (source->isEntityType(EntityTypes::IdArrow)) {
            Arrow* ar = (Arrow*)source;
            if (ar->ownerId != 0) {
                Entity* o = level->getEntity(ar->ownerId);
                if (o && o->isPlayer()) attacker = o;
            }
        }
    }

    bool applied = Zombie::hurt(source, damage);

    if (applied && attacker) {

        AABB box = bb.grow(12.0f, 12.0f, 12.0f);
        std::vector<Entity*> nearby = level->getEntities(this, box);
        for (size_t i = 0; i < nearby.size(); i++) {
            if (nearby[i]->isEntityType(EntityTypes::IdPigZombie))
                ((PigZombie*)nearby[i])->alert(attacker);
        }
        alert(attacker);
    }
    return applied;
}

void PigZombie::dropDeathLoot() {
    int count = sharedRandom.nextInt(2);
    for (int i = 0; i < count; i++) spawnAtLocation(ITEM_GOLD_INGOT, 1);
}

void PigZombie::addAdditonalSaveData(CompoundTag* tag) {
    Mob::addAdditonalSaveData(tag);
    tag->putShort("Anger", (short)angerTime);
}

void PigZombie::readAdditionalSaveData(CompoundTag* tag) {
    Mob::readAdditionalSaveData(tag);
    angerTime = tag->getShort("Anger");
}
