#include "world/item/seed_item.h"
#include "world/level/world.h"

bool SeedItem::useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face) {
    if (face != F_TOP) return false;
    if (worldBlock(world, x, y, z) != BLOCK_FARMLAND) return false;
    if (worldBlock(world, x, y + 1, z) != BLOCK_AIR) return false;

    worldSetBlockAndData(world, x, y + 1, z, resultTile, 0);
    worldUpdateLights(world);
    worldRebuildAroundNow(world, x, y + 1, z);
    return true;
}
