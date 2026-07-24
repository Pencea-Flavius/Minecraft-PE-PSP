
#include "world/level/tile/entity/reactor_tile_entity.h"
#include "world/level/tile/nether_reactor_pattern.h"
#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/item/item.h"
#include "world/entity/entity.h"
#include "world/entity/entity_types.h"
#include "world/entity/item_entity.h"
#include "world/entity/mob.h"
#include "world/entity/mob_factory.h"
#include "world/entity/local_player.h"
#include "world/difficulty.h"
#include "world/phys/aabb.h"
#include "client/gui/hud.h"
#include "world/level/levelgen/Random.h"
#include "util/mth.h"
#include "nbt/compound_tag.h"
#include <pspkernel.h>
#include <cmath>

static const int TPS = Entity::TicksPerSecond;

static Random& rng() {
    static Random r((long)sceKernelGetSystemTimeLow());
    return r;
}

namespace NetherReactor {

void setPhase(Level* level, int x, int y, int z, int phase) {
    if (level->getData(x, y, z) != phase)
        level->setData(x, y, z, phase);
}

static bool allPlayersCloseToReactor(Level* level, int x, int y, int z) {
    LocalPlayer* p = level->player;
    if (!p) return false;
    if (!(p->x >= x - 5 && p->x <= x + 5)) return false;
    if (!(p->y - p->heightOffset >= y - 1 && p->y - p->heightOffset <= y + 1)) return false;
    if (!(p->z >= z - 5 && p->z <= z + 5)) return false;
    return true;
}

static bool canSpawnStartNetherReactor(Level* level, int x, int y, int z, LocalPlayer* player) {
    (void)player;
    if (!allPlayersCloseToReactor(level, x, y, z)) {
        hudChatMessage("All players need to be close to the reactor.");
        return false;
    } else if (y > WORLD_H - 28) {
        hudChatMessage("The nether reactor needs to be built lower down.");
        return false;
    } else if (y < 2) {
        hudChatMessage("The nether reactor needs to be built higher up.");
        return false;
    }
    return true;
}

bool use(Level* level, int x, int y, int z, LocalPlayer* player) {
    NetherReactorPattern pattern;
    for (int checkLevel = 0; checkLevel <= 2; ++checkLevel)
        for (int checkX = -1; checkX <= 1; ++checkX)
            for (int checkZ = -1; checkZ <= 1; ++checkZ)
                if (level->getTile(x + checkX, y + checkLevel - 1, z + checkZ)
                        != pattern.getTileAt(checkLevel, checkX + 1, checkZ + 1)) {
                    hudChatMessage("Not the correct pattern!");
                    return false;
                }
    if (!canSpawnStartNetherReactor(level, x, y, z, player)) return false;
    hudChatMessage("Active!");
    ReactorTileEntity* reactor = static_cast<ReactorTileEntity*>(level->getTileEntity(x, y, z));
    if (reactor != 0) reactor->lightItUp(x, y, z);
    return true;
}

}

ReactorTileEntity::ReactorTileEntity()
    : super(TE_REACTOR), isInitialized(false), progress(0), curLevel(0), hasFinished(false) {}

void ReactorTileEntity::lightItUp(int x, int y, int z) {
    curLevel = 0;
    NetherReactor::setPhase(level, x, y, z, 1);
    isInitialized = true;

    level->setNightMode(true);
    buildDome(x, y, z);
}

void ReactorTileEntity::tick() {
    if (level->isClientSide) return;
    if (progress < 0) removed = true;
    if (isInitialized && !hasFinished) {
        progress++;
        if (progress % TPS == 0) {
            int currentTime = progress / TPS;
            if (currentTime < 10)
                tickGlowingRedstoneTransformation(currentTime);
            if (currentTime > 42 && currentTime <= 45)
                turnGlowingObsidianLayerToObsidian(45 - currentTime);
            if (checkLevelChange(progress / TPS)) {
                curLevel++;
                spawnItems(getNumItemsPerLevel(curLevel));
                trySpawnPigZombies(NUM_PIG_ZOMBIE_SLOTS, getNumEnemiesPerLevel(curLevel));
            }
        }
        if (progress > TPS * 46)
            finishReactorRun();
    } else if (hasFinished) {
        if (progress % (TPS * 60) == 0) {
            if (playersAreCloseBy()) trySpawnPigZombies(2, 3);
            else                     killPigZombies();
        }
    }
}

