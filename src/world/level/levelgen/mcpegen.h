#ifndef MCPEGEN_H__
#define MCPEGEN_H__

struct World;

void worldGenerateMCPE(World* w, long seed, int genMask);

void worldGenerateWindow(World* w);

void worldGuaranteeSkyLiquids(World* w, long seed);

void worldGenInit(long seed, int genMask);
void worldGenFree();

void chunkGenerateTerrain(World* w, int cx, int cz);

bool chunkPostProcessPhase(World* w, int cx, int cz, int phase);

void worldPlaceMushrooms(World* w);

void worldPlaceFlowers(World* w);

#endif
