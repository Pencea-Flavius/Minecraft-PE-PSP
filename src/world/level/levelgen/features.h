#ifndef FEATURES_H__
#define FEATURES_H__

struct World;
class Random;

void setBlock(World* w, int x, int y, int z, unsigned char id, unsigned char data = 0);
bool isSolidGen(unsigned char id);
int heightmapAt(World* w, int x, int z);

void clayFeature(World* w, Random& random, int x, int y, int z);
void treeOak(World* w, Random& random, int x, int y, int z);
void treeBirch(World* w, Random& random, int x, int y, int z);
void treeSpruce(World* w, Random& random, int x, int y, int z);
void treePine(World* w, Random& random, int x, int y, int z);
void flowerFeature(World* w, Random& random, int x, int y, int z, unsigned char tile);
void mushroomFeature(World* w, Random& random, int x, int y, int z, unsigned char tile);
void cactusFeature(World* w, Random& random, int x, int y, int z);
void reedsFeature(World* w, Random& random, int x, int y, int z);
void oreFeature(World* w, Random& random, int x, int y, int z, unsigned char tile, int count);
void springFeature(World* w, int x, int y, int z, unsigned char tile);
void lakeFeature(World* w, Random& random, int x, int y, int z, unsigned char tile);
void snowCap(World* w, int chunkX, int chunkZ, float* mTemp);

#endif
