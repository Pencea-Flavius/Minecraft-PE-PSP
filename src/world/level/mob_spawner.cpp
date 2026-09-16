#include "world/level/mob_spawner.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/entity/local_player.h"
#include "world/entity/mob.h"
#include "world/entity/mob_factory.h"
#include "world/entity/mob_category.h"
#include "world/inventory/inventory.h"
#include "world/entity/entity_types.h"
#include "world/entity/animal/animal.h"
#include "world/entity/monster/monster.h"
#include "world/difficulty.h"
#include "world/level/levelgen/Random.h"
#include <pspkernel.h>
#include <cmath>

namespace MobSpawner {

struct SpawnEntry { int mobId, weight, minCount, maxCount; };
static const SpawnEntry CREATURE_TABLE[] = {
    { EntityTypes::IdSheep,   12, 2, 3 },
    { EntityTypes::IdPig,     10, 1, 3 },
    { EntityTypes::IdChicken, 10, 2, 4 },
    { EntityTypes::IdCow,      8, 2, 3 },
};
static const int CREATURE_COUNT = (int)(sizeof(CREATURE_TABLE) / sizeof(CREATURE_TABLE[0]));
static const int CREATURE_TOTAL_WEIGHT = 40;

static const SpawnEntry MONSTER_TABLE[] = {
    { EntityTypes::IdZombie,   12, 2, 4 },
    { EntityTypes::IdSpider,    8, 2, 3 },
    { EntityTypes::IdSkeleton,  6, 1, 3 },
    { EntityTypes::IdCreeper,   4, 1, 1 },

};
static const int MONSTER_COUNT = (int)(sizeof(MONSTER_TABLE) / sizeof(MONSTER_TABLE[0]));

static const int MONSTER_TOTAL_WEIGHT = 30;

static const int MIN_SPAWN_DISTANCE = 24;

static const int MAX_SPAWN_CLUSTER = 4;

static const int SPAWN_ATTEMPTS = 8;

static const SpawnEntry& pickWeighted(const SpawnEntry* table, int n, int totalWeight,
                                      Random& rng) {
    int r = rng.nextInt(totalWeight);
    for (int i = 0; i < n; i++) {
        r -= table[i].weight;
        if (r < 0) return table[i];
    }
    return table[n - 1];
}

static Random s_rng((long)sceKernelGetSystemTimeLow());

typedef char assert_order_fits[((WORLD_W / 16) * (WORLD_D / 16) <= 256) ? 1 : -1];

static bool spawnOk(Level* L, int x, int y, int z) {
    if (y <= 0 || y + 1 >= WORLD_H) return false;
    if (!L->isSolidBlockingTile(x, y - 1, z)) return false;
    if (L->isSolidBlockingTile(x, y, z))      return false;
    if (L->isSolidBlockingTile(x, y + 1, z))  return false;
    unsigned char here = (unsigned char)L->getTile(x, y, z);
    if (isWaterId(here) || isLavaId(here)) return false;
    return true;
}

static const int SKY_ANCHOR_TRIES = 16;

static bool skyWorld() { return activeLevelSource().floatingIslands(); }

static int creatureY(Level* L, int x, int z) {
    const int top = L->getTopSolidBlock(x, z);
    if (!skyWorld()) return top;
    int ys[8], n = 0;
    for (int y = top; y > 0 && n < 8; y--)
        if (spawnOk(L, x, y, z)) { ys[n++] = y; y -= 2; }
    return n ? ys[s_rng.nextInt(n)] : top;
}

static int creatureYNear(Level* L, int x, int z, int refY) {
    if (!skyWorld()) return L->getTopSolidBlock(x, z);
    for (int d = 0; d <= 6; d++) {
        if (spawnOk(L, x, refY - d, z)) return refY - d;
        if (d && spawnOk(L, x, refY + d, z)) return refY + d;
    }
    return -1;
}

static void spawnCreatures(Level* level) {
    LocalPlayer* p = level->player;
    if (!p) return;

    float pfy = p->y - p->heightOffset;

    int mobCount = level->countInstanceOfBaseType(MobCategory::creature.baseType);
    for (int attempt = 0; attempt < SPAWN_ATTEMPTS; attempt++) {
        if (mobCount > MobCategory::creature.maxPerLevel) return;

        int cx = s_rng.nextInt(WORLD_W / 16);
        int cz = s_rng.nextInt(WORLD_D / 16);
        int bx = cx * 16 + s_rng.nextInt(16);
        int bz = cz * 16 + s_rng.nextInt(16);
        int by = creatureY(level, bx, bz);
        for (int t = 0; skyWorld() && t < SKY_ANCHOR_TRIES && !spawnOk(level, bx, by, bz); t++) {
            bx = cx * 16 + s_rng.nextInt(16);
            bz = cz * 16 + s_rng.nextInt(16);
            by = creatureY(level, bx, bz);
        }
        if (!spawnOk(level, bx, by, bz)) continue;

        float dx = bx + 0.5f - p->x, dy = by - pfy, dz = bz + 0.5f - p->z;
        if (dx * dx + dy * dy + dz * dz < (float)(MIN_SPAWN_DISTANCE * MIN_SPAWN_DISTANCE)) continue;

        const SpawnEntry& e = pickWeighted(CREATURE_TABLE, CREATURE_COUNT,
                                           CREATURE_TOTAL_WEIGHT, s_rng);
        int cluster = e.minCount + s_rng.nextInt(1 + e.maxCount - e.minCount);
        for (int i = 0; i < cluster; i++) {
            int sx = bx + s_rng.nextInt(6) - s_rng.nextInt(6);
            int sz = bz + s_rng.nextInt(6) - s_rng.nextInt(6);
            int sy = creatureYNear(level, sx, sz, by);
            if (!spawnOk(level, sx, sy, sz)) continue;
            Mob* m = MobFactory::createMob(e.mobId, level);
            if (!m) continue;
            m->moveTo(sx + 0.5f, (float)sy, sz + 0.5f, s_rng.nextFloat() * 360.0f, 0.0f);

            if (!m->canSpawn()) { delete m; continue; }

            if (s_rng.nextInt(2) == 0) ((Animal*)m)->setAge(-24000);
            level->addEntity(m);
            mobCount++;
        }
    }
}

static const int SURFACE_PROBE_ODDS = 4;
static const int PROBE_SNAP = 8;

static int probeStandableY(Level* L, int x, int z) {

    if (SURFACE_PROBE_ODDS > 0 && s_rng.nextInt(SURFACE_PROBE_ODDS) == 0) {
        int y = L->getTopSolidBlock(x, z);
        return spawnOk(L, x, y, z) ? y : -1;
    }
    int y0 = s_rng.nextInt(WORLD_H);
    for (int d = 0; d <= PROBE_SNAP; d++) {
        if (spawnOk(L, x, y0 - d, z)) return y0 - d;
        if (d && spawnOk(L, x, y0 + d, z)) return y0 + d;
    }
    return -1;
}

static void spawnMonsters(Level* level) {
    LocalPlayer* p = level->player;
    if (!p) return;
    if (level->getDifficulty() == Difficulty::PEACEFUL) return;

    int pcx = (int)floorf(p->x / 16.0f);
    int pcz = (int)floorf(p->z / 16.0f);

    const int R = 96 / 16;

    int mobCount = level->countInstanceOfBaseType(MobCategory::monster.baseType);
    if (mobCount > MobCategory::monster.maxPerLevel) return;

    for (int attempt = 0; attempt < SPAWN_ATTEMPTS; attempt++) {
        if (mobCount > MobCategory::monster.maxPerLevel) return;

        int cx = pcx + s_rng.nextInt(2 * R + 1) - R;
        int cz = pcz + s_rng.nextInt(2 * R + 1) - R;

        if (!level->hasChunksAt(cx * 16, 0, cz * 16, cx * 16 + 15, 0, cz * 16 + 15)) continue;

        int xStart = cx * 16 + s_rng.nextInt(16);
        int zStart = cz * 16 + s_rng.nextInt(16);
        int yStart = probeStandableY(level, xStart, zStart);
        if (yStart < 0) continue;

        if (level->isSolidBlockingTile(xStart, yStart, zStart)) continue;
        if (level->getTile(xStart, yStart, zStart) != BLOCK_AIR) continue;

        int clusterSize = 0;
        for (int pack = 0; pack < 3 && clusterSize < MAX_SPAWN_CLUSTER; pack++) {
            int x = xStart, y = yStart, z = zStart;
            const SpawnEntry* type = 0;
            int packMax = 0, packCount = 0;

            for (int tries = 0; tries < 4; tries++) {
                if (type && packCount > packMax) break;

                x += s_rng.nextInt(6) - s_rng.nextInt(6);
                z += s_rng.nextInt(6) - s_rng.nextInt(6);
                if (!spawnOk(level, x, y, z)) continue;

                float xx = x + 0.5f, yy = (float)y, zz = z + 0.5f;
                if (level->getNearestPlayer(xx, yy, zz, (float)MIN_SPAWN_DISTANCE)) continue;

                if (!type) {
                    type = &pickWeighted(MONSTER_TABLE, MONSTER_COUNT, MONSTER_TOTAL_WEIGHT, s_rng);

                    int typeMax = (int)(1.5f * type->weight * MobCategory::monster.maxPerLevel)
                                  / MONSTER_TOTAL_WEIGHT;
                    if (level->countInstanceOfType(type->mobId) >= typeMax) break;
                    packMax = type->minCount + s_rng.nextInt(1 + type->maxCount - type->minCount);
                }

                Mob* m = MobFactory::createMob(type->mobId, level);
                if (!m) continue;
                m->moveTo(xx, yy, zz, s_rng.nextFloat() * 360.0f, 0.0f);

                if (!m->canSpawn()) { delete m; continue; }
                level->addEntity(m);
                packCount++;
                mobCount++;
                if (++clusterSize >= MAX_SPAWN_CLUSTER) break;
            }
        }
    }
}

static const int GEN_CREATURE_CAP = 40;

void populateInitial(Level* level) {

    if (!level->player || level->player->inventory->isCreative()) return;

    if (!activeLevelSource().spawnsMobs()) return;

    s_rng.setSeed(worldSeed());
    const float CREATURE_PROBABILITY = 0.08f;
    const int NCHUNKS = (WORLD_W / 16) * (WORLD_D / 16);

    unsigned char order[256];
    for (int i = 0; i < NCHUNKS; i++) order[i] = (unsigned char)i;
    for (int i = NCHUNKS - 1; i > 0; i--) {
        int j = s_rng.nextInt(i + 1);
        unsigned char t = order[i]; order[i] = order[j]; order[j] = t;
    }

    for (int oi = 0; oi < NCHUNKS; oi++) {
        int cx = order[oi] % (WORLD_W / 16), cz = order[oi] / (WORLD_W / 16);
        int xo = cx * 16, zo = cz * 16;
        while (s_rng.nextFloat() < CREATURE_PROBABILITY) {
            if (level->countInstanceOfBaseType(MobCategory::creature.baseType)
                    >= GEN_CREATURE_CAP) return;
            const SpawnEntry& e = pickWeighted(CREATURE_TABLE, CREATURE_COUNT,
                                               CREATURE_TOTAL_WEIGHT, s_rng);
            int count = e.minCount + s_rng.nextInt(1 + e.maxCount - e.minCount);
            int x = xo + s_rng.nextInt(16), z = zo + s_rng.nextInt(16);
            int packY = creatureY(level, x, z);
            for (int t = 0; skyWorld() && t < SKY_ANCHOR_TRIES && !spawnOk(level, x, packY, z); t++) {
                x = xo + s_rng.nextInt(16); z = zo + s_rng.nextInt(16);
                packY = creatureY(level, x, z);
            }
            int startX = x, startZ = z;
            for (int c = 0; c < count; c++) {
                for (int a = 0; a < 4; a++) {
                    int y = creatureYNear(level, x, z, packY);
                    if (spawnOk(level, x, y, z)) {
                        Mob* m = MobFactory::createMob(e.mobId, level);
                        if (!m) return;
                        m->moveTo(x + 0.5f, (float)y, z + 0.5f, s_rng.nextFloat() * 360.0f, 0.0f);

                        if (s_rng.nextInt(2) == 0) ((Animal*)m)->setAge(-24000);
                        level->addEntity(m);
                        break;
                    }
                    x += s_rng.nextInt(5) - s_rng.nextInt(5);
                    z += s_rng.nextInt(5) - s_rng.nextInt(5);

                    while (x < xo || x >= xo + 16 || z < zo || z >= zo + 16) {
                        x = startX + s_rng.nextInt(5) - s_rng.nextInt(5);
                        z = startZ + s_rng.nextInt(5) - s_rng.nextInt(5);
                    }
                }
            }
        }
    }
}

void tick(Level* level, bool spawnEnemies, bool spawnFriendlies) {

    if (!level->player || level->player->inventory->isCreative()) return;

    if (!activeLevelSource().spawnsMobs()) return;

    if (spawnFriendlies) spawnCreatures(level);
    if (spawnEnemies)    spawnMonsters(level);
}

}