void ReactorTileEntity::load(CompoundTag* tag) {
    super::load(tag);
    isInitialized = tag->getBoolean("IsInitialized");
    if (isInitialized) {
        progress = tag->getShort("Progress");
        hasFinished = tag->getBoolean("HasFinished");

        if (!hasFinished) level->setNightMode(true);
    }
}

bool ReactorTileEntity::save(CompoundTag* tag) {
    super::save(tag);
    tag->putBoolean("IsInitialized", isInitialized);
    tag->putShort("Progress", progress);
    tag->putBoolean("HasFinished", hasFinished);
    return true;
}

int ReactorTileEntity::getNumEnemiesPerLevel(int curLevel) {
    if (curLevel == 0)      return 3;
    else if (curLevel < 4)  return 2;
    else if (curLevel < 6)  return Mth::Max(0, rng().nextInt(2));
    else                    return Mth::Max(0, rng().nextInt(1));
}

int ReactorTileEntity::getNumItemsPerLevel(int curLevel) {
    if (curLevel == 0)      return 3 * 3;
    else if (curLevel < 4)  return 5 * 3;
    else if (curLevel < 8)  return Mth::Max(0, rng().nextInt(14 * 3) - 4);
    else                    return Mth::Max(0, rng().nextInt(9 * 3) - 2);
}

void ReactorTileEntity::spawnItems(int numItems) {
    for (int i = 0; i < numItems; i++) spawnItem();
}

Vec3 ReactorTileEntity::getSpawnPosition(float minDistance, float variableDistance, float offset) {
    float distance = minDistance + rng().nextFloat() * variableDistance;
    float rad = rng().nextFloat() * (2.0f * Mth::PI);
    return Vec3(sinf(rad) * distance + x, offset + y, cosf(rad) * distance + z);
}

void ReactorTileEntity::spawnEnemy() {
    Mob* mob = MobFactory::createMob(EntityTypes::IdPigZombie, level);
    if (!mob) return;
    Vec3 pos = getSpawnPosition(3, 4, -1);
    while (pos.x < 0 || pos.z < 0 || pos.x >= WORLD_W || pos.z >= WORLD_D)
        pos = getSpawnPosition(3, 4, -1);
    mob->moveTo(pos.x, pos.y, pos.z, 0, 0);
    level->addEntity(mob);
}

void ReactorTileEntity::spawnItem() {
    Vec3 pos = getSpawnPosition(3, 4, -1);
    while (pos.x < 0 || pos.z < 0 || pos.x >= WORLD_W || pos.z >= WORLD_D)
        pos = getSpawnPosition(3, 4, -1);
    ItemEntity* item = new ItemEntity(level, pos.x, pos.y, pos.z, getSpawnItem());
    item->throwTime = 10;
    item->age = item->getLifeTime() - TPS * 30;
    level->addEntity(item);
}

ItemInstance ReactorTileEntity::getSpawnItem() {
    switch (rng().nextInt(8)) {
        case 0: return ItemInstance(ITEM_GLOWSTONE_DUST, 3, 0);
        case 1: return ItemInstance(ITEM_SEEDS_MELON, 1, 0);
        case 2: return ItemInstance(BLOCK_MUSHROOM_BROWN, 1, 0);
        case 3: return ItemInstance(BLOCK_MUSHROOM_RED, 1, 0);
        case 4: return ItemInstance(ITEM_REEDS, 1, 0);
        case 5: return ItemInstance(BLOCK_CACTUS, 1, 0);
        case 6: return ItemInstance(ITEM_NETHER_QUARTZ, 4, 0);
        default: return GetLowOddsSpawnItem();
    }
}

