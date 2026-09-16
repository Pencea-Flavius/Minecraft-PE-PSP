
#ifndef MCPSP_WORLD_WORLD_H
#define MCPSP_WORLD_WORLD_H

#include "world/level/levelgen/level_source.h"

#include "world/level/chunk/chunk.h"
#include "world/level/tile/material.h"
#include <stdlib.h>
#include <string.h>
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

#ifndef WORLD_SIZE_CHUNKS
#define WORLD_SIZE_CHUNKS 16
#endif

static inline bool worldChunkInBounds(int cx, int cz) {
#if WORLD_SIZE_CHUNKS
    return cx >= 0 && cz >= 0 && cx < WORLD_SIZE_CHUNKS && cz < WORLD_SIZE_CHUNKS;
#else
    (void)cx; (void)cz; return true;
#endif
}

#define LP_PAGE       128
#define LP_BLK_PAGES  256
#define LP_MAX_BLKS   256
#define LP_ALL0   0xFFFEu
#define LP_ALL15  0xFFFFu
#define LP_SENT   0xFFFEu

#define BS_SECTIONS   (WORLD_CHUNKS_X * WORLD_CHUNKS_Z * N_SECTIONS)
#define BS_CELLS      (16 * 16 * SECTION_SY)
#define BS_PAL_PAGE   (BS_CELLS / 2)
#define BS_PAL_MAX    16

struct BlockSection {
    unsigned char* page;
    unsigned char  pal[BS_PAL_MAX];
    unsigned char  palN;
    unsigned char  uniform;
};

struct LevelChunk {
    int  x, z;
    bool resident;

    bool unsaved;

    bool terrainPopulated;

    volatile bool generating;

    unsigned char stage;
    bool isAt(int cx, int cz) const { return resident && x == cx && z == cz; }
};

struct World {

    int slotBits;
    int slotMask;
    int slotN;

    BlockSection bsec[BS_SECTIONS];
    int blockPages;
    unsigned int blockPageBytes;

    unsigned char** dataCol;
    int dataPages;

    unsigned short* lightIdx;
    unsigned char*  lightPool[LP_MAX_BLKS];
    int             lightBlocksUsed;
    int             lightPagesUsed;
    int             lightPagesAlloced;
    unsigned short  lightFreeHead;
    unsigned int    lightOomDrops;
    unsigned char* heightmap;
    ChunkMesh chunks[WORLD_CHUNKS_X * WORLD_CHUNKS_Z];

    LevelChunk slots[WORLD_CHUNKS_X * WORLD_CHUNKS_Z];

    long time;

    long dayTime;
    std::vector<TickNextTickData> tickNextTickList;

    std::unordered_set<unsigned int> tickSet;

    std::vector<unsigned int> lightQueue;
    bool lightReady;

    bool simTick;

    std::vector<std::vector<unsigned char> > preservedTileEntities;
};

extern volatile int g_terrainProgress;
extern volatile bool g_terrainThreadDone;

bool worldInitTerrain(World* w, long seed, int worldType = WORLD_TYPE_OLD);

long worldSeed(void);

bool worldAllocArrays(World* w);

int worldBuildMeshesStep(World* w, int maxChunks);

void worldFindSpawn(World* w, int* outX, int* outZ, int* outFeetY);

void worldValidateSpawn(World* w, int* x, int* y, int* z);

void worldFree(World* w);

#define WORLD_SLOT_BITS 4
static inline void worldSetWindow(World* w, int slotBits) {
    w->slotBits = slotBits;
    w->slotN    = 1 << slotBits;
    w->slotMask = w->slotN - 1;
}
static inline int worldSlotIndex(const World* w, int cx, int cz) {
    return ((cz & w->slotMask) << w->slotBits) | (cx & w->slotMask);
}
static inline LevelChunk* worldSlot(World* w, int cx, int cz) {
    return &w->slots[worldSlotIndex(w, cx, cz)];
}
static inline const LevelChunk* worldSlot(const World* w, int cx, int cz) {
    return &w->slots[worldSlotIndex(w, cx, cz)];
}

