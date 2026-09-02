#include "world/entity/entity_factory.h"
#include "world/entity/entity.h"
#include "world/entity/entity_types.h"
#include "world/entity/painting.h"
#include "world/entity/arrow.h"
#include "world/entity/falling_tile.h"
#include "world/entity/primed_tnt.h"
#include "world/entity/item_entity.h"
#include "world/entity/throwable.h"
#include "world/entity/minecart.h"
#include "world/entity/tripod_camera.h"
#include "world/entity/animal/pig.h"
#include "world/entity/animal/cow.h"
#include "world/entity/animal/chicken.h"
#include "world/entity/animal/sheep.h"
#include "world/entity/monster/zombie.h"
#include "world/entity/monster/skeleton.h"
#include "world/entity/monster/creeper.h"
#include "world/entity/monster/spider.h"
#include "world/entity/monster/pig_zombie.h"
#include "world/entity/local_player.h"
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
        case EntityTypes::IdMinecart:    return new Minecart(level);
        case EntityTypes::IdTripodCamera: return new TripodCamera(level);
        case EntityTypes::IdSnowball:    return new Throwable(level, EntityTypes::IdSnowball);
        case EntityTypes::IdThrownEgg:   return new Throwable(level, EntityTypes::IdThrownEgg);
    }
    return 0;
}

Entity* loadEntity(CompoundTag* tag, Level* level) {
    if (!tag || !tag->contains("id")) return 0;
    int id = tag->getInt("id");
    Entity* e = createEntity(id, level);
    if (!e) return 0;

    if (!e->load(tag)) { delete e; return 0; }
    return e;
}

}

#define FITS(T) static_assert(sizeof(T) <= Entity::ENTITY_SLOT, #T " outgrew ENTITY_SLOT")
FITS(LocalPlayer); FITS(Chicken); FITS(Cow); FITS(Pig); FITS(Sheep);
FITS(Zombie); FITS(Skeleton); FITS(Creeper); FITS(Spider); FITS(PigZombie);
FITS(ItemEntity); FITS(Arrow); FITS(Minecart); FITS(PrimedTnt);
FITS(Painting); FITS(FallingTile); FITS(TripodCamera);
#undef FITS
