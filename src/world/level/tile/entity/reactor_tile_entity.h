
#ifndef MCPSP_WORLD_LEVEL_TILE_ENTITY_REACTOR_TILE_ENTITY_H
#define MCPSP_WORLD_LEVEL_TILE_ENTITY_REACTOR_TILE_ENTITY_H

#include "world/level/tile/entity/tile_entity.h"
#include "world/item/item_instance.h"
#include "world/level/pathfinder/vec3.h"

class CompoundTag;
class Level;
class LocalPlayer;

class ReactorTileEntity : public TileEntity {
    typedef TileEntity super;
public:
    static const int NUM_PIG_ZOMBIE_SLOTS = 3;

    ReactorTileEntity();

    virtual void tick();
    virtual bool save(CompoundTag* tag);
    virtual void load(CompoundTag* tag);

    void lightItUp(int x, int y, int z);

private:
    void finishReactorRun();

    int  getNumEnemiesPerLevel(int curLevel);
    int  getNumItemsPerLevel(int curLevel);
    void spawnItems(int numItems);
    void spawnItem();
    void spawnEnemy();
    Vec3 getSpawnPosition(float minDistance, float variableDistance, float offset);
    ItemInstance getSpawnItem();
    ItemInstance GetLowOddsSpawnItem();
    bool checkLevelChange(int progress);
    int  numOfFreeEnemySlots();
    void trySpawnPigZombies(int maxNumOfEnemies, int maxToSpawn);
    bool playersAreCloseBy();
    void killPigZombies();

    void tickGlowingRedstoneTransformation(int currentTime);
    void turnLayerToGlowingObsidian(int layer, int type);
    void turnGlowingObsidianLayerToObsidian(int layer);

    void buildDome(int x, int y, int z);

    void relightDomeColumns(int x, int y, int z, int radius);

    void settleDomeEdit(int x, int y, int z, int radius);
    void buildHollowedVolume(int x, int y, int z, int expandWidth, int height, int wallTileId, int clearTileId);
    void buildFloorVolume(int x, int y, int z, int expandWidth, int height, int tileId);
    void buildCrockedRoofVolume(bool inverted, int x, int y, int z, int expandWidth, int height, int tileId);
    static bool isEdge(int curX, int expandWidth, int curZ);
    void deterioateDome(int x, int y, int z);
    void deterioateCrockedRoofVolume(bool inverted, int x, int y, int z, int expandWidth, int height, int tileId);
    void deterioateHollowedVolume(int x, int y, int z, int expandWidth, int height, int tileId);

    bool  isInitialized;
    short progress;
    int   curLevel;
    bool  hasFinished;
};

namespace NetherReactor {

    bool use(Level* level, int x, int y, int z, LocalPlayer* player);

    void setPhase(Level* level, int x, int y, int z, int phase);
}

#endif