static inline bool worldChunkReady(const World* w, int cx, int cz) {
    return worldSlot(w, cx, cz)->isAt(cx, cz);
}

static inline bool worldChunkSettled(const World* w, int cx, int cz) {
    const LevelChunk* c = worldSlot(w, cx, cz);
    return c->isAt(cx, cz) && !c->generating;
}

static inline bool worldSlotBusy(const LevelChunk* c) { return c->generating || c->stage != 0; }

static inline bool worldNeighbourSettled(const World* w, int cx, int cz) {

    return !worldChunkInBounds(cx, cz) || worldChunkReady(w, cx, cz);
}
static inline bool worldChunkMeshable(const World* w, int cx, int cz) {
    return worldNeighbourSettled(w, cx + 1, cz) && worldNeighbourSettled(w, cx - 1, cz) &&
           worldNeighbourSettled(w, cx, cz + 1) && worldNeighbourSettled(w, cx, cz - 1);
}
static inline bool worldReady(const World* w, int x, int z) {
    return worldChunkReady(w, x >> 4, z >> 4);
}

static inline bool worldFitsInWindow(const World* w) {
#if WORLD_SIZE_CHUNKS
    return WORLD_SIZE_CHUNKS <= w->slotN;
#else
    (void)w; return false;
#endif
}

static inline void worldSlotsReset(World* w) {
    for (int i = 0; i < w->slotN * w->slotN; i++) {
        LevelChunk* c = &w->slots[i];
        c->x = c->z = 0;
        c->resident = false; c->unsaved = false; c->terrainPopulated = false;
        c->generating = false; c->stage = 0;
    }
}
static inline void worldResidentAtOrigin(World* w) {
    for (int cz = 0; cz < w->slotN; cz++)
        for (int cx = 0; cx < w->slotN; cx++) {
            LevelChunk* c = &w->slots[worldSlotIndex(w, cx, cz)];
            c->x = cx; c->z = cz; c->resident = true; c->unsaved = false;

            c->terrainPopulated = false; c->generating = false; c->stage = 0;
        }
}

static inline void worldWindowOrigin(const World* w, int* ox, int* oz) {
    *ox = w->slots[0].x * 16;
    *oz = w->slots[0].z * 16;
}

static inline int worldColumn(const World* w, int x, int z) {
    return (worldSlotIndex(w, x >> 4, z >> 4) << 8) | ((x & 15) << 4) | (z & 15);
}
static inline int worldIndex(const World* w, int x, int y, int z) {
    return worldColumn(w, x, z) * WORLD_H + y;
}

static inline int bsSection(const World* w, int x, int y, int z) {
    return (worldSlotIndex(w, x >> 4, z >> 4) * N_SECTIONS) + (y >> 4);
}
static inline int bsOffset(int x, int y, int z) {
    return (((x & 15) * 16 + (z & 15)) * SECTION_SY) + (y & (SECTION_SY - 1));
}

static inline ChunkMesh* worldMesh(World* w, int cx, int cz) {
    return &w->chunks[worldSlotIndex(w, cx, cz)];
}
static inline const ChunkMesh* worldMesh(const World* w, int cx, int cz) {
    return &w->chunks[worldSlotIndex(w, cx, cz)];
}

extern const unsigned char* g_edgeColumn;
extern int g_edgeSkyFromY;

extern const unsigned char* g_seaColumn;

extern void (*g_onEdgeSectionBuilt)(int cx, int cz, int si);

static inline bool edgeColumnTopRotates(const unsigned char* col) {
    for (int y = WORLD_H - 1; y >= 0; y--)
        if (col[y] != BLOCK_AIR) return (rotFaceMask(col[y]) >> 1) & 1;
    return false;
}

static inline bool edgeSectionVisible(const unsigned char* col, int si) {
    for (int y = si * SECTION_SY; y < (si + 1) * SECTION_SY; y++) {
        if (col[y] == BLOCK_AIR) continue;
        const unsigned char up = (y + 1 < WORLD_H) ? col[y + 1] : (unsigned char)BLOCK_AIR;
        if (up == BLOCK_AIR || isWaterId(up)) return true;
    }
    return false;
}

