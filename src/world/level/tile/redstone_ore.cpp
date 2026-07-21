#include "world/level/tile/redstone_ore.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "client/renderer/particle.h"

namespace {
struct Lit { int x, y, z, t; };
const int MAX_LIT = 48;

const int REDSTONE_LIT_TICKS = 40;
Lit g_lit[MAX_LIT];
int g_litCount = 0;

void relight(World* w, int x, int y, int z) {
    worldUpdateLights(w);
    worldRebuildAroundNow(w, x, y, z);
}
}

void redstoneOreReset() { g_litCount = 0; }

void redstoneOreInteract(World* w, int x, int y, int z) {
    unsigned char id = worldBlock(w, x, y, z);
    if (id != BLOCK_ORE_REDSTONE && id != BLOCK_ORE_REDSTONE_LIT) return;

    int slot = -1;
    for (int i = 0; i < g_litCount; i++)
        if (g_lit[i].x == x && g_lit[i].y == y && g_lit[i].z == z) { slot = i; break; }
    if (slot < 0 && g_litCount < MAX_LIT) { slot = g_litCount++; g_lit[slot].x = x; g_lit[slot].y = y; g_lit[slot].z = z; }
    if (slot >= 0) g_lit[slot].t = REDSTONE_LIT_TICKS;

    if (id == BLOCK_ORE_REDSTONE_LIT) return;
    if (slot < 0) return;

    particlesRedstonePoof(w, x, y, z);
    worldSetBlockAndData(w, x, y, z, BLOCK_ORE_REDSTONE_LIT, worldData(w, x, y, z));
    relight(w, x, y, z);
}

void redstoneOreTick(World* w) {
    for (int i = 0; i < g_litCount; ) {
        if (--g_lit[i].t <= 0) {
            int x = g_lit[i].x, y = g_lit[i].y, z = g_lit[i].z;
            if (worldBlock(w, x, y, z) == BLOCK_ORE_REDSTONE_LIT) {
                worldSetBlockAndData(w, x, y, z, BLOCK_ORE_REDSTONE, worldData(w, x, y, z));
                relight(w, x, y, z);
            }
            g_lit[i] = g_lit[--g_litCount];
        } else {
            i++;
        }
    }
}
