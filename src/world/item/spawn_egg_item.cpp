#include "world/item/spawn_egg_item.h"
#include "world/item/item_instance.h"
#include "world/entity/entity_types.h"
#include "world/entity/mob.h"
#include "world/entity/mob_factory.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include <cstdlib>

extern Level g_level;

static const int EGG_ICON = 153;

SpawnEggItem::SpawnEggItem() : Item(ITEM_SPAWN_EGG) {
    maxStackSize = 64;
}

int SpawnEggItem::getIcon(short data) const {
    switch (data) {

        case 12: return 225;
        case 11: return 226;
        case 10: return 227;
        case 13: return 228;
        case 32: return 229;
        case 33: return 230;
        case 34: return 231;
        case 35: return 232;
        case 36: return 233;
        default: return EGG_ICON;
    }
}

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