static inline unsigned char worldBlock(const World* w, int x, int y, int z) {
    if (y < 0 || y >= WORLD_H) return BLOCK_AIR;
    if (!worldReady(w, x, z))
        return g_edgeColumn ? g_edgeColumn[y] : BLOCK_INVISIBLE_BEDROCK;
    const BlockSection* s = &w->bsec[bsSection(w, x, y, z)];
    if (!s->page) return s->uniform;
    int off = bsOffset(x, y, z);
    if (!s->palN) return s->page[off];
    return s->pal[(s->page[off >> 1] >> ((off & 1) << 2)) & 0x0F];
}

void blockAlloc(World* w);
void blockFree(World* w);

bool blockPut(World* w, int x, int y, int z, unsigned char id);

void blockColumnGet(const World* w, int x, int z, unsigned char* out128);
void blockColumnPut(World* w, int x, int z, const unsigned char* in128);
unsigned int blockBytes(const World* w);

void blockStats(const World* w, int* uniform, int* paletted, int* raw);

void blockSlotRecycle(World* w, int slotIdx);

bool blockSectionUniform(const World* w, int x, int y, int z, unsigned char* out);

extern unsigned int g_blockOomDrops;

#define WORLD_DATA_PAGE (WORLD_H / 2)

static inline unsigned int worldDataBytes(const World* w) {
    return (unsigned int)(WORLD_W * WORLD_D) * sizeof(unsigned char*)
         + (unsigned int)w->dataPages * WORLD_DATA_PAGE;
}
unsigned int lightBytes(const World* w);
static inline unsigned int worldMemBytes(const World* w) {
    return blockBytes(w)
         + (unsigned int)(WORLD_W * WORLD_D)
         + worldDataBytes(w)
         + lightBytes(w);
}

static inline unsigned char worldData(const World* w, int x, int y, int z) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return 0;
    const unsigned char* pg = w->dataCol[worldColumn(w, x, z)];
    if (!pg) return 0;
    return (y & 1) ? (unsigned char)(pg[y >> 1] >> 4)
                   : (unsigned char)(pg[y >> 1] & 0x0F);
}

static inline void worldDataPut(World* w, int i, unsigned char v) {
    unsigned char* pg = w->dataCol[i / WORLD_H];
    if (!pg) {

        if (!(v & 0x0F)) return;
        pg = (unsigned char*)calloc(1, WORLD_DATA_PAGE);
        if (!pg) return;
        w->dataPages++;

        w->dataCol[i / WORLD_H] = pg;
    }
    int y = i % WORLD_H;
    unsigned char& b = pg[y >> 1];
    b = (y & 1) ? (unsigned char)((b & 0x0F) | (unsigned char)((v & 0x0F) << 4))
                : (unsigned char)((b & 0xF0) | (v & 0x0F));
}

static inline void worldDataColumnPut(World* w, int x, int z, const unsigned char* src) {
    int col = worldColumn(w, x, z);
    unsigned char* pg = w->dataCol[col];
    if (!pg) {
        int i = 0;
        while (i < WORLD_DATA_PAGE && !src[i]) i++;
        if (i == WORLD_DATA_PAGE) return;
        pg = (unsigned char*)calloc(1, WORLD_DATA_PAGE);
        if (!pg) return;
        w->dataPages++;
        w->dataCol[col] = pg;
    }
    memcpy(pg, src, WORLD_DATA_PAGE);
}

static inline void worldDataColumnGet(const World* w, int x, int z, unsigned char* dst) {
    const unsigned char* pg = w->dataCol[worldColumn(w, x, z)];
    if (pg) memcpy(dst, pg, WORLD_DATA_PAGE);
    else    memset(dst, 0, WORLD_DATA_PAGE);
}

bool worldSetBlockAndData(World* w, int x, int y, int z, unsigned char id, unsigned char data);
void worldSetData(World* w, int x, int y, int z, unsigned char data);

void worldSetDataNoUpdate(World* w, int x, int y, int z, unsigned char data);

void worldMarkDirty(World* w, int x, int y, int z);

void worldDrainPlayerEdits(World* w, int maxSections);

