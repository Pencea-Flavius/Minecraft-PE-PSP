
#include "world/entity/local_player.h"
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/level/tile/tile.h"
#include "world/level/levelgen/Random.h"
#include "world/inventory/inventory.h"
#include "world/entity/item_entity.h"
#include <pspkernel.h>

void Tile::popResource(int x, int y, int z, const ItemInstance& item) {

    if (!Entity::hasFreeSlot()) return;
    const float s = 0.7f;
    float xo = (rand() / (float)RAND_MAX) * s + (1 - s) * 0.5f;
    float yo = (rand() / (float)RAND_MAX) * s + (1 - s) * 0.5f;
    float zo = (rand() / (float)RAND_MAX) * s + (1 - s) * 0.5f;
    ItemEntity* e = new ItemEntity(&g_level, x + xo, y + yo, z + zo, item);
    e->throwTime = 10;
    g_level.addEntity(e);
}

void worldSpawnResources(World* w, int x, int y, int z, unsigned char id, int data, float odds) {

    if (g_level.isClientSide) return;

    if (g_level.player->inventory->isCreative()) return;
    static Random rng((long)sceKernelGetSystemTimeLow());
    Tile::tiles[id]->spawnResources(w, x, y, z, data, rng, odds);
}
