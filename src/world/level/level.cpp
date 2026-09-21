#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/material.h"
#include "world/entity/entity.h"
#include "world/entity/local_player.h"
#include "world/difficulty.h"
#include "world/entity/mob_category.h"
#include "world/level/tile/entity/tile_entity.h"
#include "client/player/physics.h"
#include "world/level/pathfinder/path_finder.h"
#include "world/level/mob_spawner.h"
#include "world/level/tile/tile.h"
#include "world/level/tile/entity/tile_entity_factory.h"
#include "platform/audio/sound.h"
#include "util/mth.h"

Level::Level(World* world) : w(world), player(0), isClientSide(false),
                             spawnX(WORLD_W / 2), spawnY(64), spawnZ(WORLD_D / 2),
                             caveMood(0.0f) {
    entities.reserve(Entity::ENTITY_POOL + 16);
    boxes.reserve(64);
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) chunkEntityHead[i] = 0;
    for (int i = 0; i < MAX_TILE_CRACKS; i++) tileCracks[i].id = 0;
}

void Level::validateSpawn() { worldValidateSpawn(w, &spawnX, &spawnY, &spawnZ); }

static inline int colOf(float x, float z) {
    return (((int)Mth::floor(z) >> 4) & (WORLD_CHUNKS_Z - 1)) * WORLD_CHUNKS_X
         + (((int)Mth::floor(x) >> 4) & (WORLD_CHUNKS_X - 1));
}

void Level::linkEntity(Entity* e) {
    if (!e || e->inChunk) return;
    int col = colOf(e->x, e->z);
    e->xChunk = col % WORLD_CHUNKS_X;
    e->zChunk = col / WORLD_CHUNKS_X;
    e->yChunk = Mth::floor(e->y) >> 4;
    e->nextInChunk = chunkEntityHead[col];
    chunkEntityHead[col] = e;
    e->inChunk = true;
}

void Level::unlinkEntity(Entity* e) {
    if (!e || !e->inChunk) return;
    int col = e->zChunk * WORLD_CHUNKS_X + e->xChunk;
    Entity** p = &chunkEntityHead[col];
    while (*p && *p != e) p = &(*p)->nextInChunk;
    if (*p) *p = e->nextInChunk;
    e->nextInChunk = 0;
    e->inChunk = false;
}

void Level::relinkIfMoved(Entity* e) {
    if (!e) return;
    int col = colOf(e->x, e->z);
    if (e->inChunk && col == e->zChunk * WORLD_CHUNKS_X + e->xChunk) {
        e->yChunk = Mth::floor(e->y) >> 4;
        return;
    }
    unlinkEntity(e);
    linkEntity(e);
}

static inline void colRange(const AABB& b, int* cx0, int* cx1, int* cz0, int* cz1) {
    *cx0 = ((int)Mth::floor(b.x0 - 2.0f)) >> 4; *cx1 = ((int)Mth::floor(b.x1 + 2.0f)) >> 4;
    *cz0 = ((int)Mth::floor(b.z0 - 2.0f)) >> 4; *cz1 = ((int)Mth::floor(b.z1 + 2.0f)) >> 4;
}

static inline int colIdx(int cx, int cz) {
    return (cz & (WORLD_CHUNKS_Z - 1)) * WORLD_CHUNKS_X + (cx & (WORLD_CHUNKS_X - 1));
}

int Level::getTile(int x, int y, int z) const {
    return worldBlock(w, x, y, z);
}
int Level::getData(int x, int y, int z) const {
    return worldData(w, x, y, z);
}
void Level::setTile(int x, int y, int z, int id) {
    worldSetBlockAndData(w, x, y, z, (unsigned char)id, 0);
}
void Level::setData(int x, int y, int z, int data) {
    worldSetData(w, x, y, z, (unsigned char)data);
    worldRebuildAroundNow(w, x, y, z);
}
void Level::setNightMode(bool night) {
    worldSetNightMode(w, night);
}
int Level::getRawBrightness(int x, int y, int z) const {
    return lightRawAt(w, x, y, z);
}
float Level::getBrightness(int x, int y, int z) const {

    return g_brightRamp[getRawBrightness(x, y, z)];
}
bool Level::hasChunksAt(int x0, int y0, int z0, int x1, int y1, int z1) const {

    if (y1 < 0 || y0 >= WORLD_H) return false;
    for (int cz = z0 >> 4; cz <= (z1 >> 4); cz++)
        for (int cx = x0 >> 4; cx <= (x1 >> 4); cx++) {

            if (!worldChunkInBounds(cx, cz)) continue;
            if (!worldChunkSettled(w, cx, cz)) return false;
        }
    return true;
}