int worldEditQueueDepth();

int worldEditQueueFront(int field);

void worldEditQueueDropSlot(int slotIdx);

void worldRebuildAroundNow(World* w, int x, int y, int z);

void worldRebuildRegionNow(World* w, int x0, int y0, int z0, int x1, int y1, int z1);

void lightOnBlockChanged(World* w, int x, int y, int z);

void worldRelightBox(World* w, int x0, int y0, int z0, int x1, int y1, int z1);

static inline int lightPlaneIdx(const World* w, int layer, int x, int y, int z) {
    return (((worldSlotIndex(w, x >> 4, z >> 4) << 7) | y) << 1) | layer;
}

static inline int lightPlaneSlot0(const World* w, int layer, int cx, int cz) {
    return ((worldSlotIndex(w, cx, cz) << 7) << 1) | layer;
}
static inline unsigned char* lightPage(const World* w, unsigned int id) {
    return w->lightPool[id >> 8] + ((id & (LP_BLK_PAGES - 1)) << 7);
}

static inline int lightPi(int x, int z) { return ((x & 15) << 4) | (z & 15); }

unsigned int lightPagePromote(World* w, int idxSlot, unsigned char prefill);

static inline int lightLayerGet(const World* w, int layer, int x, int y, int z) {
    unsigned int id = w->lightIdx[lightPlaneIdx(w, layer, x, y, z)];
    if (id >= LP_SENT) return (int)(id & 1) * 15;
    int pi = lightPi(x, z);
    return (lightPage(w, id)[pi >> 1] >> ((pi & 1) * 4)) & 15;
}
static inline void lightLayerSet(World* w, int layer, int x, int y, int z, int v) {
    int slot = lightPlaneIdx(w, layer, x, y, z);
    unsigned int id = w->lightIdx[slot];
    if (id >= LP_SENT) {
        if (v == (int)(id & 1) * 15) return;
        id = lightPagePromote(w, slot, (unsigned char)((id & 1) ? 0xFF : 0x00));
        if (id >= LP_SENT) return;
    }
    unsigned char* p = lightPage(w, id);
    int pi = lightPi(x, z);
    unsigned char& b = p[pi >> 1];
    b = (pi & 1) ? (unsigned char)((b & 0x0F) | (v << 4))
                 : (unsigned char)((b & 0xF0) | v);
}

static inline bool lightPlaneAllDark(const World* w, int layer, int x, int y, int z) {
    return w->lightIdx[lightPlaneIdx(w, layer, x, y, z)] == LP_ALL0;
}

static inline int lightSkyGet(const World* w, int x, int y, int z) {
    if (y >= WORLD_H) return 15;
    if (y < 0) return 0;
    if (!worldReady(w, x, z)) return g_edgeColumn ? (y >= g_edgeSkyFromY ? 15 : 2) : 0;
    return lightLayerGet(w, 0, x, y, z);
}
static inline int lightBlockGet(const World* w, int x, int y, int z) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return 0;
    return lightLayerGet(w, 1, x, y, z);
}
static inline void lightSkySet(World* w, int x, int y, int z, int v) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return;
    lightLayerSet(w, 0, x, y, z, v);
}
static inline void lightBlockSet(World* w, int x, int y, int z, int v) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return;
    lightLayerSet(w, 1, x, y, z, v);
}

#define TICKS_PER_DAY 19200

#define CREATIVE_STOP_TIME 5000

extern int g_skyDarken;

float worldTimeOfDay(long dayTime, float a);

void worldSetNightMode(World* w, bool night);

bool worldNightModeTick(World* w);

void worldUpdateSkyDarken(World* w);

static inline bool worldIsDay() { return g_skyDarken < 4; }

static inline int lightRawAtNoProp(const World* w, int x, int y, int z) {

    if (y >= WORLD_H + 8) return 15;

    if (y >= WORLD_H) { int s = 15 - g_skyDarken; return s < 0 ? 0 : s; }
    if (y < 0) return 0;
    if (!worldReady(w, x, z)) {
        if (!g_edgeColumn) return 0;
        int s = (y >= g_edgeSkyFromY ? 15 : 2) - g_skyDarken;
        return s < 0 ? 0 : s;
    }

    int s = lightLayerGet(w, 0, x, y, z) - g_skyDarken, b = lightLayerGet(w, 1, x, y, z);
    if (s < 0) s = 0;
    return s > b ? s : b;
}

