#include "world/level/tile/nether_reactor_pattern.h"
#include "world/level/chunk/chunk.h"

NetherReactorPattern::NetherReactorPattern() {
    const int gold  = BLOCK_GOLD_BLOCK;
    const int cob   = BLOCK_COBBLESTONE;
    const int core  = BLOCK_NETHER_REACTOR;
    const int types[3][3][3] = {

        { { gold, cob, gold }, { cob, cob, cob }, { gold, cob, gold } },

        { { cob, 0, cob }, { 0, core, 0 }, { cob, 0, cob } },

        { { 0, cob, 0 }, { cob, cob, cob }, { 0, cob, 0 } },
    };
    for (int l = 0; l <= 2; ++l)
        for (int x = 0; x <= 2; ++x)
            for (int z = 0; z <= 2; ++z)
                setTileAt(l, x, z, types[l][x][z]);
}

void NetherReactorPattern::setTileAt(int level, int x, int z, int tile) {
    pattern[level][x][z] = tile;
}

int NetherReactorPattern::getTileAt(int level, int x, int z) const {
    return pattern[level][x][z];
}