bool Level::isSolidBlockingTile(int x, int y, int z) const {
    return isOpaque((unsigned char)worldBlock(w, x, y, z));
}

bool Level::containsAnyLiquid(const AABB& box) const {
    int x0 = Mth::floor(box.x0), x1 = Mth::floor(box.x1 + 1);
    int y0 = Mth::floor(box.y0), y1 = Mth::floor(box.y1 + 1);
    int z0 = Mth::floor(box.z0), z1 = Mth::floor(box.z1 + 1);
    if (box.x0 < 0) x0--;
    if (box.y0 < 0) y0--;
    if (box.z0 < 0) z0--;
    for (int x = x0; x < x1; x++)
        for (int y = y0; y < y1; y++)
            for (int z = z0; z < z1; z++) {
                unsigned char id = (unsigned char)worldBlock(w, x, y, z);
                if (isWaterId(id) || isLavaId(id)) return true;
            }
    return false;
}

bool Level::containsFireTile(const AABB& box) const {
    int x0 = Mth::floor(box.x0), x1 = Mth::floor(box.x1 + 1);
    int y0 = Mth::floor(box.y0), y1 = Mth::floor(box.y1 + 1);
    int z0 = Mth::floor(box.z0), z1 = Mth::floor(box.z1 + 1);
    if (box.x0 < 0) x0--;
    if (box.y0 < 0) y0--;
    if (box.z0 < 0) z0--;
    for (int x = x0; x < x1; x++)
        for (int y = y0; y < y1; y++)
            for (int z = z0; z < z1; z++) {
                unsigned char id = (unsigned char)worldBlock(w, x, y, z);
                if (id == BLOCK_FIRE || isLavaId(id)) return true;
            }
    return false;
}

void Level::handleFallOn(int x, int y, int z, Entity* e, float dist) const {
    int t = getTile(x, y, z);
    if (t > 0) Tile::tiles[t]->fallOn(w, x, y, z, e, dist);
}

bool Level::isSolidTile(int x, int y, int z) const {
    return materialOf((unsigned char)worldBlock(w, x, y, z)).isSolid();
}

std::vector<AABB>& Level::getCubes(Entity* , const AABB& box) {
    std::vector<AABB>& out = boxes;
    out.clear();
    int x0 = Mth::floor(box.x0), x1 = Mth::floor(box.x1);
    int y0 = Mth::floor(box.y0), y1 = Mth::floor(box.y1);
    int z0 = Mth::floor(box.z0), z1 = Mth::floor(box.z1);

    for (int x = x0; x <= x1; x++)
        for (int y = y0 - 1; y <= y1; y++)
            for (int z = z0; z <= z1; z++) {
                BlockAABB b[3];
                int n = getBlockAABBs(w, x, y, z, b);
                for (int i = 0; i < n; i++) {
                    AABB a(b[i].x0, b[i].y0, b[i].z0, b[i].x1, b[i].y1, b[i].z1);
                    if (a.intersects(box)) out.push_back(a);
                }
            }
    return out;
}

enum { FILTER_ANY = -0x7fffffff };
static int gather(Entity* const* heads, const AABB& box, const Entity* except,
                  int wantType, int wantClass, EntityList& out);

void Level::getEntities(Entity* except, const AABB& box, EntityList& out) const {
    gather(chunkEntityHead, box, except, FILTER_ANY, FILTER_ANY, out);
}
int Level::getEntitiesOfType(int entityType, const AABB& box, EntityList& out) const {
    return gather(chunkEntityHead, box, 0, entityType, FILTER_ANY, out);
}
int Level::getEntitiesOfClass(int baseType, const AABB& box, EntityList& out) const {
    return gather(chunkEntityHead, box, 0, FILTER_ANY, baseType, out);
}