static inline int lightRawAt(const World* w, int x, int y, int z) {
    unsigned char id = worldBlock(w, x, y, z);
    if (isSlab(id) || id == BLOCK_FARMLAND) {
        int br = lightRawAtNoProp(w, x, y + 1, z);
        int b1 = lightRawAtNoProp(w, x + 1, y, z); if (b1 > br) br = b1;
        int b2 = lightRawAtNoProp(w, x - 1, y, z); if (b2 > br) br = b2;
        int b3 = lightRawAtNoProp(w, x, y, z + 1); if (b3 > br) br = b3;
        int b4 = lightRawAtNoProp(w, x, y, z - 1); if (b4 > br) br = b4;
        return br;
    }
    return lightRawAtNoProp(w, x, y, z);
}

static inline int lightSample(const World* w, unsigned char* cache, int i, int x, int y, int z);
static inline bool translucentSample(const World* w, const unsigned char* cache, int i, int x, int y, int z);

static inline int lightLazy(const World* w, unsigned char* cache, int i, int x, int y, int z) {
    int v = cache[i];
    if (v != 0xFF) return v;
    v = lightRawAt(w, x, y, z);
    cache[i] = (unsigned char)v;
    return v;
}

extern bool g_smoothLighting;

static inline void smoothFaceLight(const World* w, const unsigned char* lc,
                                   unsigned char* llc, int nbi, int nx, int ny, int nz,
                                   int f, float br[2][2]) {

    static const int kAxisStride[3] = { 18 * 18, 1, 18 };
    const int a  = f >> 1;
    const int a1 = (a + 1) % 3, a2 = (a + 2) % 3;
    const int s1 = kAxisStride[a1], s2 = kAxisStride[a2];
    const int p0[3] = { nx, ny, nz };
    const float mid = g_brightRamp[lightSample(w, llc, nbi, nx, ny, nz)];
    if (!g_smoothLighting) {
        br[0][0] = br[0][1] = br[1][0] = br[1][1] = mid;
        return;
    }

    int   ei[2], ej[2];
    int   ep1[2][3], ep2[2][3];
    float eb1[2], eb2[2];
    bool  et1[2], et2[2];
    for (int i = 0; i < 2; i++) {
        const int d1 = i ? 1 : -1;
        ei[i] = nbi + d1 * s1;
        ep1[i][0] = p0[0]; ep1[i][1] = p0[1]; ep1[i][2] = p0[2]; ep1[i][a1] += d1;
        eb1[i] = g_brightRamp[lightSample(w, llc, ei[i], ep1[i][0], ep1[i][1], ep1[i][2])];
        et1[i] = translucentSample(w, lc, ei[i], ep1[i][0], ep1[i][1], ep1[i][2]);
    }
    for (int j = 0; j < 2; j++) {
        const int d2 = j ? 1 : -1;
        ej[j] = nbi + d2 * s2;
        ep2[j][0] = p0[0]; ep2[j][1] = p0[1]; ep2[j][2] = p0[2]; ep2[j][a2] += d2;
        eb2[j] = g_brightRamp[lightSample(w, llc, ej[j], ep2[j][0], ep2[j][1], ep2[j][2])];
        et2[j] = translucentSample(w, lc, ej[j], ep2[j][0], ep2[j][1], ep2[j][2]);
    }
    for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++) {
        const int d2 = j ? 1 : -1;
        const int i1 = ei[i];
        const int* p1 = ep1[i];
        const float b1 = eb1[i], b2 = eb2[j];

        float bc = (a == 2) ? b1 : b2;

        if (et1[i] || et2[j]) {
            int pd[3] = { p1[0], p1[1], p1[2] }; pd[a2] += d2;
            bc = g_brightRamp[lightSample(w, llc, i1 + d2 * s2, pd[0], pd[1], pd[2])];
        }
        br[i][j] = (mid + b1 + b2 + bc) * 0.25f;
    }
}

