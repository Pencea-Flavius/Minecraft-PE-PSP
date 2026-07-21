#include "world/level/levelgen/biome.h"
#include "world/level/levelgen/mcpegen_internal.h"
#include "world/level/world.h"

BiomeId classifyBiome(float temperature, float downfall) {
    float t = (int)(temperature * 63) / 63.0f;
    float d = (int)(downfall * 63) / 63.0f;
    d *= t;
    if (t < 0.10f) return B_TUNDRA;
    if (d < 0.20f) {
        if (t < 0.50f) return B_TUNDRA;
        else if (t < 0.95f) return B_SAVANNA;
        else return B_DESERT;
    }
    if (d > 0.5f && t < 0.7f) return B_SWAMP;
    if (t < 0.50f) return B_TAIGA;
    if (t < 0.97f) return (d < 0.35f) ? B_SHRUB : B_FOREST;
    if (d < 0.45f) return B_PLAINS;
    else if (d < 0.90f) return B_SEASONAL;
    else return B_RAIN;
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