static int gather(Entity* const* heads, const AABB& box, const Entity* except,
                  int wantType, int wantClass, EntityList& out) {
    out.clear();
    int cx0, cx1, cz0, cz1;
    colRange(box, &cx0, &cx1, &cz0, &cz1);
    for (int cz = cz0; cz <= cz1; cz++)
        for (int cx = cx0; cx <= cx1; cx++)
            for (Entity* e = heads[colIdx(cx, cz)]; e; e = e->nextInChunk) {
                if (e == except || e->removed) continue;
                if (wantType  != FILTER_ANY && e->getEntityTypeId()     != wantType)  continue;
                if (wantClass != FILTER_ANY && e->getCreatureBaseType() != wantClass) continue;
                if (e->bb.intersects(box)) out.push_back(e);
            }
    return (int)out.size();
}

bool Level::isUnobstructed(const AABB& box) const {

    int cx0, cx1, cz0, cz1;
    colRange(box, &cx0, &cx1, &cz0, &cz1);
    for (int cz = cz0; cz <= cz1; cz++)
        for (int cx = cx0; cx <= cx1; cx++)
            for (Entity* e = chunkEntityHead[colIdx(cx, cz)]; e; e = e->nextInChunk)
                if (!e->removed && e->blocksBuilding && e->bb.intersects(box)) return false;

    if (player && !player->removed && player->bb.intersects(box)) return false;
    return true;
}

bool Level::isDay() const { return g_skyDarken < 4; }

LocalPlayer* Level::getNearestPlayer(Entity* from, float maxDist) const {
    if (!player || !player->isAlive()) return 0;
    if (maxDist >= 0.0f && from->distanceToSqr((Entity*)player) > maxDist * maxDist)
        return 0;
    return player;
}

static PathFinder s_pathFinder;

bool Level::findPath(Path* path, Entity* from, Entity* to, float maxDist, bool openDoors, bool avoidWater) {
    s_pathFinder.setLevel(this);
    s_pathFinder.avoidWater = avoidWater;
    s_pathFinder.passDoors = openDoors;
    return s_pathFinder.findPath(path, from, to, maxDist);
}
bool Level::findPath(Path* path, Entity* from, int x, int y, int z, float maxDist, bool openDoors, bool avoidWater) {
    s_pathFinder.setLevel(this);
    s_pathFinder.avoidWater = avoidWater;
    s_pathFinder.passDoors = openDoors;
    return s_pathFinder.findPath(path, from, x, y, z, maxDist);
}

bool Level::isLoadedAt(float x, float z) const {
    return worldChunkSettled(w, Mth::floor(x) >> 4, Mth::floor(z) >> 4);
}

int Level::countInstanceOfBaseType(int baseType) const {
    int n = 0;
    for (size_t i = 0; i < entities.size(); i++) {
        Entity* e = entities[i];
        if (e && !e->removed && e->isMob() && e->getCreatureBaseType() == baseType &&
            isLoadedAt(e->x, e->z)) n++;
    }
    return n;
}

int Level::countInstanceOfType(int entityType) const {
    int n = 0;
    for (size_t i = 0; i < entities.size(); i++) {
        Entity* e = entities[i];
        if (e && !e->removed && e->getEntityTypeId() == entityType &&
            isLoadedAt(e->x, e->z)) n++;
    }
    return n;
}

