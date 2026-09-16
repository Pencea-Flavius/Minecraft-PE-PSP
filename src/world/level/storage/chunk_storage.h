
#ifndef MCPSP_WORLD_STORAGE_CHUNK_STORAGE_H
#define MCPSP_WORLD_STORAGE_CHUNK_STORAGE_H

struct World;

void chunkStorageInit(const char* absDir);
void chunkStorageShutdown();

void chunkStorageDropOpenFiles();

bool chunkStorageHasSave(const char* absDir);

bool chunkStorageLoad(World* w, int cx, int cz, bool* outGotLight, bool* outPopulated = 0);
extern unsigned int g_chunkCrcFails;

bool chunkStorageSave(World* w, int cx, int cz);

#endif
