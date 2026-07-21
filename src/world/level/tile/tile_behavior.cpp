
#include "world/level/tile/tile_behavior.h"
#include "world/level/tile/tile.h"
#include "world/level/level.h"
#include "world/entity/falling_tile.h"
#include <stdlib.h>

static inline bool heavyIsFree(World* w, int x, int y, int z) {
    unsigned char id = worldBlock(w, x, y, z);
    return id == BLOCK_AIR || isLiquidId(id);
}

void heavyTileTick(World* w, int x, int y, int z, unsigned char id) {
    if (y >= 0 && heavyIsFree(w, x, y - 1, z)) {
        FallingTile* e = new FallingTile(&g_level, x + 0.5f, y + 0.5f, z + 0.5f,
                                         id, worldData(w, x, y, z));
        g_level.addEntity(e);
    }
}

bool tileMayPlace(World* w, unsigned char id, int x, int y, int z, int data) {
    return Tile::tiles[id]->mayPlace(w, x, y, z, data);
}

void tileNeighborChanged(World* w, int x, int y, int z) {
    unsigned char id = worldBlock(w, x, y, z);
    if (id == BLOCK_ORE_REDSTONE) {
        extern void redstoneOreInteract(World* w, int x, int y, int z);
        redstoneOreInteract(w, x, y, z);
    }

    if (isHeavyTile(id)) { worldScheduleTick(w, x, y, z, id, 2); return; }
    if (Tile::tiles[id]->mayPlace(w, x, y, z, -1)) return;
    worldSpawnResources(w, x, y, z, id, worldData(w, x, y, z));
    worldSetBlockAndData(w, x, y, z, BLOCK_AIR, 0);
    worldNotifyNeighborsChanged(w, x, y, z);
}

void tileRandomTick(World* w) {
    for (int cx = 0; cx < WORLD_CHUNKS_X; cx++)
    for (int cz = 0; cz < WORLD_CHUNKS_Z; cz++) {
        int xo = cx * CHUNK_SX, zo = cz * CHUNK_SZ;
        for (int i = 0; i < 20; i++) {
            int x = xo + (rand() % CHUNK_SX), z = zo + (rand() % CHUNK_SZ), y = rand() % WORLD_H;
            Tile* t = Tile::tiles[worldBlock(w, x, y, z)];
            if (t->randomTicks) t->randomTick(w, x, y, z);
        }
    }
}