ItemInstance ReactorTileEntity::GetLowOddsSpawnItem() {
    if (rng().nextInt(10) <= 9) {
        static const short items[] = {
            ITEM_ARROW, ITEM_BED_ITEM, ITEM_BONE, ITEM_BOOK, ITEM_BOW,
            ITEM_BOWL, ITEM_FEATHER, ITEM_PAINTING, ITEM_DOOR_WOOD_ITEM
        };
        int n = (int)(sizeof(items) / sizeof(items[0]));
        return ItemInstance(items[rng().nextInt(n)], 1, 0);
    }
    return ItemInstance(BLOCK_BOOKSHELF, 1, 0);
}

bool ReactorTileEntity::checkLevelChange(int progress) {
    static const int levelChangeTime[] = {10, 13, 20, 22, 25, 30, 34, 36, 38, 40};
    for (int a = 0; a < (int)(sizeof(levelChangeTime) / sizeof(int)); ++a)
        if (levelChangeTime[a] == progress) return true;
    return false;
}

void ReactorTileEntity::finishReactorRun() {
    NetherReactor::setPhase(level, x, y, z, 2);
    level->setNightMode(false);
    hasFinished = true;

    for (int cx = x - 1; cx <= x + 1; ++cx)
        for (int cy = y - 1; cy <= y + 1; ++cy)
            for (int cz = z - 1; cz <= z + 1; ++cz)
                if (cx != x || cy != y || cz != z)
                    level->setTile(cx, cy, cz, BLOCK_OBSIDIAN);
    deterioateDome(x, y, z);
}

int ReactorTileEntity::numOfFreeEnemySlots() {
    int found = 0;
    AABB bb((float)x, (float)y, (float)z, x + 1.0f, y + 1.0f, z + 1.0f);
    std::vector<Entity*> nearby = level->getEntities(0, bb.grow(7, 7, 7));
    for (size_t i = 0; i < nearby.size(); i++)
        if (nearby[i]->isEntityType(EntityTypes::IdPigZombie) && nearby[i]->isAlive())
            found++;
    return NUM_PIG_ZOMBIE_SLOTS - found;
}

void ReactorTileEntity::trySpawnPigZombies(int maxNumOfEnemies, int maxToSpawn) {
    if (level->getDifficulty() == Difficulty::PEACEFUL) return;
    int cur = NUM_PIG_ZOMBIE_SLOTS - numOfFreeEnemySlots();
    if (cur < maxNumOfEnemies)
        for (int a = 0; a < maxToSpawn && cur < maxNumOfEnemies; ++a) { spawnEnemy(); cur++; }
}

void ReactorTileEntity::tickGlowingRedstoneTransformation(int currentTime) {
    switch (currentTime) {
        case 2: return turnLayerToGlowingObsidian(0, BLOCK_COBBLESTONE);
        case 3: return turnLayerToGlowingObsidian(1, BLOCK_COBBLESTONE);
        case 4: return turnLayerToGlowingObsidian(2, BLOCK_COBBLESTONE);
        case 7: return turnLayerToGlowingObsidian(0, BLOCK_GOLD_BLOCK);
        case 8: return turnLayerToGlowingObsidian(1, BLOCK_GOLD_BLOCK);
        case 9: return turnLayerToGlowingObsidian(2, BLOCK_GOLD_BLOCK);
    }
}

void ReactorTileEntity::turnLayerToGlowingObsidian(int layer, int type) {
    NetherReactorPattern pattern;
    for (int cx = -1; cx <= 1; ++cx)
        for (int cz = -1; cz <= 1; ++cz)
            if (pattern.getTileAt(layer, cx + 1, cz + 1) == type)
                level->setTile(x + cx, y - 1 + layer, z + cz, BLOCK_GLOWING_OBSIDIAN);
}

