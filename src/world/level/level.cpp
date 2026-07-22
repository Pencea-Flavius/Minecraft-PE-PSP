#include "world/level/level.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/entity/entity.h"
#include "world/entity/local_player.h"
#include "world/difficulty.h"
#include "world/level/tile/entity/tile_entity.h"
#include "client/player/physics.h"
#include "world/level/pathfinder/path_finder.h"
#include "world/level/mob_spawner.h"
#include "world/level/tile/tile.h"
#include "platform/audio/sound.h"
#include "util/mth.h"

Level::Level() { entities.reserve(Entity::ENTITY_POOL + 16); }

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

    if (x1 < 0 || z1 < 0 || x0 >= WORLD_W || z0 >= WORLD_D) return false;
    if (y1 < 0 || y0 >= WORLD_H) return false;
    return true;
}
bool Level::isSolidBlockingTile(int x, int y, int z) const {
    return isOpaque((unsigned char)worldBlock(w, x, y, z));
}

bool Level::isSolidTile(int x, int y, int z) const {
    return isSolidMaterial((unsigned char)worldBlock(w, x, y, z));
}

std::vector<AABB> Level::getCubes(Entity* , const AABB& box) const {
    std::vector<AABB> out;
    int x0 = Mth::floor(box.x0), x1 = Mth::floor(box.x1);
    int y0 = Mth::floor(box.y0), y1 = Mth::floor(box.y1);
    int z0 = Mth::floor(box.z0), z1 = Mth::floor(box.z1);
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++)
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

std::vector<Entity*> Level::getEntities(Entity* except, const AABB& box) const {
    std::vector<Entity*> out;
    for (size_t i = 0; i < entities.size(); i++) {
        Entity* e = entities[i];
        if (e == except || e->removed) continue;
        if (e->bb.intersects(box)) out.push_back(e);
    }
    return out;
}

bool Level::isUnobstructed(const AABB& box) const {
    for (size_t i = 0; i < entities.size(); i++) {
        Entity* e = entities[i];
        if (e && !e->removed && e->blocksBuilding && e->bb.intersects(box)) return false;
    }

    if (player && !player->removed && player->bb.intersects(box)) return false;
    return true;
}

static PathFinder s_pathFinder;

void Level::findPath(Path* path, Entity* from, Entity* to, float maxDist, bool , bool avoidWater) {
    s_pathFinder.setLevel(this);
    s_pathFinder.avoidWater = avoidWater;
    s_pathFinder.findPath(path, from, to, maxDist);
}
void Level::findPath(Path* path, Entity* from, int x, int y, int z, float maxDist, bool , bool avoidWater) {
    s_pathFinder.setLevel(this);
    s_pathFinder.avoidWater = avoidWater;
    s_pathFinder.findPath(path, from, x, y, z, maxDist);
}

int Level::countInstanceOfBaseType(int baseType) const {
    int n = 0;
    for (size_t i = 0; i < entities.size(); i++) {
        Entity* e = entities[i];
        if (e && !e->removed && e->isMob() && e->getCreatureBaseType() == baseType) n++;
    }
    return n;
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
    if (x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return WORLD_H;
    return w->heightmap[x * WORLD_D + z];
}

void Level::addEntity(Entity* e) {
    entities.push_back(e);
}

static const int SPAWN_INTERVAL = 20;

void Level::tickEntities() {

    static int s_spawnTimer = 0;
    if (++s_spawnTimer >= SPAWN_INTERVAL) { s_spawnTimer = 0; MobSpawner::tick(this, true, false); }

    if ((w->time % 400) < 2) MobSpawner::tick(this, false, true);

    for (size_t i = 0; i < entities.size(); i++) {
        Entity* e = entities[i];
        if (!e->removed) e->tick();
    }

    for (size_t i = 0; i < entities.size(); ) {
        if (entities[i]->removed) {
            delete entities[i];
            entities[i] = entities.back();
            entities.pop_back();
        } else {
            i++;
        }
    }
}

void Level::removeAllEntities() {
    for (size_t i = 0; i < entities.size(); i++) delete entities[i];
    entities.clear();
}

TileEntity* Level::getTileEntity(int x, int y, int z) {
    for (size_t i = 0; i < tileEntities.size(); i++) {
        TileEntity* te = tileEntities[i];
        if (te && !te->removed && te->x == x && te->y == y && te->z == z) return te;
    }
    return 0;
}

void Level::setTileEntity(int x, int y, int z, TileEntity* te) {
    removeTileEntity(x, y, z);
    te->setLevelAndPos(this, x, y, z);
    tileEntities.push_back(te);
}

void Level::removeTileEntity(int x, int y, int z) {
    for (size_t i = 0; i < tileEntities.size(); ) {
        TileEntity* te = tileEntities[i];
        if (te && te->x == x && te->y == y && te->z == z) {
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
        if (te && !te->removed) te->tick();
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
void furnaceSetLitBlock(Level* level, int x, int y, int z, bool lit) {
    if (!level || !level->w) return;
    unsigned char id = (unsigned char)level->getTile(x, y, z);
    if (id != BLOCK_FURNACE && id != BLOCK_FURNACE_LIT) return;
    unsigned char data = (unsigned char)level->getData(x, y, z);
    worldSetBlockAndData(level->w, x, y, z, lit ? BLOCK_FURNACE_LIT : BLOCK_FURNACE, data);
    worldRebuildAroundNow(level->w, x, y, z);
}

bool Level::isInWater(Entity* e) const {
    const AABB& box = e->bb;
    int x0 = Mth::floor(box.x0), x1 = Mth::floor(box.x1 + 1);
    int y0 = Mth::floor(box.y0), y1 = Mth::floor(box.y1 + 1);
    int z0 = Mth::floor(box.z0), z1 = Mth::floor(box.z1 + 1);
    bool ok = false;
    float cx = 0, cy = 0, cz = 0;
    for (int x = x0; x < x1; x++)
        for (int y = y0; y < y1; y++)
            for (int z = z0; z < z1; z++) {
                unsigned char id = worldBlock(w, x, y, z);
                if (!isWaterId(id)) continue;
                ok = true;
                float fx, fy, fz;
                liquidFlow(w, x, y, z, id, &fx, &fy, &fz);
                cx += fx * 0.5f; cy += fy * 0.5f; cz += fz * 0.5f;
            }
    float len = Mth::sqrt(cx * cx + cy * cy + cz * cz);
    if (len > 0.0f) {
        float p = 0.004f / len;
        e->xd += cx * p; e->yd += cy * p; e->zd += cz * p;
    }
    return ok;
}

bool Level::isInLava(Entity* e) const {
    const AABB& box = e->bb;
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
