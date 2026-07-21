
#ifndef MCPSP_WORLD_WORLDLIST_H
#define MCPSP_WORLD_WORLDLIST_H

#define MCPSP_MAX_WORLDS 32

struct WorldList {
    char names[MCPSP_MAX_WORLDS][64];
    char displayNames[MCPSP_MAX_WORLDS][64];
    char dates[MCPSP_MAX_WORLDS][64];
    int gameModes[MCPSP_MAX_WORLDS];
    long seeds[MCPSP_MAX_WORLDS];
    int count;
};

long worldSeedFromString(const char* str);

void worldListScan(WorldList* out);

void worldListTouch(const char* absDir);

bool worldListCreate(WorldList* list, const char* inName, char* outName, int gamemode, long seed);

bool worldListDelete(WorldList* list, int index);

#endif
