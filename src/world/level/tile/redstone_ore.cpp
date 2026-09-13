#include "world/level/tile/redstone_ore.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "client/renderer/particle.h"

static void relight(World* w, int x, int y, int z) {
    worldUpdateLights(w);
    worldRebuildAroundNow(w, x, y, z);
}

void redstoneOreInteract(World* w, int x, int y, int z) {

    particlesRedstonePoof(w, x, y, z);
    if (worldBlock(w, x, y, z) != BLOCK_ORE_REDSTONE) return;
    worldSetBlockAndData(w, x, y, z, BLOCK_ORE_REDSTONE_LIT, worldData(w, x, y, z));
    relight(w, x, y, z);
    worldScheduleTick(w, x, y, z, BLOCK_ORE_REDSTONE_LIT, REDSTONE_LIT_DELAY);
}

void redstoneOreRevert(World* w, int x, int y, int z) {
    worldSetBlockAndData(w, x, y, z, BLOCK_ORE_REDSTONE, worldData(w, x, y, z));
    relight(w, x, y, z);
}
