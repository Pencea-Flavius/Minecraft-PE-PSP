
#ifndef MCPSP_WORLD_LEVEL_LEVEL_H
#define MCPSP_WORLD_LEVEL_LEVEL_H

#include <vector>
#include "world/phys/aabb.h"

struct World;
class Entity;
class TileEntity;
class LocalPlayer;
class Path;

class Level {
public:
    World* w;
    Level();
    std::vector<Entity*> entities;
    std::vector<TileEntity*> tileEntities;

    LocalPlayer* player;
    bool isClientSide;

    explicit Level(World* world) : w(world), player(0), isClientSide(false) {}

    int   getTile(int x, int y, int z) const;
    int   getData(int x, int y, int z) const;

    void  setTile(int x, int y, int z, int id);
    void  setData(int x, int y, int z, int data);

    void  setNightMode(bool night);
    int   getRawBrightness(int x, int y, int z) const;
    float getBrightness(int x, int y, int z) const;
    bool  hasChunksAt(int x0, int y0, int z0, int x1, int y1, int z1) const;
    bool  isSolidBlockingTile(int x, int y, int z) const;
    bool  isSolidTile(int x, int y, int z) const;

    std::vector<AABB> getCubes(Entity* except, const AABB& box) const;

    std::vector<Entity*> getEntities(Entity* except, const AABB& box) const;

    bool isUnobstructed(const AABB& box) const;

    void findPath(Path* path, Entity* from, Entity* to, float maxDist, bool openDoors, bool avoidWater);
    void findPath(Path* path, Entity* from, int x, int y, int z, float maxDist, bool openDoors, bool avoidWater);

    void addEntity(Entity* e);
    void tickEntities();
    int  countInstanceOfBaseType(int baseType) const;
    Entity* getEntity(int id) const;
    int  getDifficulty() const;
    int  getTopSolidBlock(int x, int z) const;
    void removeAllEntities();

    TileEntity* getTileEntity(int x, int y, int z);
    void setTileEntity(int x, int y, int z, TileEntity* te);
    void tickTileEntities();
    void removeTileEntity(int x, int y, int z);
    void removeAllTileEntities();

    bool checkAndHandleWater(const AABB&, class Entity*) const { return false; }
    bool containsAnyLiquid(const AABB&) const { return false; }
    bool containsFireTile(const AABB&) const { return false; }

    bool isInWater(Entity*) const;
    bool isInLava(Entity*) const;
    void addParticle(int, float,float,float, float,float,float) const {}

    void playSound(Entity* e, const char* name, float volume, float pitch) const;
    void playSound(float x, float y, float z, const char* name, float volume, float pitch) const;

    void playStepSound(Entity*, int x, int y, int z, int tileId) const;

    void playLandSound(Entity*, int x, int y, int z, int tileId) const;

    void handleFallOn(int x, int y, int z, Entity*, float dist) const {}
};

extern Level g_level;

#endif
