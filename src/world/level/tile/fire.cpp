#include "world/level/tile/fire.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/level.h"
#include "client/renderer/particle.h"
#include <stdlib.h>

extern Level g_level;

static inline float fr01() { return (rand() % 100) / 100.0f; }

static int flameOdds[256];
static int burnOdds[256];
static bool s_init = false;

static void setFlammable(int id, int flame, int burn) { flameOdds[id] = flame; burnOdds[id] = burn; }

void fireInitFlammables() {
    if (s_init) return;
    s_init = true;
    for (int i = 0; i < 256; i++) flameOdds[i] = burnOdds[i] = 0;

    setFlammable(BLOCK_PLANKS,   5,  20);
    setFlammable(BLOCK_LOG,      5,   5);
    setFlammable(BLOCK_LEAVES,  30,  60);
    setFlammable(BLOCK_BOOKSHELF,30,  20);
    setFlammable(BLOCK_TNT,     15, 100);
    setFlammable(BLOCK_WOOL,    30,  60);
}

bool fireCanBurn(const World* w, int x, int y, int z) { return flameOdds[worldBlock(w, x, y, z)] > 0; }

static int getFlammability(World* w, int x, int y, int z, int odds) {
    int f = flameOdds[worldBlock(w, x, y, z)];
    return f > odds ? f : odds;
}

static bool isValidFireLocation(World* w, int x, int y, int z) {
    return fireCanBurn(w, x + 1, y, z) || fireCanBurn(w, x - 1, y, z) ||
           fireCanBurn(w, x, y - 1, z) || fireCanBurn(w, x, y + 1, z) ||
           fireCanBurn(w, x, y, z - 1) || fireCanBurn(w, x, y, z + 1);
}

static int getFireOdds(World* w, int x, int y, int z) {
    if (worldBlock(w, x, y, z) != BLOCK_AIR) return 0;
    int odds = 0;
    odds = getFlammability(w, x + 1, y, z, odds);
    odds = getFlammability(w, x - 1, y, z, odds);
    odds = getFlammability(w, x, y - 1, z, odds);
    odds = getFlammability(w, x, y + 1, z, odds);
    odds = getFlammability(w, x, y, z - 1, odds);
    odds = getFlammability(w, x, y, z + 1, odds);
    return odds;
}

static void setTile(World* w, int x, int y, int z, unsigned char id) {
    if (x < 0 || x >= WORLD_W || y < 0 || y >= WORLD_H || z < 0 || z >= WORLD_D) return;
    worldSetBlockAndData(w, x, y, z, id, 0);
    worldNotifyNeighborsChanged(w, x, y, z);
    worldUpdateLights(w);
}

void firePlace(World* w, int x, int y, int z) {
    setTile(w, x, y, z, BLOCK_FIRE);
    worldScheduleTick(w, x, y, z, BLOCK_FIRE, FIRE_TICK_DELAY);
}

void fireExtinguishFx(int x, int y, int z) {

    for (int i = 0; i < 5; i++)
        particlesLargeSmoke((float)x + fr01(), (float)y + 0.3f + fr01() * 0.6f, (float)z + fr01());
    g_level.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, "random.fizz", 0.6f, 1.0f);
}

static void extinguish(World* w, int x, int y, int z) {
    for (int i = 0; i < 3; i++)
        particlesLargeSmoke((float)x + fr01(), (float)y + 0.3f + fr01() * 0.6f, (float)z + fr01());
    setTile(w, x, y, z, BLOCK_AIR);
}

static void checkBurn(World* w, int x, int y, int z, int chance) {
    int odds = burnOdds[worldBlock(w, x, y, z)];
    if ((rand() % chance) < odds) {
        bool wasTnt = worldBlock(w, x, y, z) == BLOCK_TNT;
        if (rand() % 2 == 0) firePlace(w, x, y, z);
        else                 setTile(w, x, y, z, BLOCK_AIR);
        if (wasTnt) worldPrimeTnt(w, x, y, z, 80);
    }
}

void fireTileTick(World* w, int x, int y, int z) {
    fireInitFlammables();

    bool infiniBurn = worldBlock(w, x, y - 1, z) == BLOCK_NETHERRACK;

    int age = worldData(w, x, y, z);

    if (age < 15) worldSetDataNoUpdate(w, x, y, z, (unsigned char)(age + 1));

    worldScheduleTick(w, x, y, z, BLOCK_FIRE, FIRE_TICK_DELAY);

    if (!infiniBurn && !isValidFireLocation(w, x, y, z)) {
        if (!isSolidBlocking(worldBlock(w, x, y - 1, z)) || age > 3) extinguish(w, x, y, z);
        return;
    }

    if (!infiniBurn && !fireCanBurn(w, x, y - 1, z)) {
        if (age == 15 && rand() % 4 == 0) { extinguish(w, x, y, z); return; }
    }

    if (age % 2 == 0 && age > 2) {
        checkBurn(w, x + 1, y, z, 300);
        checkBurn(w, x - 1, y, z, 300);
        checkBurn(w, x, y - 1, z, 250);
        checkBurn(w, x, y + 1, z, 250);
        checkBurn(w, x, y, z - 1, 300);
        checkBurn(w, x, y, z + 1, 300);

        for (int xx = x - 1; xx <= x + 1; xx++) {
            for (int zz = z - 1; zz <= z + 1; zz++) {
                for (int yy = y - 1; yy <= y + 4; yy++) {
                    if (xx == x && yy == y && zz == z) continue;
                    int rate = 100;
                    if (yy > y + 1) rate += (yy - (y + 1)) * 100;
                    int odds = getFireOdds(w, xx, yy, zz);
                    if (odds > 0 && (rand() % rate) <= odds) firePlace(w, xx, yy, zz);
                }
            }
        }
    }

    if (age == 15) {
        checkBurn(w, x + 1, y, z, 1);
        checkBurn(w, x - 1, y, z, 1);
        checkBurn(w, x, y - 1, z, 1);
        checkBurn(w, x, y + 1, z, 1);
        checkBurn(w, x, y, z - 1, 1);
        checkBurn(w, x, y, z + 1, 1);
    }

    if (worldBlock(w, x, y, z) == BLOCK_FIRE) {
        particlesLargeSmoke((float)x + fr01(), (float)y + 0.5f + fr01() * 0.5f, (float)z + fr01());
        if (rand() % 24 == 0)
            g_level.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, "fire.fire",
                              1.0f + fr01(), fr01() * 0.7f + 0.3f);
    }
}

bool fireMayPlace(World* w, int x, int y, int z) {
    return isSolidBlocking(worldBlock(w, x, y - 1, z)) || isValidFireLocation(w, x, y, z);
}

void fireNeighborChanged(World* w, int x, int y, int z) {
    if (!isSolidBlocking(worldBlock(w, x, y - 1, z)) && !isValidFireLocation(w, x, y, z))
        extinguish(w, x, y, z);
}
