
#ifndef MCPSP_WORLD_WORLD_H
#define MCPSP_WORLD_WORLD_H

#include "world/level/chunk/chunk.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

struct TickNextTickData {
    int x, y, z;
    unsigned char tileId;
    long delay;
    bool operator==(const TickNextTickData& t) const {
        return x == t.x && y == t.y && z == t.z && tileId == t.tileId;
    }
};

#define WORLD_CHUNKS_X 16
#define WORLD_CHUNKS_Z 16
#define WORLD_W (WORLD_CHUNKS_X * CHUNK_SX)
#define WORLD_H CHUNK_SY
#define WORLD_D (WORLD_CHUNKS_Z * CHUNK_SZ)

#define WORLD_VIEW_DIST 64.0f

struct LightUpdate {
    int layer;
    int x0, y0, z0, x1, y1, z1;
};

struct World {
    unsigned char* blocks;

    unsigned char* data;
    unsigned char* light;
    unsigned char* heightmap;
    ChunkMesh chunks[WORLD_CHUNKS_X * WORLD_CHUNKS_Z];

    bool unsaved[WORLD_CHUNKS_X * WORLD_CHUNKS_Z];

    long time;

    long dayTime;
    std::vector<TickNextTickData> tickNextTickList;

    std::unordered_set<unsigned int> tickSet;
    std::vector<LightUpdate> lightQueue;
    bool lightReady;

    std::vector<std::vector<unsigned char> > preservedTileEntities;
};

extern volatile int g_terrainProgress;
extern volatile bool g_terrainThreadDone;

bool worldInitTerrain(World* w, long seed);

bool worldAllocArrays(World* w);

int worldBuildMeshesStep(World* w, int maxChunks);

void worldFindSpawn(World* w, int* outX, int* outZ, int* outFeetY);

void worldFree(World* w);

