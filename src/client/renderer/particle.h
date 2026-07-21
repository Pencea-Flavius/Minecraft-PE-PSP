
#pragma once

struct World;
struct Texture;

void particlesReset();

void particlesTick(World* w, float px, float py, float pz);

void particlesRender(World* w, float yawDeg, float pitchDeg, float alpha,
                     const Texture* terrain, const Texture* misc, const Texture* items);

void particlesDestroyBlock(World* w, int x, int y, int z, unsigned char id, unsigned char data);

void particlesCrackHit(World* w, int x, int y, int z, unsigned char id, unsigned char data, int face);

void particlesEat(float px, float py, float pz, float yawDeg, float pitchDeg, int iconCell, int count);

void particlesThrowPoof(float x, float y, float z, int iconCell);
void particlesExplosion(World* w, float x, float y, float z);

void particlesHeart(float x, float y, float z, float xa, float ya, float za);

void particlesHeartBurst(float x, float feetY, float z, float w, float h);

void particlesMobDeath(float x, float y, float z, float w, float h);

void particlesRedstonePoof(World* w, int x, int y, int z);

void particlesBubble(float x, float y, float z, float xa, float ya, float za);

void particlesSplash(float x, float y, float z, float xa, float ya, float za);

void particlesCrit(float x, float y, float z, float xa, float ya, float za);

void particlesSmoke(float x, float y, float z);

void particlesLargeSmoke(float x, float y, float z);

void particlesFurnaceFire(int x, int y, int z, unsigned char dir);