void ReactorTileEntity::turnGlowingObsidianLayerToObsidian(int layer) {
    for (int cx = -1; cx <= 1; ++cx)
        for (int cz = -1; cz <= 1; ++cz)
            if (level->getTile(x + cx, y - 1 + layer, z + cz) != BLOCK_NETHER_REACTOR)
                level->setTile(x + cx, y - 1 + layer, z + cz, BLOCK_OBSIDIAN);
}

void ReactorTileEntity::relightDomeColumns(int x, int y, int z, int radius) {
    World* w = level->w;
    for (int cx = x - radius; cx <= x + radius; ++cx)
        for (int cz = z - radius; cz <= z + radius; ++cz) {
            if (cx < 0 || cx >= WORLD_W || cz < 0 || cz >= WORLD_D) continue;
            lightOnBlockChanged(w, cx, y, cz);
        }
}

void ReactorTileEntity::settleDomeEdit(int x, int y, int z, int radius) {
    World* w = level->w;
    relightDomeColumns(x, y - 3, z, radius);

    while (!w->lightQueue.empty()) worldUpdateLights(w);

    int yBottom = y - 3, yTop = y + 31;
    if (yBottom < 0) yBottom = 0;
    if (yTop > WORLD_H) yTop = WORLD_H;
    int siFirst = yBottom / SECTION_SY, siLast = (yTop - 1) / SECTION_SY;
    if (siLast >= N_SECTIONS) siLast = N_SECTIONS - 1;
    for (int cz = (z - radius) >> 4; cz <= ((z + radius) >> 4); ++cz)
        for (int cx = (x - radius) >> 4; cx <= ((x + radius) >> 4); ++cx) {
            if (cx < 0 || cx >= WORLD_CHUNKS_X || cz < 0 || cz >= WORLD_CHUNKS_Z) continue;
            ChunkMesh* c = &w->chunks[cz * WORLD_CHUNKS_X + cx];
            for (int si = siFirst; si <= siLast; ++si) chunkBuildSection(c, w, si);
        }
}

void ReactorTileEntity::buildDome(int x, int y, int z) {
    World* w = level->w;
    bool wasLightReady = w->lightReady;
    w->lightReady = false;
    buildFloorVolume(x, y - 3, z, 8, 2, BLOCK_NETHERRACK);
    buildHollowedVolume(x, y - 1, z, 8, 4, BLOCK_NETHERRACK, 0);
    buildFloorVolume(x, y - 1 + 4, z, 8, 1, BLOCK_NETHERRACK);
    buildCrockedRoofVolume(false, x, y - 1 + 5, z, 8, 1, BLOCK_NETHERRACK);
    buildCrockedRoofVolume(true,  x, y - 1 + 6, z, 5, 8, BLOCK_NETHERRACK);
    buildCrockedRoofVolume(false, x, y - 1 + 12, z, 3, 14, BLOCK_NETHERRACK);
    w->lightReady = wasLightReady;
    if (wasLightReady) settleDomeEdit(x, y, z, 8);
}

void ReactorTileEntity::buildHollowedVolume(int x, int y, int z, int expandWidth, int height, int wallTileId, int clearTileId) {
    for (int cy = 0; cy < height; ++cy)
        for (int cx = -expandWidth; cx <= expandWidth; ++cx)
            for (int cz = -expandWidth; cz <= expandWidth; ++cz) {
                if (cx == -expandWidth || cx == expandWidth || cz == -expandWidth || cz == expandWidth)
                    level->setTile(cx + x, cy + y, cz + z, wallTileId);
                else if (cy > 2 || cx < -1 || cx > 1 || cz < -1 || cz > 1)
                    level->setTile(cx + x, cy + y, cz + z, clearTileId);
            }
}