static inline int worldIndex(int x, int y, int z) {
    return (x * WORLD_D + z) * WORLD_H + y;
}
static inline unsigned char worldBlock(const World* w, int x, int y, int z) {
    if (y < 0 || y >= WORLD_H) return BLOCK_AIR;
    if (x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D)
        return BLOCK_INVISIBLE_BEDROCK;
    return w->blocks[worldIndex(x, y, z)];
}
static inline unsigned char worldData(const World* w, int x, int y, int z) {
    if (y < 0 || y >= WORLD_H || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return 0;
    int i = worldIndex(x, y, z);
    return (i & 1) ? (unsigned char)(w->data[i >> 1] >> 4)
                   : (unsigned char)(w->data[i >> 1] & 0x0F);
}

static inline void worldDataPut(World* w, int i, unsigned char v) {
    unsigned char& b = w->data[i >> 1];
    b = (i & 1) ? (unsigned char)((b & 0x0F) | (unsigned char)((v & 0x0F) << 4))
                : (unsigned char)((b & 0xF0) | (v & 0x0F));
}
void worldSetBlockAndData(World* w, int x, int y, int z, unsigned char id, unsigned char data);
void worldSetData(World* w, int x, int y, int z, unsigned char data);

void worldSetDataNoUpdate(World* w, int x, int y, int z, unsigned char data);

void worldMarkDirty(World* w, int x, int y, int z);

void worldDrainPlayerEdits(World* w, int maxSections);

void worldRebuildAroundNow(World* w, int x, int y, int z);

extern unsigned int g_editWorstUs, g_drainWorstUs;
void worldRecordEditUs(unsigned int us);

extern unsigned int g_lightWorstUs, g_rebuildWorstUs;
void worldRecordLightUs(unsigned int us);
void worldRecordRebuildUs(unsigned int us);

void lightOnBlockChanged(World* w, int x, int y, int z);

static inline int lightSkyGet(const World* w, int x, int y, int z) {
    if (y >= WORLD_H) return 15;
    if (y < 0 || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return 0;
    return w->light[worldIndex(x, y, z)] >> 4;
}
static inline int lightBlockGet(const World* w, int x, int y, int z) {
    if (y < 0 || y >= WORLD_H || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return 0;
    return w->light[worldIndex(x, y, z)] & 0x0F;
}
static inline void lightSkySet(World* w, int x, int y, int z, int v) {
    if (y < 0 || y >= WORLD_H || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return;
    unsigned char* p = &w->light[worldIndex(x, y, z)];
    *p = (unsigned char)((*p & 0x0F) | (v << 4));
}
static inline void lightBlockSet(World* w, int x, int y, int z, int v) {
    if (y < 0 || y >= WORLD_H || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return;
    unsigned char* p = &w->light[worldIndex(x, y, z)];
    *p = (unsigned char)((*p & 0xF0) | v);
}

#define TICKS_PER_DAY 19200

extern int g_skyDarken;

float worldTimeOfDay(long dayTime, float a);

void worldSetNightMode(World* w, bool night);

void worldUpdateSkyDarken(World* w);

static inline bool worldIsDay() { return g_skyDarken < 4; }

static inline int lightRawAtNoProp(const World* w, int x, int y, int z) {

    if (y >= WORLD_H) return 15;
    if (y < 0 || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return 0;
    unsigned char v = w->light[worldIndex(x, y, z)];
    int s = (v >> 4) - g_skyDarken, b = v & 0x0F;
    if (s < 0) s = 0;
    return s > b ? s : b;
}

static inline int lightRawAt(const World* w, int x, int y, int z) {
    unsigned char id = worldBlock(w, x, y, z);
    if (id == BLOCK_SLAB || id == BLOCK_FARMLAND) {
        int br = lightRawAtNoProp(w, x, y + 1, z);
        int b1 = lightRawAtNoProp(w, x + 1, y, z); if (b1 > br) br = b1;
        int b2 = lightRawAtNoProp(w, x - 1, y, z); if (b2 > br) br = b2;
        int b3 = lightRawAtNoProp(w, x, y, z + 1); if (b3 > br) br = b3;
        int b4 = lightRawAtNoProp(w, x, y, z - 1); if (b4 > br) br = b4;
        return br;
    }
    return lightRawAtNoProp(w, x, y, z);
}

static inline bool worldCanSeeSky(const World* w, int x, int y, int z) {
    if (y >= WORLD_H) return true;
    if (y < 0 || x < 0 || x >= WORLD_W || z < 0 || z >= WORLD_D) return false;
    return y >= w->heightmap[x * WORLD_D + z];
}

void worldInitLight(World* w);
void worldRecalcHeightmap(World* w);
void worldUpdateLights(World* w);
void worldRemoveBlockLight(World* w, int x, int y, int z);

void worldScheduleTick(World* w, int x, int y, int z, unsigned char id, int tickDelay);
void worldTick(World* w);
void worldSettleLiquids(World* w);
void worldScheduleLoadedLiquids(World* w);
void worldUpdateNeighbors(World* w, int x, int y, int z, unsigned char id);

void liquidFlow(const World* w, int x, int y, int z, unsigned char id,
                float* fx, float* fy, float* fz);

void worldNotifyNeighborsChanged(World* w, int x, int y, int z);

void worldExplode(World* w, float x, float y, float z, float r);

void worldPrimeTnt(World* w, int x, int y, int z, int fuseTicks);

bool tileMayPlace(World* w, unsigned char id, int x, int y, int z, int data = -1);
void tileNeighborChanged(World* w, int x, int y, int z);
void tileRandomTick(World* w);

void heavyTileTick(World* w, int x, int y, int z, unsigned char id);

void farmlandCheckDry(World* w, int x, int y, int z);

void leafFlagNeighbors(World* w, int x, int y, int z);
void leafDecayTick(World* w, int x, int y, int z);

void worldSpawnResources(World* w, int x, int y, int z, unsigned char id, int data);

void worldSetFrustumCamera(float ex, float ey, float ez, float fx, float fy, float fz,
                           float yawDeg, float fovyDeg, float aspect, float nearD, float farD);

struct Texture;
void worldDraw(const World* w, float camX, float camY, float camZ, float viewDist, const Texture* terrain);

void worldRebuildStep(const World* w, float camX, float camY, float camZ, float viewDist);
void worldDrawWater(const World* w, float camX, float camY, float camZ, float viewDist);

struct BlockHit { bool hit; int x, y, z; int face; float clickX, clickY, clickZ; };
BlockHit worldPick(const World* w, float px, float py, float pz, float yaw, float pitch, float range);

int worldSelectionBoxes(const World* w, int x, int y, int z, float boxes[3][6]);

#endif
