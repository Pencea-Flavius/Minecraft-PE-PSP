
#ifndef MCPSP_WORLD_STORAGE_LEVEL_STORAGE_H
#define MCPSP_WORLD_STORAGE_LEVEL_STORAGE_H

struct World;

namespace LevelStorage {

bool hasSave(const char* absDir);

bool save(World* w, const char* absDir, long seed, int gameType, const char* levelName,
          bool fullSave = false);

bool load(World* w, const char* absDir, long* outSeed, int* outGameType);

void applyLoadedHotbar();

bool readInfo(const char* absDir, char* nameOut, int nameCap, int* outGameType, long* outSeed);

}

#endif
