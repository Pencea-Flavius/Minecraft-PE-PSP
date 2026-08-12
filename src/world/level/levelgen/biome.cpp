#include "world/level/levelgen/biome.h"
#include "world/level/levelgen/mcpegen_internal.h"
#include "world/level/levelgen/Random.h"
#include "world/level/levelgen/PerlinNoise.h"
#include "world/level/world.h"

// Biome placement: each biome gets exactly one seed point, placed once per
// world (deterministic from the world seed) on a jittered grid so seeds are
// well spread out without needing rejection sampling. A column's biome is
// simply whichever seed is nearest, giving compact, non-repeating regions
// instead of the old noise-based classifier's scattered patches. Distance is
// perturbed by a low-frequency Perlin field so region borders wiggle
// naturally instead of forming razor-straight Voronoi edges.

static const int N_BIOMES = 11;
static const BiomeId kBiomeOrder[N_BIOMES] = {
    B_TUNDRA, B_SAVANNA, B_DESERT, B_SWAMP, B_TAIGA, B_SHRUB,
    B_FOREST, B_PLAINS, B_SEASONAL, B_RAIN, B_JUNGLE
};

static bool  s_seedsReady = false;
static long  s_seedsForWorldSeed = 0;
static float s_seedX[N_BIOMES];
static float s_seedZ[N_BIOMES];
static PerlinNoise* s_borderNoise = 0;

static void ensureBiomeSeeds(long worldSeed) {
    if (s_seedsReady && s_seedsForWorldSeed == worldSeed) return;

    Random seedRandom(worldSeed ^ 0x610E5EEDL);

    const int cols = 4, rows = 3; // 12 grid cells, 11 used
    float worldSpanX = (float)(WORLD_SIZE_CHUNKS * CHUNK_SX);
    float worldSpanZ = (float)(WORLD_SIZE_CHUNKS * CHUNK_SZ);
    float cellW = worldSpanX / cols;
    float cellD = worldSpanZ / rows;

    for (int i = 0; i < N_BIOMES; i++) {
        int gx = i % cols, gz = i / cols;
        float centerX = gx * cellW + cellW * 0.5f;
        float centerZ = gz * cellD + cellD * 0.5f;
        // Jitter within the cell, keeping a margin so seeds don't land right
        // on a cell edge (which would crowd two regions' borders together).
        float jitterX = (seedRandom.nextFloat() - 0.5f) * cellW * 0.5f;
        float jitterZ = (seedRandom.nextFloat() - 0.5f) * cellD * 0.5f;
        s_seedX[i] = centerX + jitterX;
        s_seedZ[i] = centerZ + jitterZ;
    }

    delete s_borderNoise;
    s_borderNoise = new PerlinNoise(&seedRandom, 2);

    s_seedsForWorldSeed = worldSeed;
    s_seedsReady = true;
}

BiomeId classifyBiomeSpatial(long worldSeed, int worldX, int worldZ) {
    ensureBiomeSeeds(worldSeed);

    float bx = (float)worldX, bz = (float)worldZ;
    int best = 0;
    float bestD2 = 1e18f;
    for (int i = 0; i < N_BIOMES; i++) {
        float dx = bx - s_seedX[i], dz = bz - s_seedZ[i];
        float d2 = dx * dx + dz * dz;

        // Perturb each seed's effective distance a little so the boundary
        // between two regions wiggles instead of being a straight line.
        float wobble = s_borderNoise->getValue(worldX * 0.01f, worldZ * 0.01f + i * 37.0f);
        d2 += wobble * 900.0f;

        if (d2 < bestD2) { bestD2 = d2; best = i; }
    }
    return kBiomeOrder[best];
}

void biomeSurface(BiomeId b, unsigned char* top, unsigned char* material) {
    if (b == B_DESERT) { *top = BLOCK_SAND; *material = BLOCK_SAND; }
    else               { *top = BLOCK_GRASS; *material = BLOCK_DIRT; }
}

void McpeGen::computeBiome(int chunkX, int chunkZ) {
    int x = chunkX * 16, z = chunkZ * 16;
    rawTemp     = temperatureMap.getRegion(rawTemp,     x, z, 16, 16, BIOME_TEMP_SCALE,  BIOME_TEMP_SCALE,  0.25f);
    rawDownfall = downfallMap.getRegion(rawDownfall,    x, z, 16, 16, BIOME_DOWN_SCALE,  BIOME_DOWN_SCALE,  0.3333f);
    rawNoise    = noiseMap.getRegion(rawNoise,          x, z, 16, 16, BIOME_NOISE_SCALE, BIOME_NOISE_SCALE, 0.588f);

    for (int pp = 0; pp < 16 * 16; pp++) {
        float noise = (rawNoise[pp] * 1.1f + 0.5f);

        float split2 = 0.01f, split1 = 1 - split2;
        float temperature = (rawTemp[pp] * 0.15f + 0.7f) * split1 + noise * split2;
        split2 = 0.002f; split1 = 1 - split2;
        float downfall = (rawDownfall[pp] * 0.15f + 0.5f) * split1 + noise * split2;

        temperature = 1 - ((1 - temperature) * (1 - temperature));
        if (temperature < 0) temperature = 0;
        if (downfall < 0) downfall = 0;
        if (temperature > 1) temperature = 1;
        if (downfall > 1) downfall = 1;

        mTemp[pp] = temperature;
        mDownfall[pp] = downfall;
    }
}
