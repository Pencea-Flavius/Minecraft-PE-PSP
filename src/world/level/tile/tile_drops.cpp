
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/level/tile/tile.h"
#include "world/level/levelgen/Random.h"
#include "world/inventory/inventory.h"
#include <pspkernel.h>

void worldSpawnResources(World* w, int x, int y, int z, unsigned char id, int data) {

    if (g_level.isClientSide) return;

    if (g_inv.isCreative()) return;
    static Random rng((long)sceKernelGetSystemTimeLow());
    Tile::tiles[id]->spawnResources(w, x, y, z, data, rng);
}
