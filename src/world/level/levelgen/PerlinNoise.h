#ifndef PERLIN_NOISE_H__
#define PERLIN_NOISE_H__

#include "world/level/levelgen/Random.h"
#include "world/level/levelgen/Synth.h"

class ImprovedNoise;

class PerlinNoise: public Synth
{
public:
    PerlinNoise(int levels);
    PerlinNoise(Random* random, int levels);
	~PerlinNoise();

    float getValue(float x, float y);
    float getValue(float x, float y, float z);

    float* getRegion(float* buffer, float x, float y, float z, int xSize, int ySize, int zSize, float xScale, float yScale, float zScale);
    float* getRegion(float* sr, int x, int z, int xSize, int zSize, float xScale, float zScale, float pow);

    int hashCode();

private:
    ImprovedNoise** noiseLevels;
    int levels;

    Random _random;
    Random* _rndPtr;

    void init(int levels);
};

#endif
