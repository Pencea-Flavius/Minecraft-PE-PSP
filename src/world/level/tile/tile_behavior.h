
#ifndef WORLD_LEVEL_TILE_TILE_BEHAVIOR_H
#define WORLD_LEVEL_TILE_TILE_BEHAVIOR_H

#include "world/level/world.h"

struct World;

bool bushMayPlaceOn(World* w, unsigned char id, int x, int y, int z);
bool bushFamilyCanSurvive(World* w, unsigned char id, int x, int y, int z);
void saplingTick(World* w, int x, int y, int z);
void saplingGrow(World* w, int x, int y, int z);
void mushroomTick(World* w, int x, int y, int z);
void cropTick(World* w, int x, int y, int z);
void stemTick(World* w, int x, int y, int z);

bool reedCanSurvive(World* w, int x, int y, int z);
bool cactusCanSurvive(World* w, int x, int y, int z);
void reedCactusGrow(World* w, int x, int y, int z, unsigned char id, int ageThreshold);

void tickFarmland(World* w, int x, int y, int z);

void heavyTileTick(World* w, int x, int y, int z, unsigned char id);

bool supportCanSurvive(World* w, unsigned char id, int x, int y, int z, int data);

#endif
