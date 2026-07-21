#include "world/entity/entity_factory.h"
#include "world/entity/entity.h"
#include "world/entity/entity_types.h"
#include "world/entity/painting.h"
#include "world/entity/arrow.h"
#include "world/entity/falling_tile.h"
#include "world/entity/primed_tnt.h"
#include "world/entity/item_entity.h"
#include "world/entity/throwable.h"
#include "world/entity/animal/pig.h"
#include "world/entity/animal/cow.h"
#include "world/entity/animal/chicken.h"
#include "world/entity/animal/sheep.h"
#include "world/entity/monster/zombie.h"
#include "world/entity/monster/skeleton.h"
#include "world/entity/monster/creeper.h"
#include "world/entity/monster/spider.h"
#include "world/entity/monster/pig_zombie.h"
#include "nbt/compound_tag.h"

namespace EntityFactory {

Entity* createEntity(int typeId, Level* level) {
    switch (typeId) {
        case EntityTypes::IdPig:      return new Pig(level);
        case EntityTypes::IdCow:      return new Cow(level);
        case EntityTypes::IdChicken:  return new Chicken(level);
        case EntityTypes::IdSheep:    return new Sheep(level);
        case EntityTypes::IdZombie:   return new Zombie(level);
        case EntityTypes::IdSkeleton: return new Skeleton(level);
        case EntityTypes::IdCreeper:  return new Creeper(level);
        case EntityTypes::IdSpider:   return new Spider(level);
        case EntityTypes::IdPigZombie:return new PigZombie(level);
        case EntityTypes::IdPainting: return new Painting(level);
        case EntityTypes::IdArrow:    return new Arrow(level);
        case EntityTypes::IdFallingTile: return new FallingTile(level);
        case EntityTypes::IdPrimedTnt: return new PrimedTnt(level);
        case EntityTypes::IdItemEntity:  return new ItemEntity(level);
        case EntityTypes::IdSnowball:    return new Throwable(level, EntityTypes::IdSnowball);
        case EntityTypes::IdThrownEgg:   return new Throwable(level, EntityTypes::IdThrownEgg);
    }
    return 0;
}

Entity* loadEntity(CompoundTag* tag, Level* level) {
    if (!tag || !tag->contains("id")) return 0;
    int id = tag->getInt("id");
    Entity* e = createEntity(id, level);
    if (e) e->load(tag);
    return e;
}

}
