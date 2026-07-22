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

struct SpawnEntry { int mobId, minCount, maxCount; };
static const SpawnEntry CREATURE_TABLE[] = {
    { EntityTypes::IdPig,     4, 4 },
    { EntityTypes::IdCow,     4, 4 },
    { EntityTypes::IdChicken, 4, 4 },
    { EntityTypes::IdSheep,   4, 4 },
};
static const int CREATURE_COUNT = (int)(sizeof(CREATURE_TABLE) / sizeof(CREATURE_TABLE[0]));

static const SpawnEntry MONSTER_TABLE[] = {
    { EntityTypes::IdZombie,    2, 4 },
    { EntityTypes::IdSkeleton,  2, 4 },
    { EntityTypes::IdCreeper,   1, 2 },
    { EntityTypes::IdSpider,    1, 3 },

};
static const int MONSTER_COUNT = (int)(sizeof(MONSTER_TABLE) / sizeof(MONSTER_TABLE[0]));

static const int MONSTER_CAP = 20;

static const int SPAWN_ATTEMPTS = 12;

static const int MIN_SPAWN_DISTANCE = 16;

static Random s_rng((long)sceKernelGetSystemTimeLow());

static bool spawnOk(Level* L, int x, int y, int z) {
    if (y <= 0 || y + 1 >= WORLD_H) return false;
    if (!L->isSolidBlockingTile(x, y - 1, z)) return false;
    if (L->isSolidBlockingTile(x, y, z))      return false;
    if (L->isSolidBlockingTile(x, y + 1, z))  return false;
    unsigned char here = (unsigned char)L->getTile(x, y, z);
    if (isWaterId(here) || isLavaId(here)) return false;
    return true;
}

static void spawnCreatures(Level* level) {
    LocalPlayer* p = level->player;
    if (!p) return;

    float pfy = p->y - p->heightOffset;
    for (int attempt = 0; attempt < SPAWN_ATTEMPTS; attempt++) {

        if (level->countInstanceOfBaseType(MobCategory::creature.baseType)
                >= MobCategory::creature.maxPerLevel) return;

        int cx = s_rng.nextInt(WORLD_W / 16);
        int cz = s_rng.nextInt(WORLD_D / 16);
        int bx = cx * 16 + s_rng.nextInt(16);
        int bz = cz * 16 + s_rng.nextInt(16);
        int by = level->getTopSolidBlock(bx, bz);
        if (!spawnOk(level, bx, by, bz)) continue;
        if (level->getTile(bx, by - 1, bz) != BLOCK_GRASS) continue;

        float dx = bx + 0.5f - p->x, dy = by - pfy, dz = bz + 0.5f - p->z;
        if (dx * dx + dy * dy + dz * dz < (float)(MIN_SPAWN_DISTANCE * MIN_SPAWN_DISTANCE)) continue;

        const SpawnEntry& e = CREATURE_TABLE[s_rng.nextInt(CREATURE_COUNT)];
        int cluster = e.minCount + s_rng.nextInt(1 + e.maxCount - e.minCount);
        for (int i = 0; i < cluster; i++) {
            int sx = bx + s_rng.nextInt(6) - s_rng.nextInt(6);
            int sz = bz + s_rng.nextInt(6) - s_rng.nextInt(6);
            int sy = level->getTopSolidBlock(sx, sz);
            if (!spawnOk(level, sx, sy, sz)) continue;
            if (level->getTile(sx, sy - 1, sz) != BLOCK_GRASS) continue;
            Mob* m = MobFactory::createMob(e.mobId, level);
            if (!m) continue;
            m->moveTo(sx + 0.5f, (float)sy, sz + 0.5f, s_rng.nextFloat() * 360.0f, 0.0f);

            if (s_rng.nextInt(2) == 0) ((Animal*)m)->setAge(-24000);
            level->addEntity(m);
        }
    }
}