void ReactorTileEntity::buildFloorVolume(int x, int y, int z, int expandWidth, int height, int tileId) {
    for (int cy = 0; cy < height; ++cy)
        for (int cx = -expandWidth; cx <= expandWidth; ++cx)
            for (int cz = -expandWidth; cz <= expandWidth; ++cz)
                level->setTile(cx + x, cy + y, cz + z, tileId);
}

void ReactorTileEntity::buildCrockedRoofVolume(bool inverted, int x, int y, int z, int expandWidth, int height, int tileId) {
    int fullHeight = height + expandWidth;
    for (int cx = -expandWidth; cx <= expandWidth; ++cx)
        for (int cz = -expandWidth; cz <= expandWidth; ++cz) {
            int offset = inverted ? ((-cx - cz) / 2) : ((cx + cz) / 2);
            int acceptHeight = fullHeight + offset;
            for (int cy = 0; cy < fullHeight + expandWidth; ++cy)
                if (acceptHeight >= cy && (isEdge(cx, expandWidth, cz) || acceptHeight == cy))
                    level->setTile(cx + x, cy + y, cz + z, tileId);
        }
}

bool ReactorTileEntity::isEdge(int curX, int expandWidth, int curZ) {
    return curX == -expandWidth || curX == expandWidth || curZ == -expandWidth || curZ == expandWidth;
}

void ReactorTileEntity::deterioateDome(int x, int y, int z) {

    World* w = level->w;
    bool wasLightReady = w->lightReady;
    w->lightReady = false;
    deterioateHollowedVolume(x, y - 1, z, 8, 5, 0);
    deterioateCrockedRoofVolume(false, x, y - 1 + 5, z, 8, 1, 0);
    deterioateCrockedRoofVolume(true,  x, y - 1 + 6, z, 5, 8, 0);
    deterioateCrockedRoofVolume(false, x, y - 1 + 12, z, 3, 14, 0);
    w->lightReady = wasLightReady;
    if (wasLightReady) settleDomeEdit(x, y, z, 8);
}

void ReactorTileEntity::deterioateCrockedRoofVolume(bool inverted, int x, int y, int z, int expandWidth, int height, int tileId) {
    int fullHeight = height + expandWidth;
    for (int cx = -expandWidth; cx <= expandWidth; ++cx)
        for (int cz = -expandWidth; cz <= expandWidth; ++cz) {
            int offset = inverted ? ((-cx - cz) / 2) : ((cx + cz) / 2);
            int acceptHeight = fullHeight + offset;
            for (int cy = 0; cy < fullHeight + expandWidth; ++cy)
                if (acceptHeight >= cy && isEdge(cx, expandWidth, cz))
                    if (rng().nextInt(3) == 0)
                        level->setTile(cx + x, cy + y, cz + z, tileId);
        }
}

void ReactorTileEntity::deterioateHollowedVolume(int x, int y, int z, int expandWidth, int height, int tileId) {
    for (int cy = 0; cy < height; ++cy)
        for (int cx = -expandWidth; cx <= expandWidth; ++cx)
            for (int cz = -expandWidth; cz <= expandWidth; ++cz)
                if (cx == -expandWidth || cx == expandWidth || cz == -expandWidth || cz == expandWidth)
                    if (rng().nextInt(2) == 0)
                        level->setTile(cx + x, cy + y, cz + z, tileId);
}

bool ReactorTileEntity::playersAreCloseBy() {

    LocalPlayer* p = level->player;
    return p && p->isAlive() && p->distanceTo((float)x, (float)y, (float)z) < 40;
}

void ReactorTileEntity::killPigZombies() {
    AABB bb((float)x, (float)y, (float)z, x + 1.0f, y + 1.0f, z + 1.0f);
    std::vector<Entity*> nearby = level->getEntities(0, bb.grow(40, 40, 40));
    for (size_t i = 0; i < nearby.size(); i++)
        if (nearby[i]->isEntityType(EntityTypes::IdPigZombie))
            nearby[i]->remove();
}
