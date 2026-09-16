#include "world/level/levelgen/level_source.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/levelgen/mcpegen.h"
#include "world/level/levelgen/gen_features.h"
#include "world/level/storage/level_storage.h"

#include <cstring>

void LevelSource::edgeColumn(unsigned char* col, int* skyFromY) const {
    const int sea = 64;
    for (int y = 0; y < WORLD_H; y++)
        col[y] = (y <= sea - 10) ? BLOCK_STONE : (y < sea) ? BLOCK_CALM_WATER : BLOCK_AIR;
    *skyFromY = sea - 1;
}

namespace {

class RandomLevelSource : public LevelSource {
public:
    void buildTerrain(World* w, long seed) {

        worldGenerateMCPE(w, seed, LevelStorage::getActiveGenMask());
        worldSettleLiquids(w);
    }
    void buildChunk(World* w, int cx, int cz) { chunkGenerateTerrain(w, cx, cz); }
    const char* label() const { return "Old"; }
};

class FlatLevelSource : public LevelSource {
public:

    void buildTerrain(World* w, long ) { worldGenerateWindow(w); }

    void buildChunk(World* w, int cx, int cz) {

        unsigned char col[WORLD_H];
        int sky;
        edgeColumn(col, &sky);

        for (int gz = cz * CHUNK_SZ; gz < cz * CHUNK_SZ + CHUNK_SZ; gz++)
            for (int gx = cx * CHUNK_SX; gx < cx * CHUNK_SX + CHUNK_SX; gx++)
                blockColumnPut(w, gx, gz, col);
    }

    void edgeColumn(unsigned char* col, int* skyFromY) const {
        std::memset(col, BLOCK_AIR, WORLD_H);
        col[0] = BLOCK_BEDROCK;
        col[1] = BLOCK_DIRT;
        col[2] = BLOCK_DIRT;
        col[3] = BLOCK_GRASS;
        *skyFromY = 3;
    }

    bool spawnsMobs() const { return false; }

    bool supportsGenFeatures() const { return false; }

    bool worldTypeHasBedrockFog() const { return false; }
    float clearColorScale() const { return 1.0f; }

    float horizonHeight() const { return 0.0f; }

    int forcedGameType() const { return 1; }
    const char* label() const { return "Flat"; }
};

class SkyLevelSource : public LevelSource {
public:

    void buildTerrain(World* w, long seed) {
        worldGenerateMCPE(w, seed, LevelStorage::getActiveGenMask());
        worldGuaranteeSkyLiquids(w, seed);
        worldSettleLiquids(w);
    }
    void buildChunk(World* w, int cx, int cz) { chunkGenerateTerrain(w, cx, cz); }
    bool floatingIslands() const { return true; }

    bool genFeatureAllowed(int feature) const { return feature != GEN_FEATURE_CAVES; }

    bool worldTypeHasBedrockFog() const { return false; }
    float clearColorScale() const { return 1.0f; }
    float horizonHeight() const { return 0.0f; }

    void edgeColumn(unsigned char* col, int* skyFromY) const {
        std::memset(col, BLOCK_AIR, WORLD_H);
        *skyFromY = 0;
    }

    float cloudHeight() const { return -16.0f; }
    const char* label() const { return "Sky"; }
};

RandomLevelSource s_random;
FlatLevelSource   s_flat;
SkyLevelSource    s_sky;

}

LevelSource& levelSourceFor(int worldType) {

    if (worldType == WORLD_TYPE_FLAT) return s_flat;
    if (worldType == WORLD_TYPE_SKY)  return s_sky;
    return s_random;
}

void activeBorderColumn(unsigned char* col, int* skyFromY) {

    switch (LevelStorage::getActiveBorder()) {
        case LevelStorage::WORLD_BORDER_OCEAN:     levelSourceFor(WORLD_TYPE_OLD).edgeColumn(col, skyFromY);  return;
        case LevelStorage::WORLD_BORDER_SUPERFLAT: levelSourceFor(WORLD_TYPE_FLAT).edgeColumn(col, skyFromY); return;
        case LevelStorage::WORLD_BORDER_VOID:      levelSourceFor(WORLD_TYPE_SKY).edgeColumn(col, skyFromY);  return;
    }
    activeLevelSource().edgeColumn(col, skyFromY);
}

LevelSource& activeLevelSource() {
    return levelSourceFor(LevelStorage::getActiveWorldType());
}