bool Level::canCreateMore(int mobType, SpawnAttempt how, int* why) const {
    if (why) *why = MobCap::OK;

    const bool egg = (how == SPAWN_EGG);
    const int  base = MobCategory::baseTypeOf(mobType);

    if (base == EntityTypes::BaseEnemy) {
        if (g_difficulty == Difficulty::PEACEFUL) {
            if (why) *why = MobCap::PEACEFUL;
            return false;
        }

        int max = egg ? MobCategory::MAX_MONSTERS_EGG : MobCategory::monster.maxPerLevel;
        if (countInstanceOfBaseType(EntityTypes::BaseEnemy) >= max) {
            if (why) *why = MobCap::TOO_MANY_ENEMIES;
            return false;
        }
        return true;
    }

    if (base == EntityTypes::BaseCreature) {

        if (mobType == EntityTypes::IdChicken) {
            int max = egg ? MobCategory::MAX_CHICKENS_EGG : MobCategory::MAX_CHICKENS_BREED;
            if (countInstanceOfType(EntityTypes::IdChicken) >= max) {
                if (why) *why = MobCap::TOO_MANY_CHICKENS;
                return false;
            }
        }
        int max = egg ? MobCategory::MAX_ANIMALS_EGG : MobCategory::MAX_ANIMALS_BREED;
        if (countInstanceOfBaseType(EntityTypes::BaseCreature) >= max) {
            if (why) *why = MobCap::TOO_MANY_ANIMALS;
            return false;
        }
        return true;
    }

    return true;
}

Entity* Level::getNearestPlayer(float px, float py, float pz, float maxDist) const {
    if (!player || player->removed) return 0;
    float dx = player->x - px, dy = player->y - py, dz = player->z - pz;
    if (dx * dx + dy * dy + dz * dz <= maxDist * maxDist) return (Entity*)player;
    return 0;
}

Entity* Level::getEntity(int id) const {
    if (id == 0) return 0;
    if (player && player->entityId == id) return (Entity*)player;
    for (size_t i = 0; i < entities.size(); i++) {
        Entity* e = entities[i];
        if (e && !e->removed && e->entityId == id) return e;
    }
    return 0;
}

int Level::getDifficulty() const { return g_difficulty; }

int Level::getTopSolidBlock(int x, int z) const {
    if (!worldReady(w, x, z)) return WORLD_H;
    return w->heightmap[worldColumn(w, x, z)];
}

void Level::addEntity(Entity* e) {
    entities.push_back(e);
    linkEntity(e);
}

static const int SPAWN_INTERVAL = 2;

void Level::tickEntities() {

    static int s_spawnTimer = 0;
    if (++s_spawnTimer >= SPAWN_INTERVAL) { s_spawnTimer = 0; MobSpawner::tick(this, true, false); }

    if ((w->time % 400) < 2) MobSpawner::tick(this, false, true);

    for (size_t i = 0; i < entities.size(); i++) {
        Entity* e = entities[i];

        if (!e->removed && isLoadedAt(e->x, e->z)) {

            if (e->riding) e->rideTick(); else e->tick();

            relinkIfMoved(e);
        }
    }

    for (size_t i = 0; i < entities.size(); ) {
        if (entities[i]->removed) {

            Entity* dead = entities[i];
            if (dead->rider)  { dead->positionRider(true); dead->rider->riding = 0; dead->rider = 0; }
            if (dead->riding) { dead->riding->rider = 0; dead->riding = 0; }
            unlinkEntity(entities[i]);
            delete entities[i];
            entities[i] = entities.back();
            entities.pop_back();
        } else {
            i++;
        }
    }
}

void Level::removeAllEntities() {

    for (size_t i = 0; i < entities.size(); i++) { entities[i]->rider = 0; entities[i]->riding = 0; }
    for (size_t i = 0; i < entities.size(); i++) delete entities[i];
    entities.clear();
    for (int i = 0; i < WORLD_CHUNKS_X * WORLD_CHUNKS_Z; i++) chunkEntityHead[i] = 0;
    for (int i = 0; i < MAX_TILE_CRACKS; i++) tileCracks[i].id = 0;
}

void Level::destroyTileProgress(int id, int x, int y, int z, int progress) {
    TileCrack* slot = 0;
    for (int i = 0; i < MAX_TILE_CRACKS; i++) {
        if (tileCracks[i].id == id) { slot = &tileCracks[i]; break; }
        if (!slot && tileCracks[i].id == 0) slot = &tileCracks[i];
    }
    if (progress < 0 || progress > 9) {
        if (slot && slot->id == id) slot->id = 0;
        return;
    }
    if (!slot) return;
    slot->id = id; slot->x = x; slot->y = y; slot->z = z; slot->progress = progress;
}

