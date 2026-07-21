#include "world/entity/animal/cow.h"
#include "world/entity/entity_types.h"
#include "world/item/item.h"

Cow::Cow(Level* level) : Animal(level) {
    setSize(0.9f, 1.3f);
    heightOffset = 0.0f;
    walkingSpeed = 0.1f;
    entityRendererId = ER_COW_RENDERER;
    health = getMaxHealth();
}

int Cow::getEntityTypeId() const { return EntityTypes::IdCow; }

void Cow::dropDeathLoot() {
    int beef = 1 + sharedRandom.nextInt(3);
    short meat = onFire > 0 ? ITEM_BEEF_COOKED : ITEM_BEEF_RAW;
    for (int i = 0; i < beef; i++) spawnAtLocation(meat, 1);
    int leather = sharedRandom.nextInt(3);
    for (int i = 0; i < leather; i++) spawnAtLocation(ITEM_LEATHER, 1);
}
