#include "world/item/spawn_egg_item.h"
#include "world/item/item_instance.h"
#include "world/entity/entity_types.h"
#include "world/entity/mob.h"
#include "world/entity/mob_factory.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "gpu/item_icons.h"
#include <cstdlib>

extern Level g_level;

SpawnEggItem::SpawnEggItem() : Item(ITEM_SPAWN_EGG) {
    maxStackSize = 64;
}

int SpawnEggItem::getIcon(short ) const { return II_SPAWN_EGG_BASE; }

bool SpawnEggItem::useOn(ItemInstance* item, Player* , World* ,
                         int x, int y, int z, int face) {
    if (face < 0 || face > 5) return true;
    int nx = x + kFaceNeighbor[face][0];
    int ny = y + kFaceNeighbor[face][1];
    int nz = z + kFaceNeighbor[face][2];

    int mobType = item ? item->data : 0;
    Mob* m = MobFactory::createMob(mobType, &g_level);
    if (!m) return true;
    m->moveTo(nx + 0.5f, (float)ny, nz + 0.5f, (float)(rand() % 360), 0.0f);
    g_level.addEntity(m);
    if (item) item->count--;
    return true;
}
