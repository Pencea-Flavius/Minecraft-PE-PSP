#include "world/entity/monster/creeper.h"
#include "world/entity/entity_types.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/item/item.h"
#include "world/item/item_instance.h"
#include "world/entity/player.h"
#include "world/entity/local_player.h"
#include "world/inventory/inventory.h"
#include "client/renderer/particle.h"
#include "world/entity/ai/goals/float_goal.h"
#include "world/entity/ai/goals/swell_goal.h"
#include "world/entity/ai/goals/melee_attack_goal.h"
#include "world/entity/ai/goals/random_stroll_goal.h"
#include "world/entity/ai/goals/look_at_player_goal.h"
#include "world/entity/ai/goals/random_look_around_goal.h"
#include "world/entity/ai/goals/hurt_by_target_goal.h"
#include "world/entity/ai/goals/nearest_attackable_target_goal.h"

Creeper::Creeper(Level* level)
:   Monster(level), swell(0), oldSwell(0), swellDir(-1) {
    setSize(0.6f, 1.8f);
    entityRendererId = ER_CREEPER_RENDERER;

    runSpeed = 0.25f;
    health = getMaxHealth();

    goalSelector.addGoal(1, new FloatGoal(this));
    goalSelector.addGoal(2, new SwellGoal(this));

    goalSelector.addGoal(4, new MeleeAttackGoal(this, 0.25f, false));
    goalSelector.addGoal(5, new RandomStrollGoal(this, 0.20f));
    goalSelector.addGoal(6, new LookAtPlayerGoal(this, 8.0f));
    goalSelector.addGoal(6, new RandomLookAroundGoal(this));
    goalSelector2.addGoal(2, new NearestAttackableTargetGoal(this, 16.0f));
    goalSelector2.addGoal(2, new HurtByTargetGoal(this, 16.0f));
}

int Creeper::getEntityTypeId() const { return EntityTypes::IdCreeper; }
int Creeper::getDeathLoot()          { return ITEM_GUNPOWDER; }

void Creeper::tick() {
    oldSwell = swell;
    Monster::tick();
    if (removed) return;

    if (!useNewAi() && attackTargetId == 0 && swellDir == 1) swellDir = -1;

    if (swellDir > 0) {

        if (swell == 0) level->playSound(this, "random.fuse", 1.0f, 0.5f);
        if (++swell >= MAX_SWELL) {

            swell = MAX_SWELL;
            if (!isAlive()) return;

            remove();

            worldExplode(level->w, x, y, z, 2.4f, isInWater());
        }
        return;
    }
    if (swell > 0 && --swell < 0) swell = 0;
}

float Creeper::getSwelling(float a) const {
    return (oldSwell + (swell - oldSwell) * a) / (float)(MAX_SWELL - 2);
}

void Creeper::checkHurtTarget(Entity* target, float d) {

    if (swellDir == 2) return;

    if ((swellDir <= 0 && d < 3.0f) || (swellDir > 0 && d < 7.0f)) {
        swellDir = 1;
        holdGround = true;
    } else {
        swellDir = -1;
    }
}

void Creeper::checkCantSeeTarget(Entity* target, float d) {

    if (swellDir != 2) swellDir = -1;
    Monster::checkCantSeeTarget(target, d);
}

bool Creeper::playerInteract() {
    ItemInstance* sel = g_level.player->inventory->getSelected();
    if (!sel || sel->id != ITEM_FLINT_AND_STEEL) return Monster::playerInteract();
    if (!level->isClientSide && swellDir != 2) {

        level->playSound(g_level.player->x + 0.5f, g_level.player->y + 0.5f,
                         g_level.player->z + 0.5f, "fire.ignite",
                         1.0f, sharedRandom.nextFloat() * 0.4f + 0.8f);
        swellDir = 2;
    }

    g_level.player->inventory->hurtSelected(1);
    return true;
}
