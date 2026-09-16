
#ifndef MCPSP_WORLD_STORAGE_LEVEL_STORAGE_H
#define MCPSP_WORLD_STORAGE_LEVEL_STORAGE_H

#include "world/level/levelgen/level_source.h"
#include "world/level/levelgen/gen_features.h"

struct World;

namespace LevelStorage {

bool hasSave(const char* absDir);

bool save(World* w, const char* absDir, long seed, int gameType, const char* levelName,
          bool fullSave = false);

bool load(World* w, const char* absDir, long* outSeed, int* outGameType);

void applyLoadedHotbar();

bool loadedValidPlayerPos();

bool readInfo(const char* absDir, char* nameOut, int nameCap, int* outGameType, long* outSeed);

void setActiveWorld(const char* absDir, long seed, int gameType, const char* levelName,
                    int worldType = WORLD_TYPE_OLD, int genMask = GEN_FEATURES_ALL_ON);
const char* getActiveDir();
long getActiveSeed();
int getActiveGameType();
int getActiveWorldType();
int getActiveGenMask();
const char* getActiveName();

enum { WORLD_BORDER_BY_TYPE = -1, WORLD_BORDER_OCEAN = 0, WORLD_BORDER_SUPERFLAT = 1, WORLD_BORDER_VOID = 2 };
int getActiveBorder();

}

#endif