TileEntity* Level::findTileEntity(int x, int y, int z) {
    for (size_t i = 0; i < tileEntities.size(); i++) {
        TileEntity* te = tileEntities[i];
        if (te && !te->removed && te->x == x && te->y == y && te->z == z) return te;
    }
    return 0;
}

TileEntity* Level::getTileEntity(int x, int y, int z) {
    if (TileEntity* found = findTileEntity(x, y, z)) return found;
    int id = getTile(x, y, z);
    if (id <= 0 || !Tile::isEntityTile[id]) return 0;
    TileEntity* made = TileEntityFactory::createTileEntity(Tile::tiles[id]->getTileEntityType());
    if (!made) return 0;
    setTileEntity(x, y, z, made);

    return findTileEntity(x, y, z);
}

void Level::setTileEntity(int x, int y, int z, TileEntity* te) {

    if (!te) return;
    removeTileEntity(x, y, z);

    if (findTileEntity(x, y, z)) { delete te; return; }
    te->setLevelAndPos(this, x, y, z);
    tileEntities.push_back(te);
}

void Level::removeTileEntity(int x, int y, int z) {
    for (size_t i = 0; i < tileEntities.size(); ) {
        TileEntity* te = tileEntities[i];
        if (te && te->x == x && te->y == y && te->z == z) {
            te->setRemoved();
            if (!te->isRemoved()) { i++; continue; }
            delete te;
            tileEntities[i] = tileEntities.back();
            tileEntities.pop_back();
        } else {
            i++;
        }
    }
}

void Level::removeAllTileEntities() {
    for (size_t i = 0; i < tileEntities.size(); i++) delete tileEntities[i];
    tileEntities.clear();
}

void Level::tickTileEntities() {
    for (size_t i = 0; i < tileEntities.size(); ) {
        TileEntity* te = tileEntities[i];

        if (te && !te->removed && isLoadedAt((float)te->x, (float)te->z)) te->tick();
        if (te && te->removed) {
            delete te;
            tileEntities[i] = tileEntities.back();
            tileEntities.pop_back();
        } else {
            i++;
        }
    }
}

#include "world/level/tile/entity/furnace_tile_entity.h"
bool g_furnaceNoDrop = false;
void furnaceSetLitBlock(Level* level, int x, int y, int z, bool lit) {
    if (!level || !level->w) return;
    unsigned char id = (unsigned char)level->getTile(x, y, z);
    if (id != BLOCK_FURNACE && id != BLOCK_FURNACE_LIT) return;
    unsigned char data = (unsigned char)level->getData(x, y, z);

    TileEntity* te = level->getTileEntity(x, y, z);
    FurnaceTileEntity* fte = (te && te->type == TE_FURNACE) ? (FurnaceTileEntity*)te : 0;
    if (fte) fte->keepOnRemove = true;
    g_furnaceNoDrop = true;
    worldSetBlockAndData(level->w, x, y, z, lit ? BLOCK_FURNACE_LIT : BLOCK_FURNACE, data);
    g_furnaceNoDrop = false;
    if (fte) fte->keepOnRemove = false;
    worldRebuildAroundNow(level->w, x, y, z);
}

bool Level::isInWater(Entity* e, const AABB& box) const {
    int x0 = Mth::floor(box.x0), x1 = Mth::floor(box.x1 + 1);
    int y0 = Mth::floor(box.y0), y1 = Mth::floor(box.y1 + 1);
    int z0 = Mth::floor(box.z0), z1 = Mth::floor(box.z1 + 1);
    if (!hasChunksAt(x0, y0, z0, x1, y1, z1)) return false;
    bool ok = false;
    float cx = 0, cy = 0, cz = 0;
    for (int x = x0; x < x1; x++)
        for (int y = y0; y < y1; y++)
            for (int z = z0; z < z1; z++) {
                unsigned char id = worldBlock(w, x, y, z);
                if (!isWaterId(id)) continue;

                float yt0 = y + 1 - liquidTileHeight(worldData(w, x, y, z));
                if (!((float)y1 >= yt0)) continue;
                ok = true;
                float fx, fy, fz;
                liquidFlow(w, x, y, z, id, &fx, &fy, &fz);
                cx += fx * 0.5f; cy += fy * 0.5f; cz += fz * 0.5f;
            }

    float len = Mth::sqrt(cx * cx + cy * cy + cz * cz);
    if (len > 0.0f && e) {
        float p = 0.004f / len;
        e->xd += cx * p; e->yd += cy * p; e->zd += cz * p;
    }
    return ok;
}

