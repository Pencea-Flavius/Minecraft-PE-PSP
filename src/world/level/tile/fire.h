
#pragma once

struct World;
class Entity;

static const int FIRE_TICK_DELAY = 30;

void fireInitFlammables();
bool fireCanBurn(const World* w, int x, int y, int z);
void firePlace(World* w, int x, int y, int z);
void fireTileTick(World* w, int x, int y, int z);
bool fireMayPlace(World* w, int x, int y, int z);
void fireNeighborChanged(World* w, int x, int y, int z);

bool fireExtinguishAt(World* w, int x, int y, int z, int face);

void fireExtinguishFx(int x, int y, int z);
