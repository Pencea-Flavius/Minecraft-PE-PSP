#include "world/entity/mob_factory.h"
#include "world/entity/entity_types.h"
#include "world/entity/mob.h"
#include "world/entity/entity.h"
#include "world/entity/animal/pig.h"
#include "world/entity/animal/cow.h"
#include "world/entity/animal/chicken.h"
#include "world/entity/animal/sheep.h"
#include "world/entity/monster/zombie.h"
#include "world/entity/monster/skeleton.h"
#include "world/entity/monster/creeper.h"
#include "world/entity/monster/spider.h"
#include "world/entity/monster/pig_zombie.h"
#include <cstdlib>

namespace MobFactory {

Mob* createMob(int mobType, Level* level) {

    if (!Entity::hasFreeSlot()) return 0;
    Mob* r = 0;
    switch (mobType) {
        case EntityTypes::IdPig:      r = new Pig(level); break;
        case EntityTypes::IdCow:      r = new Cow(level); break;
        case EntityTypes::IdChicken:  r = new Chicken(level); break;
        case EntityTypes::IdSheep:    r = new Sheep(level); break;
        case EntityTypes::IdZombie:   r = new Zombie(level); break;
        case EntityTypes::IdSkeleton: r = new Skeleton(level); break;
        case EntityTypes::IdCreeper:  r = new Creeper(level); break;
        case EntityTypes::IdSpider:   r = new Spider(level); break;
        case EntityTypes::IdPigZombie:r = new PigZombie(level); break;
    }
    return r;
}

}