static void spawnMonsters(Level* level) {
    LocalPlayer* p = level->player;
    if (!p) return;
    if (level->getDifficulty() == Difficulty::PEACEFUL) return;

    int pcx = (int)floorf(p->x / 16.0f);
    int pcz = (int)floorf(p->z / 16.0f);

    for (int attempt = 0; attempt < SPAWN_ATTEMPTS; attempt++) {
        if (level->countInstanceOfBaseType(MobCategory::monster.baseType) >= MONSTER_CAP) return;

        int cx = pcx + s_rng.nextInt(17) - 8;
        int cz = pcz + s_rng.nextInt(17) - 8;
        if (cx < 0 || cx >= WORLD_W / 16 || cz < 0 || cz >= WORLD_D / 16) continue;

        int bx = cx * 16 + s_rng.nextInt(16);
        int bz = cz * 16 + s_rng.nextInt(16);

        bool surfaceProbe = (s_rng.nextInt(2) == 0);
        int by = surfaceProbe ? level->getTopSolidBlock(bx, bz)
                              : 1 + s_rng.nextInt(WORLD_H - 2);
        if (!spawnOk(level, bx, by, bz)) continue;

        float pfy = p->y - p->heightOffset;
        float dx = bx + 0.5f - p->x, dy = by - pfy, dz = bz + 0.5f - p->z;
        if (dx * dx + dy * dy + dz * dz < (float)(MIN_SPAWN_DISTANCE * MIN_SPAWN_DISTANCE)) continue;

        const SpawnEntry& e = MONSTER_TABLE[s_rng.nextInt(MONSTER_COUNT)];
        int cluster = e.minCount + s_rng.nextInt(1 + e.maxCount - e.minCount);
        for (int i = 0; i < cluster; i++) {
            int sx = bx + s_rng.nextInt(6) - s_rng.nextInt(6);
            int sz = bz + s_rng.nextInt(6) - s_rng.nextInt(6);

            int sy = surfaceProbe ? level->getTopSolidBlock(sx, sz) : by;
            if (!spawnOk(level, sx, sy, sz)) continue;
            Mob* m = MobFactory::createMob(e.mobId, level);
            if (!m) continue;
            m->moveTo(sx + 0.5f, (float)sy, sz + 0.5f, s_rng.nextFloat() * 360.0f, 0.0f);

            if (!((Monster*)m)->canSpawn()) { delete m; continue; }
            level->addEntity(m);
        }
    }
}

void populateInitial(Level* level) {
    if (g_level.player->inventory->isCreative()) return;
    const float CREATURE_PROBABILITY = 0.10f;
    const int   cap = MobCategory::creature.maxPerLevel;
    for (int cz = 0; cz < WORLD_D / 16; cz++)
    for (int cx = 0; cx < WORLD_W / 16; cx++) {
        int xo = cx * 16, zo = cz * 16;
        while (s_rng.nextFloat() < CREATURE_PROBABILITY) {
            if (level->countInstanceOfBaseType(MobCategory::creature.baseType) >= cap) return;
            const SpawnEntry& e = CREATURE_TABLE[s_rng.nextInt(CREATURE_COUNT)];
            int count = e.minCount + s_rng.nextInt(1 + e.maxCount - e.minCount);
            int x = xo + s_rng.nextInt(16), z = zo + s_rng.nextInt(16);
            int startX = x, startZ = z;
            for (int c = 0; c < count; c++) {
                for (int a = 0; a < 4; a++) {
                    int y = level->getTopSolidBlock(x, z);
                    if (spawnOk(level, x, y, z) && level->getTile(x, y - 1, z) == BLOCK_GRASS) {
                        Mob* m = MobFactory::createMob(e.mobId, level);
                        if (!m) return;
                        m->moveTo(x + 0.5f, (float)y, z + 0.5f, s_rng.nextFloat() * 360.0f, 0.0f);
                        if (s_rng.nextInt(2) == 0) ((Animal*)m)->setAge(-24000);
                        level->addEntity(m);
                        break;
                    }
                    x += s_rng.nextInt(5) - s_rng.nextInt(5);
                    z += s_rng.nextInt(5) - s_rng.nextInt(5);
                    if (x < xo || x >= xo + 16 || z < zo || z >= zo + 16) { x = startX; z = startZ; }
                }
            }
        }
    }
}

void tick(Level* level, bool spawnEnemies, bool spawnFriendlies) {

    if (g_level.player->inventory->isCreative()) return;

    if (spawnFriendlies) spawnCreatures(level);
    if (spawnEnemies)    spawnMonsters(level);
}

}
