#ifndef BIOME_H__
#define BIOME_H__

enum BiomeId { B_TUNDRA, B_SAVANNA, B_DESERT, B_SWAMP, B_TAIGA, B_SHRUB,
               B_FOREST, B_PLAINS, B_SEASONAL, B_RAIN };

BiomeId classifyBiome(float temperature, float downfall);
void biomeSurface(BiomeId b, unsigned char* top, unsigned char* material);

#endif
