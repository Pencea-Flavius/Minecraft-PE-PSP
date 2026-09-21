
#ifndef MCPSP_WORLD_LEVEL_LEVEL_H
#define MCPSP_WORLD_LEVEL_LEVEL_H

#include <vector>
#include "world/phys/aabb.h"
#include "world/level/world.h"

struct World;
class Entity;
class TileEntity;
class LocalPlayer;
class Path;

typedef std::vector<Entity*> EntityList;

class Level {
public:
    World* w;
    EntityList entities;
    std::vector<TileEntity*> tileEntities;
    std::vector<AABB> boxes;

    Entity* chunkEntityHead[WORLD_CHUNKS_X * WORLD_CHUNKS_Z];

    LocalPlayer* player;
    bool isClientSide;

    int spawnX, spawnY, spawnZ;
    void setSpawnPos(int x, int y, int z) { spawnX = x; spawnY = y; spawnZ = z; }

    void validateSpawn();

    explicit Level(World* world);

    int   getTile(int x, int y, int z) const;
    int   getData(int x, int y, int z) const;

    void  setTile(int x, int y, int z, int id);
    void  setData(int x, int y, int z, int data);

    void  setNightMode(bool night);
    int   getRawBrightness(int x, int y, int z) const;
    float getBrightness(int x, int y, int z) const;
    bool  hasChunksAt(int x0, int y0, int z0, int x1, int y1, int z1) const;

    bool  isLoadedAt(float x, float z) const;
    bool  isSolidBlockingTile(int x, int y, int z) const;
    bool  isSolidTile(int x, int y, int z) const;

    std::vector<AABB>& getCubes(Entity* except, const AABB& box);

    void getEntities(Entity* except, const AABB& box, EntityList& out) const;

    int getEntitiesOfType(int entityType, const AABB& box, EntityList& out) const;
    int getEntitiesOfClass(int baseType, const AABB& box, EntityList& out) const;

    LocalPlayer* getNearestPlayer(Entity* from, float maxDist) const;

    bool isDay() const;

    void linkEntity(Entity* e);
    void unlinkEntity(Entity* e);
    void relinkIfMoved(Entity* e);

    bool isUnobstructed(const AABB& box) const;

    bool findPath(Path* path, Entity* from, Entity* to, float maxDist, bool openDoors, bool avoidWater);
    bool findPath(Path* path, Entity* from, int x, int y, int z, float maxDist, bool openDoors, bool avoidWater);

    void addEntity(Entity* e);
    void tickEntities();
    int  countInstanceOfBaseType(int baseType) const;

    int  countInstanceOfType(int entityType) const;

    enum SpawnAttempt { SPAWN_EGG, SPAWN_BREED };
    bool canCreateMore(int mobType, SpawnAttempt how, int* why = 0) const;

    Entity* getNearestPlayer(float x, float y, float z, float maxDist) const;
    Entity* getEntity(int id) const;
    int  getDifficulty() const;
    int  getTopSolidBlock(int x, int z) const;
    void removeAllEntities();

    struct TileCrack { int id, x, y, z, progress; };
    enum { MAX_TILE_CRACKS = 8 };
    TileCrack tileCracks[MAX_TILE_CRACKS];
    void destroyTileProgress(int id, int x, int y, int z, int progress);

    TileEntity* getTileEntity(int x, int y, int z);
    void setTileEntity(int x, int y, int z, TileEntity* te);
    void tickTileEntities();
    void removeTileEntity(int x, int y, int z);

    TileEntity* findTileEntity(int x, int y, int z);
    void removeAllTileEntities();

    bool containsAnyLiquid(const AABB& box) const;
    bool containsFireTile(const AABB& box) const;

    bool isInWater(Entity*, const AABB& box) const;
    bool isInLava(const AABB& box) const;

    void playSound(Entity* e, const char* name, float volume, float pitch) const;
    void playSound(float x, float y, float z, const char* name, float volume, float pitch) const;

    void tickCaveMood();
    float caveMood;

    void playStepSound(Entity*, int x, int y, int z, int tileId) const;

    void playLandSound(Entity*, int x, int y, int z, int tileId) const;

    void handleFallOn(int x, int y, int z, Entity* e, float dist) const;
};

extern Level g_level;

#endif
