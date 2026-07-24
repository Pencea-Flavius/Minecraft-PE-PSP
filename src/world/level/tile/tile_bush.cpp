
#include "world/level/tile/tile_behavior.h"
#include "world/level/levelgen/features.h"
#include "world/level/levelgen/Random.h"
#include <stdlib.h>

bool bushMayPlaceOn(World* w, unsigned char id, int x, int y, int z) {
    unsigned char below = worldBlock(w, x, y - 1, z);
    switch (id) {
        case BLOCK_FLOWER: case BLOCK_ROSE: case BLOCK_SAPLING:

            return below == BLOCK_GRASS || below == BLOCK_DIRT || below == BLOCK_FARMLAND;
        case BLOCK_WHEAT: case BLOCK_MELON_STEM:

            return below == BLOCK_FARMLAND;
        case BLOCK_MUSHROOM_BROWN: case BLOCK_MUSHROOM_RED:

            return isOpaque(below);
        default:
            return true;
    }
}

bool bushFamilyCanSurvive(World* w, unsigned char id, int x, int y, int z) {
    if (!bushMayPlaceOn(w, id, x, y, z)) return false;

    if (id == BLOCK_MUSHROOM_BROWN || id == BLOCK_MUSHROOM_RED)
        return lightRawAt(w, x, y, z) < 13;
    return lightRawAt(w, x, y, z) >= 8 || worldCanSeeSky(w, x, y, z);
}

void saplingGrow(World* w, int x, int y, int z) {
    unsigned char data = worldData(w, x, y, z);
    Random rnd(rand());
    worldSetBlockAndData(w, x, y, z, BLOCK_AIR, 0);
    if ((data & 3) == 1)      treeSpruce(w, rnd, x, y, z);
    else if ((data & 3) == 2) treeBirch(w, rnd, x, y, z);
    else                      treeOak(w, rnd, x, y, z);
    if (worldBlock(w, x, y, z) == BLOCK_AIR)
        worldSetBlockAndData(w, x, y, z, BLOCK_SAPLING, data);
}

void saplingTick(World* w, int x, int y, int z) {
    if (lightRawAt(w, x, y + 1, z) < 9 || rand() % 7 != 0) return;
    unsigned char data = worldData(w, x, y, z);
    if ((data & 8) == 0) {

        worldSetDataNoUpdate(w, x, y, z, (unsigned char)(data | 8));
        return;
    }
    saplingGrow(w, x, y, z);
}

void mushroomTick(World* w, int x, int y, int z) {
    if (rand() % 25 != 0) return;
    unsigned char id = worldBlock(w, x, y, z);
    int max = 5;
    for (int xx = x - 4; xx <= x + 4; xx++)
    for (int zz = z - 4; zz <= z + 4; zz++)
    for (int yy = y - 1; yy <= y + 1; yy++)
        if (worldBlock(w, xx, yy, zz) == id && --max <= 0) return;

    int x2 = x + rand() % 3 - 1;
    int y2 = y + (rand() % 2) - (rand() % 2);
    int z2 = z + rand() % 3 - 1;
    for (int i = 0; i < 4; i++) {
        if (worldBlock(w, x2, y2, z2) == BLOCK_AIR && bushFamilyCanSurvive(w, id, x2, y2, z2)) {
            x = x2; y = y2; z = z2;
        }
        x2 = x + rand() % 3 - 1;
        y2 = y + (rand() % 2) - (rand() % 2);
        z2 = z + rand() % 3 - 1;
    }
    if (worldBlock(w, x2, y2, z2) == BLOCK_AIR && bushFamilyCanSurvive(w, id, x2, y2, z2)) {
        worldSetBlockAndData(w, x2, y2, z2, id, 0);
        worldNotifyNeighborsChanged(w, x2, y2, z2);
    }
}

static float cropGrowthSpeed(World* w, unsigned char id, int x, int y, int z) {
    float speed = 1.0f;
    bool horizontal = worldBlock(w, x - 1, y, z) == id || worldBlock(w, x + 1, y, z) == id;
    bool vertical   = worldBlock(w, x, y, z - 1) == id || worldBlock(w, x, y, z + 1) == id;
    bool diagonal   = worldBlock(w, x - 1, y, z - 1) == id || worldBlock(w, x + 1, y, z - 1) == id ||
                      worldBlock(w, x + 1, y, z + 1) == id || worldBlock(w, x - 1, y, z + 1) == id;
    for (int xx = x - 1; xx <= x + 1; xx++)
    for (int zz = z - 1; zz <= z + 1; zz++) {
        float tileSpeed = 0.0f;
        if (worldBlock(w, xx, y - 1, zz) == BLOCK_FARMLAND)
            tileSpeed = worldData(w, xx, y - 1, zz) > 0 ? 3.0f : 1.0f;
        if (xx != x || zz != z) tileSpeed /= 4.0f;
        speed += tileSpeed;
    }
    if (diagonal || (horizontal && vertical)) speed /= 2.0f;
    return speed;
}

static int cropGrowthOdds(World* w, unsigned char id, int x, int y, int z, int bias) {
    int odds = (int)(25.0f / cropGrowthSpeed(w, id, x, y, z)) + bias;
    return odds < 1 ? 1 : odds;
}

void cropTick(World* w, int x, int y, int z) {
    if (lightRawAt(w, x, y, z) < 9) return;
    unsigned char age = worldData(w, x, y, z);
    if (age >= 7) return;
    if (rand() % cropGrowthOdds(w, worldBlock(w, x, y, z), x, y, z, 0) != 0) return;
    worldSetData(w, x, y, z, age + 1);
}

void stemTick(World* w, int x, int y, int z) {
    if (lightRawAt(w, x, y + 1, z) < 9) return;
    if (rand() % cropGrowthOdds(w, BLOCK_MELON_STEM, x, y, z, 1) != 0) return;
    unsigned char age = worldData(w, x, y, z);
    if (age < 7) { worldSetData(w, x, y, z, age + 1); return; }
    static const signed char dir[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
    for (int i = 0; i < 4; i++)
        if (worldBlock(w, x + dir[i][0], y, z + dir[i][1]) == BLOCK_MELON) return;
    for (int i = 0; i < 4; i++) {
        int nx = x + dir[i][0], nz = z + dir[i][1];
        if (worldBlock(w, nx, y, nz) != BLOCK_AIR) continue;
        unsigned char below = worldBlock(w, nx, y - 1, nz);
        if (below != BLOCK_FARMLAND && below != BLOCK_DIRT && below != BLOCK_GRASS) continue;
        worldSetBlockAndData(w, nx, y, nz, BLOCK_MELON, 0);
        worldNotifyNeighborsChanged(w, nx, y, nz);
        return;
    }
}
