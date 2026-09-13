#include "world/entity/player.h"
#include "world/inventory/inventory.h"
#include "world/item/spawn_egg_item.h"
#include "world/item/item_instance.h"
#include "world/entity/entity_types.h"
#include "world/entity/mob.h"
#include "world/entity/mob_factory.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "world/entity/mob_category.h"
#include "client/gui/hud.h"
#include "gpu/item_icons.h"
#include <cstdlib>

extern Level g_level;

SpawnEggItem::SpawnEggItem() : Item(ITEM_SPAWN_EGG) {
    maxStackSize = 64;
}

int SpawnEggItem::getIcon(short ) const { return II_SPAWN_EGG_BASE; }

static void spawnEggRefused(int why) {
    switch (why) {
        case MobCap::TOO_MANY_ANIMALS:
            hudChatMessage("Can't use Spawn Egg at the moment. The maximum number of "
                           "Pigs, Sheep and Cows has been reached.");
            break;
        case MobCap::TOO_MANY_CHICKENS:
            hudChatMessage("Can't use Spawn Egg at the moment. The maximum number of "
                           "Chickens in a world has been reached.");
            break;
        case MobCap::TOO_MANY_ENEMIES:
            hudChatMessage("Can't use Spawn Egg at the moment. The maximum number of "
                           "enemies in a world has been reached.");
            break;
        case MobCap::PEACEFUL:
            hudChatMessage("You can't spawn this creature while the difficulty "
                           "is set to Peaceful.");
            break;
        case MobCap::NO_ROOM:

            hudChatMessage("Can't use Spawn Egg at the moment. There is no room "
                           "left for more entities.");
            break;
    }
}

bool SpawnEggItem::useOn(ItemInstance* item, Player* player, World* world,
                         int x, int y, int z, int face, float, float, float) {
    if (face < 0 || face > 5) return true;
    int nx = x + kFaceNeighbor[face][0];
    int ny = y + kFaceNeighbor[face][1];
    int nz = z + kFaceNeighbor[face][2];

    int mobType = item ? item->data : 0;

    int why = MobCap::OK;
    if (!g_level.canCreateMore(mobType, Level::SPAWN_EGG, &why)) {
        spawnEggRefused(why);
        return true;
    }

    Mob* m = MobFactory::createMob(mobType, &g_level);
    if (!m) {
        spawnEggRefused(MobCap::NO_ROOM);
        return true;
    }

    float yo = (face == F_TOP && worldBlock(world, x, y, z) == BLOCK_FENCE) ? 0.5f : 0.0f;
    m->moveTo(nx + 0.5f, (float)ny + yo, nz + 0.5f, (float)(rand() % 360), 0.0f);
    g_level.addEntity(m);
    if (player) player->inventory->consumeSelected();
    return true;
}
