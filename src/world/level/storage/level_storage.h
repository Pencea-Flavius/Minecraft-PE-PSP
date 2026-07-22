
#ifndef MCPSP_WORLD_STORAGE_LEVEL_STORAGE_H
#define MCPSP_WORLD_STORAGE_LEVEL_STORAGE_H

struct World;

namespace LevelStorage {

bool hasSave(const char* absDir);

bool save(World* w, const char* absDir, long seed, int gameType, const char* levelName,
          bool fullSave = false);

bool load(World* w, const char* absDir, long* outSeed, int* outGameType);

void applyLoadedHotbar();

bool loadedValidPlayerPos();

bool readInfo(const char* absDir, char* nameOut, int nameCap, int* outGameType, long* outSeed);

void setActiveWorld(const char* absDir, long seed, int gameType, const char* levelName);
const char* getActiveDir();
long getActiveSeed();
int getActiveGameType();
const char* getActiveName();

}

#endif