static inline int lightSample(const World* w, unsigned char* cache, int i, int x, int y, int z) {
    return cache ? lightLazy(w, cache, i, x, y, z) : lightRawAt(w, x, y, z);
}
static inline bool translucentSample(const World* w, const unsigned char* cache, int i, int x, int y, int z) {
    unsigned char id = cache ? cache[i] : worldBlock(w, x, y, z);

    return !Tile::tiles[id]->blocksLight;
}

static inline void smoothFaceLightAt(const World* w, int nx, int ny, int nz,
                                     int f, float br[2][2]) {
    smoothFaceLight(w, 0, 0, 0, nx, ny, nz, f, br);
}

static inline unsigned int brightColorF(float b) {
    int c = (int)(b * 255.0f + 0.5f);
    if (c > 255) c = 255;
    if (c < 0) c = 0;
    return 0xFF000000u | ((unsigned)c << 16) | ((unsigned)c << 8) | (unsigned)c;
}

static inline bool worldCanSeeSky(const World* w, int x, int y, int z) {
    if (y >= WORLD_H) return true;
    if (y < 0 || !worldReady(w, x, z)) return false;
    return y >= w->heightmap[worldColumn(w, x, z)];
}

void worldInitLight(World* w);

void worldInitChunkLight(World* w, int cx, int cz);
void worldRecalcChunkHeightmap(World* w, int cx, int cz);

void worldScheduleChunkTicks(World* w, int cx, int cz);
void worldRecalcHeightmap(World* w);
void worldUpdateLights(World* w);
void worldMarkAllDirty(World* w);
void worldRemoveBlockLight(World* w, int x, int y, int z);

void lightQueuesReserve(World* w);

bool         lightAlloc(World* w);
void         lightFree(World* w);
void         lightClearAll(World* w);
unsigned int lightBytes(const World* w);

void         lightCompactStep(World* w);
void         lightCompactAll(World* w);

void         lightSlotRelease(World* w, int slotIdx);
void         lightChunkClearBlock(World* w, int cx, int cz);

void         lightInitSkyFromHeightmap(World* w);

void         lightInitSkyChunk(World* w, int cx, int cz);

void         lightLoadChunk(World* w, int cx, int cz,
                            const unsigned char* skyNib, const unsigned char* blockNib);

void worldScheduleTick(World* w, int x, int y, int z, unsigned char id, int tickDelay);
void worldTick(World* w);
void worldSettleLiquids(World* w);
void worldScheduleLoadedTicks(World* w);
void worldUpdateNeighbors(World* w, int x, int y, int z, unsigned char id);

void liquidFlow(const World* w, int x, int y, int z, unsigned char id,
                float* fx, float* fy, float* fz);

bool liquidStopsWater(unsigned char id);

void worldNotifyNeighborsChanged(World* w, int x, int y, int z);

bool worldSetTileUpdate(World* w, int x, int y, int z, unsigned char id, unsigned char data);

void worldExplode(World* w, float x, float y, float z, float r, bool inWater = false);

void worldPrimeTnt(World* w, int x, int y, int z, int fuseTicks, bool playFuse = true);

void tntSpawnPrimed(World* w, int x, int y, int z, int fuseTicks, bool playFuse = true);

bool tileMayPlace(World* w, unsigned char id, int x, int y, int z, int face, int data = -1);

bool tileUnobstructedAt(World* w, unsigned char id, int x, int y, int z);
void tileNeighborChanged(World* w, int x, int y, int z);
void tileRandomTick(World* w);

void heavyTileTick(World* w, int x, int y, int z, unsigned char id);

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

BlockHit worldClip(const World* w, float ax, float ay, float az,
                   float bx, float by, float bz, bool clipLiquids, bool solidOnly = false);

BlockHit worldPick(const World* w, float px, float py, float pz, float yaw, float pitch, float range,
                   bool clipLiquids = false);

int worldSelectionBoxes(const World* w, int x, int y, int z, float boxes[3][6]);

#endif
