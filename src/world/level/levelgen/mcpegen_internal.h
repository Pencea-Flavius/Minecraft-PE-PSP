#ifndef MCPEGEN_INTERNAL_H__
#define MCPEGEN_INTERNAL_H__

#include "world/level/levelgen/Random.h"
#include "world/level/levelgen/PerlinNoise.h"

#define BIOME_ZOOM        2.0f
#define BIOME_TEMP_SCALE  (BIOME_ZOOM / 80.0f)
#define BIOME_DOWN_SCALE  (BIOME_ZOOM / 40.0f)
#define BIOME_NOISE_SCALE (1.0f / 4.0f)

struct World;

struct McpeGen {
    Random random;
    Random rndTemp, rndDownfall, rndNoise;

    PerlinNoise lperlinNoise1, lperlinNoise2, perlinNoise1, perlinNoise2, perlinNoise3,
                scaleNoise, depthNoise, forestNoise;
    PerlinNoise temperatureMap, downfallMap, noiseMap;

    Random rndSky;
    PerlinNoise skyBottom0, skyBottom1;
    bool mSky;

    float* buffer;
    float *pnr, *ar, *br, *sr, *dr;
    float *rawTemp, *rawDownfall, *rawNoise;
    float mTemp[16 * 16], mDownfall[16 * 16];
    float sandBuffer[16 * 16], gravelBuffer[16 * 16], depthBuffer[16 * 16];
    long  worldSeed;

    McpeGen(long seed);
    ~McpeGen();

    void computeBiome(int chunkX, int chunkZ);
    float* getHeights(int x, int y, int z, int xSize, int ySize, int zSize);
    void prepareChunk(World* w, int chunkX, int chunkZ);
    void prepareSkyChunk(World* w, int chunkX, int chunkZ);
    void buildSurfacesChunk(World* w, int chunkX, int chunkZ);

    bool postProcessPhase(World* w, int chunkX, int chunkZ, int phase);
    int mPhaseBiome;
};

#endif
