#include "world/entity/animal/sheep.h"
#include "world/entity/entity_types.h"
#include "world/entity/item_entity.h"
#include "world/item/item.h"
#include "world/item/item_instance.h"
#include "world/inventory/inventory.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"
#include "nbt/compound_tag.h"
#include "client/gamemode/gamemode.h"
#include <math.h>

extern Inventory g_inv;
extern World g_world;

static const float MTH_PI = 3.14159265f;

Sheep::Sheep(Level* level) : Animal(level), woolColor(0), sheared(false) {
    setSize(0.9f, 1.3f);
    heightOffset = 0.0f;
    walkingSpeed = 0.1f;
    entityRendererId = ER_SHEEP_RENDERER;
    health = getMaxHealth();
    woolColor = getSheepColor(sharedRandom);
}

int Sheep::getEntityTypeId() const { return EntityTypes::IdSheep; }

int Sheep::getSheepColor(Random& random) {
    int r = random.nextInt(100);
    if (r < 5)  return 15;
    if (r < 10) return 7;
    if (r < 15) return 8;
    if (r < 18) return 12;
    if (random.nextInt(500) == 0) return 6;
    return 0;
}

static void dropWool(Level* level, float x, float y, float z, int color) {
    ItemEntity* ie = new ItemEntity(level, x, y, z, ItemInstance(BLOCK_WOOL, 1, (short)color));
    level->addEntity(ie);
}

void Sheep::dropDeathLoot() {
    if (!sheared) dropWool(level, x, y, z, woolColor);
}

bool Sheep::playerInteract() {
    ItemInstance* sel = g_inv.getSelected();
    if (!sel || sel->isNull()) return false;

    if (sel->id == ITEM_BONEMEAL && !isBaby()) {
        int newColor = 15 - sel->data;
        if (!level->isClientSide && getColor() != newColor) {
            setColor(newColor);

            if (g_gameMode && !g_gameMode->isCreative()) {
                sel->count--;
                if (sel->count <= 0) sel->setNull();
            }
        }
        return true;
    }

    if (sel->id != ITEM_SHEARS) return false;
    if (sheared || isBaby()) return false;
    if (!level->isClientSide) {
        sheared = true;
        int count = 1 + sharedRandom.nextInt(3);
        for (int i = 0; i < count; i++) dropWool(level, x, y, z, woolColor);
    }

    if (g_gameMode && !g_gameMode->isCreative()) sel->hurt(1);
    return true;

}

void Sheep::addAdditonalSaveData(CompoundTag* tag) {
    Animal::addAdditonalSaveData(tag);
    tag->putBoolean("Sheared", sheared);
    tag->putByte("Color", (char)woolColor);
}

void Sheep::readAdditionalSaveData(CompoundTag* tag) {
    Animal::readAdditionalSaveData(tag);
    sheared = tag->getBoolean("Sheared");
    woolColor = tag->getByte("Color") & 0x0f;
}

void Sheep::aiStep() {
    PathfinderMob::aiStep();
    if (eatAnimationTick > 0) eatAnimationTick--;

    int bx = (int)floorf(x), by = (int)floorf(y), bz = (int)floorf(z);

    if (eatAnimationTick <= 0) {

        if (((isBaby() && sharedRandom.nextInt(50) == 0) || sharedRandom.nextInt(1000) == 0) &&
            worldBlock(&g_world, bx, by - 1, bz) == BLOCK_GRASS) {
            eatAnimationTick = EAT_ANIMATION_TICKS;
        }
    } else if (eatAnimationTick == 4) {

        if (worldBlock(&g_world, bx, by - 1, bz) == BLOCK_GRASS) {
            worldSetBlockAndData(&g_world, bx, by - 1, bz, BLOCK_DIRT, 0);
            worldNotifyNeighborsChanged(&g_world, bx, by - 1, bz);
            worldRebuildAroundNow(&g_world, bx, by - 1, bz);
            setSheared(false);
            if (isBaby()) {
                int na = getAge() + TicksPerSecond * 60;
                if (na > 0) na = 0;
                setAge(na);
            }
        }
    }
}

float Sheep::getHeadEatPositionScale(float a) const {
    if (eatAnimationTick <= 0) return 0.0f;
    if (eatAnimationTick >= 4 && eatAnimationTick <= EAT_ANIMATION_TICKS - 4) return 1.0f;
    if (eatAnimationTick < 4) return ((float)eatAnimationTick - a) / 4.0f;
    return -((float)(eatAnimationTick - EAT_ANIMATION_TICKS) - a) / 4.0f;
}

float Sheep::getHeadEatAngleScale(float a) const {
    if (eatAnimationTick > 4 && eatAnimationTick <= EAT_ANIMATION_TICKS - 4) {
        float s = ((float)(eatAnimationTick - 4) - a) / (float)(EAT_ANIMATION_TICKS - 8);
        return MTH_PI * 0.20f + MTH_PI * 0.07f * sinf(s * 28.7f);
    }
    if (eatAnimationTick > 0) return MTH_PI * 0.20f;
    return 0.0f;
}
