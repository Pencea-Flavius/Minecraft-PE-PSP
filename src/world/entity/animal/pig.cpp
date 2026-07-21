#include "world/entity/animal/pig.h"
#include "world/entity/entity_types.h"
#include "world/item/item.h"

Pig::Pig(Level* level) : Animal(level) {
    setSize(0.9f, 0.9f);
    heightOffset = 0.0f;
    walkingSpeed = 0.1f;
    entityRendererId = ER_PIG_RENDERER;
    health = getMaxHealth();
}

int Pig::getEntityTypeId() const { return EntityTypes::IdPig; }

int Pig::getDeathLoot()          { return onFire > 0 ? ITEM_PORKCHOP_COOKED : ITEM_PORKCHOP_RAW; }