bool Level::isInLava(const AABB& box) const {
    int x0 = Mth::floor(box.x0), x1 = Mth::floor(box.x1 + 1);
    int y0 = Mth::floor(box.y0), y1 = Mth::floor(box.y1 + 1);
    int z0 = Mth::floor(box.z0), z1 = Mth::floor(box.z1 + 1);
    for (int x = x0; x < x1; x++)
        for (int y = y0; y < y1; y++)
            for (int z = z0; z < z1; z++)
                if (isLavaId(worldBlock(w, x, y, z))) return true;
    return false;
}

void Level::playSound(float x, float y, float z, const char* name, float volume, float pitch) const {
    if (!player) return;
    float dx = x - player->x, dy = y - player->y, dz = z - player->z;
    float v = soundAttenuate(dx * dx + dy * dy + dz * dz, volume);
    if (v > 0.0f) soundPlay(name, v, pitch);
}

void Level::tickCaveMood() {
    if (!player) return;

    const int   kExtent    = 8;
    const float kTickDelay = 6000.0f;
    const float kPosOffset = 2.0f;

    int bx = Mth::floor(player->x) + (rand() % (2 * kExtent + 1)) - kExtent;
    int by = Mth::floor(player->y) + (rand() % (2 * kExtent + 1)) - kExtent;
    int bz = Mth::floor(player->z) + (rand() % (2 * kExtent + 1)) - kExtent;

    if (!hasChunksAt(bx, by, bz, bx, by, bz)) return;

    int sky = lightSkyGet(w, bx, by, bz);
    if (sky > 0) caveMood -= (sky / 15.0f) * 0.001f;
    else         caveMood -= (lightBlockGet(w, bx, by, bz) - 1) / kTickDelay;

    if (caveMood < 1.0f) {
        if (caveMood < 0.0f) caveMood = 0.0f;
        return;
    }
    caveMood = 0.0f;

    float dx = (bx + 0.5f) - player->x;
    float dy = (by + 0.5f) - player->y;
    float dz = (bz + 0.5f) - player->z;
    float d  = sqrtf(dx * dx + dy * dy + dz * dz);
    if (d < 0.0001f) return;
    float off = d + kPosOffset;
    playSound(player->x + dx / d * off, player->y + dy / d * off, player->z + dz / d * off,
              "ambient.cave", 1.0f, 1.0f);
}

void Level::playSound(Entity* e, const char* name, float volume, float pitch) const {
    if (!e) return;
    playSound(e->x, e->y - e->heightOffset, e->z, name, volume, pitch);
}

void Level::playStepSound(Entity* e, int x, int y, int z, int tileId) const {
    int soundTile = tileId & 0xFF;
    if (getTile(x, y + 1, z) == BLOCK_TOPSNOW) {
        soundTile = BLOCK_TOPSNOW;
    } else if (isLiquidId((unsigned char)soundTile)) {
        return;
    }
    const SoundType& s = g_tileSounds[Tile::tiles[soundTile]->soundType];
    if (!s.stepSound) return;
    playSound(e, s.stepSound, s.volume * 0.25f, s.pitch);
}

void Level::playLandSound(Entity* e, int , int , int , int tileId) const {
    const SoundType& s = g_tileSounds[Tile::tiles[tileId & 0xFF]->soundType];
    if (!s.stepSound) return;
    playSound(e, s.stepSound, s.volume * 0.5f, s.pitch * 0.75f);
}
