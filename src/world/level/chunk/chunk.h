
#ifndef MCPSP_WORLD_CHUNK_H
#define MCPSP_WORLD_CHUNK_H

#include "world/level/tile/tile.h"

#define CHUNK_SX 16
#define CHUNK_SZ 16
#define CHUNK_SY 128

#define SECTION_SY  16
#define N_SECTIONS  (CHUNK_SY / SECTION_SY)

enum { BLOCK_AIR = 0,
       BLOCK_STONE = 1, BLOCK_GRASS = 2, BLOCK_DIRT = 3, BLOCK_COBBLESTONE = 4,
       BLOCK_PLANKS = 5, BLOCK_SAPLING = 6, BLOCK_BEDROCK = 7,

       BLOCK_WATER = 8, BLOCK_CALM_WATER = 9, BLOCK_LAVA = 10, BLOCK_CALM_LAVA = 11,
       BLOCK_SAND = 12, BLOCK_GRAVEL = 13,
       BLOCK_ORE_GOLD = 14, BLOCK_ORE_IRON = 15, BLOCK_ORE_COAL = 16,

       BLOCK_LOG = 17, BLOCK_LEAVES = 18,

       BLOCK_SPONGE = 19,
       BLOCK_GLASS = 20, BLOCK_ORE_LAPIS = 21, BLOCK_LAPIS_BLOCK = 22,
       BLOCK_SANDSTONE = 24,
       BLOCK_BED = 26,

       BLOCK_GOLDEN_RAIL = 27,

       BLOCK_COBWEB = 30,

       BLOCK_TALLGRASS = 31,

       BLOCK_WOOL = 35,

       BLOCK_FLOWER = 37, BLOCK_ROSE = 38, BLOCK_MUSHROOM_BROWN = 39, BLOCK_MUSHROOM_RED = 40,
       BLOCK_GOLD_BLOCK = 41, BLOCK_IRON_BLOCK = 42,

       BLOCK_DOUBLE_SLAB = 43, BLOCK_SLAB = 44,
       BLOCK_BRICKS = 45, BLOCK_TNT = 46, BLOCK_BOOKSHELF = 47,
       BLOCK_MOSSY_COBBLE = 48, BLOCK_OBSIDIAN = 49, BLOCK_TORCH = 50,
       BLOCK_FIRE = 51,
       BLOCK_STAIRS_PLANKS = 53, BLOCK_CHEST = 54,
       BLOCK_ORE_EMERALD = 56,
       BLOCK_DIAMOND_BLOCK = 57, BLOCK_CRAFTING_TABLE = 58,
       BLOCK_WHEAT = 59, BLOCK_FARMLAND = 60,

       BLOCK_FURNACE = 61, BLOCK_FURNACE_LIT = 62,

       BLOCK_SIGN = 63,
       BLOCK_DOOR_WOOD = 64, BLOCK_LADDER = 65,

       BLOCK_RAIL = 66,
       BLOCK_STAIRS_COBBLESTONE = 67, BLOCK_WALL_SIGN = 68, BLOCK_DOOR_IRON = 71,
       BLOCK_ORE_REDSTONE = 73, BLOCK_ORE_REDSTONE_LIT = 74,
       BLOCK_TOPSNOW = 78,
       BLOCK_ICE = 79, BLOCK_SNOW_BLOCK = 80,
       BLOCK_CACTUS = 81,
       BLOCK_CLAY = 82,
       BLOCK_REEDS = 83,
       BLOCK_FENCE = 85, BLOCK_NETHERRACK = 87, BLOCK_GLOWSTONE = 89,
       BLOCK_CAKE = 92,

       BLOCK_INVISIBLE_BEDROCK = 95,
       BLOCK_TRAPDOOR = 96,
       BLOCK_STONE_BRICKS = 98,

       BLOCK_IRON_BARS = 101,
       BLOCK_GLASS_PANE = 102, BLOCK_MELON = 103, BLOCK_MELON_STEM = 105,

       BLOCK_PUMPKIN = 86, BLOCK_PUMPKIN_LIT = 91,

       BLOCK_PUMPKIN_STEM = 104,
       BLOCK_FENCE_GATE = 107, BLOCK_STAIRS_BRICK = 108, BLOCK_STAIRS_STONE_BRICK = 109,
       BLOCK_NETHER_BRICK = 112, BLOCK_STAIRS_NETHER_BRICK = 114,
       BLOCK_STAIRS_SANDSTONE = 128,

       BLOCK_HAY_BLOCK = 170,

       BLOCK_CARPET = 171,

       BLOCK_COAL_BLOCK = 173,

       BLOCK_BEETROOT = 244,

       BLOCK_STAIRS_SPRUCE = 134, BLOCK_STAIRS_BIRCH = 135, BLOCK_STAIRS_JUNGLE = 136,

       BLOCK_COBBLE_WALL = 139,

       BLOCK_CARROTS = 141, BLOCK_POTATOES = 142,
       BLOCK_QUARTZ_BLOCK = 155,
       BLOCK_STAIRS_QUARTZ = 156,

       BLOCK_WOOD_SLAB_DOUBLE = 157, BLOCK_WOOD_SLAB = 158,

       BLOCK_STONECUTTER = 245, BLOCK_GLOWING_OBSIDIAN = 246, BLOCK_NETHER_REACTOR = 247,

       BLOCK_UPDATE1 = 248, BLOCK_UPDATE2 = 249 };
enum { WOOL_COLORS = 16 };

static inline bool isUpdateBlock(unsigned char id) { return id == BLOCK_UPDATE1 || id == BLOCK_UPDATE2; }

enum { STAIR_DIR_MASK = 3, STAIR_DIR_EAST=0, STAIR_DIR_WEST=1, STAIR_DIR_SOUTH=2, STAIR_DIR_NORTH=3,
       STAIR_UPSIDEDOWN_BIT = 4 };

enum { SLAB_TOP_SLOT_BIT = 8 };
enum { DSLAB_STONE = 0, DSLAB_SAND = 1, DSLAB_WOOD = 2, DSLAB_COBBLE = 3,
       DSLAB_BRICK = 4, DSLAB_SMOOTHBRICK = 5, DSLAB_QUARTZ = 6, DSLAB_MAT_MASK = 7 };

enum { LOG_OAK = 0, LOG_SPRUCE = 1, LOG_BIRCH = 2, LOG_JUNGLE = 3, LOG_TYPE_MASK = 3 };
enum { LOG_AXIS_Y = 0, LOG_AXIS_X = 4, LOG_AXIS_Z = 8, LOG_AXIS_MASK = 0xC };

enum { PLANK_OAK = 0, PLANK_SPRUCE = 1, PLANK_BIRCH = 2, PLANK_JUNGLE = 3,
       PLANK_TYPE_MASK = 3 };
enum { SS_DEFAULT = 0, SS_CHISELED = 1, SS_SMOOTH = 2 };

enum { TG_DEAD_SHRUB = 0, TG_TALL_GRASS = 1, TG_FERN = 2 };
enum { QZ_DEFAULT = 0, QZ_CHISELED = 1, QZ_PILLAR = 2 };
enum { SB_NORMAL = 0, SB_MOSSY = 1, SB_CRACKED = 2 };

enum { WALL_COBBLE = 0, WALL_MOSSY = 1 };

static inline bool isStairs(unsigned char id) {
    switch (id) {
        case BLOCK_STAIRS_PLANKS: case BLOCK_STAIRS_COBBLESTONE:
        case BLOCK_STAIRS_SPRUCE: case BLOCK_STAIRS_BIRCH: case BLOCK_STAIRS_JUNGLE:
        case BLOCK_STAIRS_BRICK:  case BLOCK_STAIRS_STONE_BRICK:
        case BLOCK_STAIRS_SANDSTONE: case BLOCK_STAIRS_NETHER_BRICK:
        case BLOCK_STAIRS_QUARTZ:
            return true;
        default: return false;
    }
}
static inline bool isSlab(unsigned char id)   { return id == BLOCK_SLAB || id == BLOCK_WOOD_SLAB; }

static inline bool isPane(unsigned char id)   { return id == BLOCK_GLASS_PANE || id == BLOCK_IRON_BARS; }

static inline bool isCarpet(unsigned char id) { return id == BLOCK_CARPET; }

static inline bool isHayBlock(unsigned char id) { return id == BLOCK_HAY_BLOCK; }

#define PANE_EDGE_COL 4
#define PANE_EDGE_ROW 9
static inline bool isFence(unsigned char id)  { return id == BLOCK_FENCE; }
static inline bool isWall(unsigned char id)   { return id == BLOCK_COBBLE_WALL; }
static inline bool isFenceGate(unsigned char id){ return id == BLOCK_FENCE_GATE; }
static inline bool isDoor(unsigned char id)   { return id == BLOCK_DOOR_WOOD || id == BLOCK_DOOR_IRON; }
static inline bool isBed(unsigned char id)    { return id == BLOCK_BED; }
static inline bool isTrapdoor(unsigned char id) { return id == BLOCK_TRAPDOOR; }
static inline bool isLadder(unsigned char id) { return id == BLOCK_LADDER; }

static inline bool isRail(unsigned char id) { return id == BLOCK_RAIL || id == BLOCK_GOLDEN_RAIL; }

static inline bool railUsesDataBit(unsigned char id) { return id == BLOCK_GOLDEN_RAIL; }

static inline int railDir(unsigned char id, unsigned char data) {
    return railUsesDataBit(id) ? (data & 7) : (data & 15);
}
static inline bool isTorch(unsigned char id)  { return id == BLOCK_TORCH; }
static inline bool isSign(unsigned char id)   { return id == BLOCK_SIGN || id == BLOCK_WALL_SIGN; }

static inline bool isWaterId(unsigned char id) { return id == BLOCK_WATER || id == BLOCK_CALM_WATER; }
static inline bool isLavaId(unsigned char id)  { return id == BLOCK_LAVA  || id == BLOCK_CALM_LAVA;  }
static inline bool isLiquidId(unsigned char id){ return isWaterId(id) || isLavaId(id); }

static inline bool nearBlocksView(unsigned char id) {
    return id != BLOCK_AIR && !isLiquidId(id);
}

static inline int liquidTickDelay(unsigned char id) { return isWaterId(id) ? 5 : 30; }

static inline float liquidTileHeight(int d) {
    if (d >= 8) d = 0;
    return (d + 1) / 9.0f;
}

static inline bool sameLiquid(unsigned char a, unsigned char b) {
    return (isWaterId(a) && isWaterId(b)) || (isLavaId(a) && isLavaId(b));
}

static inline unsigned char calmOf(unsigned char id) {
    return id == BLOCK_WATER ? BLOCK_CALM_WATER : id == BLOCK_LAVA ? BLOCK_CALM_LAVA : id;
}
static inline unsigned char dynOf(unsigned char id) {
    return id == BLOCK_CALM_WATER ? BLOCK_WATER : id == BLOCK_CALM_LAVA ? BLOCK_LAVA : id;
}

static inline bool isHeavyTile(unsigned char id) { return id == BLOCK_SAND || id == BLOCK_GRAVEL; }

static inline bool isReplaceable(unsigned char id) { return Tile::tiles[id]->replaceable; }

static inline bool isSolidPhys(unsigned char id) { return Tile::tiles[id]->solidPhys; }

static inline bool isCubeShaped(unsigned char id) { return Tile::tiles[id]->cube; }

static inline bool isSolidBlocking(unsigned char id) { return Tile::tiles[id]->solidPhys; }

static inline bool autoJumpable(unsigned char id, unsigned char data) {
    (void)data;
    if (isStairs(id))                   return false;
    if (isSlab(id))                     return false;
    if (isFence(id) || isFenceGate(id)) return false;
    if (isWall(id))                     return false;
    if (isTrapdoor(id) || isSign(id))   return false;
    if (id == BLOCK_COBWEB)             return false;
    if (isCarpet(id))                   return false;
    return true;
}

static inline bool isTrapdoorAttachable(unsigned char id) {
    if (id == 0) return false;
    if (isSolidBlocking(id) && isCubeShaped(id)) return true;
    if (id == BLOCK_GLOWSTONE || isSlab(id) || isStairs(id)) return true;
    return false;
}

static inline int lightOpacity(unsigned char id) { return Tile::tiles[id]->lightBlock; }
static inline int lightEmit(unsigned char id)     { return Tile::tiles[id]->lightEmission; }

static inline unsigned char rotFaceMask(unsigned char id) { return Tile::tiles[id]->rotFaceMask; }

extern unsigned int g_brightColor[16];
extern float g_brightRamp[16];

#define BRIGHT_GAMMA_MAX 0.6f
extern float g_brightGamma;

static inline float brightGammaApply(float b, float gamma) {
    if (gamma <= 0.0f) return b;
    float inv = 1.0f - b;
    inv = 1.0f - inv * inv * inv * inv;
    return b * (1.0f - gamma) + inv * gamma;
}

void chunkInitBrightRamp(void);

#define ENTITY_LIGHT_FLOOR     0.35f
#define TILE_ENTITY_LIGHT_FLOOR 0.4f
static inline unsigned int brightColorFloored(int raw, float floorB) {
    if (raw < 0)  raw = 0;
    if (raw > 15) raw = 15;
    float b = g_brightRamp[raw];
    if (b > 1.0f)     b = 1.0f;
    else if (b <= floorB) b = floorB;
    unsigned int c = (unsigned int)(b * 255.0f + 0.5f);
    return 0xFF000000u | (c << 16) | (c << 8) | c;
}

static inline bool isCropTile(unsigned char id) {
    return id == BLOCK_WHEAT || id == BLOCK_CARROTS ||
           id == BLOCK_POTATOES || id == BLOCK_BEETROOT;
}

static inline bool isStemTile(unsigned char id) {
    return id == BLOCK_MELON_STEM || id == BLOCK_PUMPKIN_STEM;
}

static inline unsigned char stemFruit(unsigned char id) {
    return id == BLOCK_PUMPKIN_STEM ? BLOCK_PUMPKIN : BLOCK_MELON;
}

static inline bool isCrossShaped(unsigned char id) {
    return id == BLOCK_FLOWER || id == BLOCK_ROSE ||
           id == BLOCK_MUSHROOM_BROWN || id == BLOCK_MUSHROOM_RED ||
           id == BLOCK_REEDS || id == BLOCK_SAPLING ||
           isCropTile(id) || isStemTile(id) ||
           id == BLOCK_TALLGRASS ||
           id == BLOCK_COBWEB;
}

static inline bool isLeaf(unsigned char id) { return id == BLOCK_LEAVES; }
static inline bool isLog(unsigned char id) { return id == BLOCK_LOG; }
static inline bool isWool(unsigned char id) { return id == BLOCK_WOOL; }
static inline bool isGlass(unsigned char id) { return id == BLOCK_GLASS; }

enum { LEAF_TYPE_MASK = 3, LEAF_OAK = 0, LEAF_SPRUCE = 1, LEAF_BIRCH = 2,
       LEAF_JUNGLE = 3,
       LEAF_UPDATE_BIT = 4, LEAF_PERSISTENT_BIT = 8 };

static inline bool isOpaque(unsigned char id) { return Tile::tiles[id]->opaque; }

static inline bool connectsFence(unsigned char nb) { return isFence(nb) || isFenceGate(nb) || isOpaque(nb); }

static inline bool connectsWall(unsigned char nb) {
    return isWall(nb) || isFenceGate(nb) || Tile::tiles[nb]->wallConnect;
}

static inline bool paneAttachsTo(unsigned char id, unsigned char nb) {
    return nb == id || isGlass(nb) || isOpaque(nb);
}

static inline unsigned int div255(unsigned int x) { return (x + 1 + (x >> 8)) >> 8; }

static inline unsigned int mulColor(unsigned int a, unsigned int b) {
    unsigned int r = div255(((a)       & 0xFF) * ((b)       & 0xFF));
    unsigned int g = div255(((a >> 8)  & 0xFF) * ((b >> 8)  & 0xFF));
    unsigned int bl= div255(((a >> 16) & 0xFF) * ((b >> 16) & 0xFF));
    return 0xFF000000u | (bl << 16) | (g << 8) | r;
}

#define TILE_UV (1.0f / 16.0f)

enum { F_LEFT, F_RIGHT, F_DOWN, F_TOP, F_BACK, F_FORWARD };

static inline int faceFromMcpe(int d) {
    switch (d) {
        case 2:  return F_BACK;
        case 3:  return F_FORWARD;
        case 4:  return F_LEFT;
        default: return F_RIGHT;
    }
}

static inline int mcpeFromFace(int f) {
    switch (f) {
        case F_BACK:    return 2;
        case F_FORWARD: return 3;
        case F_LEFT:    return 4;
        default:        return 5;
    }
}

extern const signed char kFaceNeighbor[6][3];

extern const unsigned int kFaceShade[6];

void tileForBlock(unsigned char id, unsigned char data, int f, int* col, int* row, unsigned int* tint);

struct World;

struct ChunkVertex {
    float u, v;
    unsigned int color;
    float x, y, z;
};

int emitLiquid(const World* w, int gx, int y, int gz, unsigned char id, ChunkVertex* out, int n);

int emitPartialBox(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data,
                   float x0, float y0, float z0, float x1, float y1, float z1,
                   int boundaryMask, int hiddenFaces, ChunkVertex* out, int n, bool fixUV = true,
                   bool fullTileUV = false, int edgeFaceMask = 0);
int emitSlab(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitCarpet(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitStairs(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);

int emitPane(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitFence(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitWall(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitDoor(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitTrapdoor(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitFenceGate(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitCake(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitTorch(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitBed(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n);
int emitMelonStem(const World* w, ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id, unsigned char data, unsigned int bright);

int emitCross(ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id, unsigned char data, unsigned int bright, bool jitter = true);
int emitCropRows(ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id, unsigned char data, unsigned int bright);

#define POS_ENC 256
#define POS_MODEL_SCALE (32768.0f / POS_ENC)

extern float g_relBaseX, g_relBaseY, g_relBaseZ;
static inline short posQ(float v) { return (short)(v * POS_ENC + (v < 0 ? -0.5f : 0.5f)); }

static inline short uvQ(float t) {
    float e = t * 32767.0f;
    if (e >  32767.0f) e =  32767.0f;
    if (e < -32768.0f) e = -32768.0f;
    return (short)(e + (e < 0 ? -0.5f : 0.5f));
}

struct DrawVertex {
    short u, v;
    unsigned int color;
    short x, y, z, w;
};

struct ChunkSection {
    DrawVertex*  mesh;
    int          vertexCount;
    DrawVertex*  water;
    int          waterCount;
    DrawVertex*  leaves;
    int          leavesCount;

    DrawVertex*  noMip;
    int          noMipCount;

    int          noMipLavaStart;

    int          ox, oy, oz;
    float        by0, by1;
    float        lby0, lby1;
    float        wby0, wby1;
    bool         dirty;
    bool         visible;

    bool         leavesOpaqueBand;

    bool         leavesCullBand;

    bool          skyLit;
};

struct ChunkMesh {
    ChunkSection sec[N_SECTIONS];
    int          ox, oz;
    float        cx, cz;

    bool         drawn;
};

bool worldColumnDrawn(const World* w, float x, float z);

int meshPass(const World* w, int ox, int oz, int y0, int y1, ChunkVertex* out, int layer, int cap = 0x7fffffff, bool leavesOpaque = true, bool leavesCull = true, int* nLava = 0);

int meshSection(const World* w, int ox, int oz, int y0, int y1,
                ChunkVertex* out0, ChunkVertex* out1, ChunkVertex* out2, ChunkVertex* out3,
                int cap0, int cap1, int cap2, int cap3, int* n0, int* n1, int* n2, int* n3,
                int* nLava, bool leavesOpaque, bool leavesCull);

DrawVertex* chunkPack(const ChunkVertex* src, int n, int ox, int oy, int oz,
                      float* ylo = 0, float* yhi = 0);

void chunkBuildMesh(ChunkMesh* c, const World* w, int ox, int oz);

void chunkInitLazy(ChunkMesh* c, int ox, int oz);

bool sectionCannotEmit(const World* w, int ox, int oz, int si);

void chunkBuildSection(ChunkMesh* c, const World* w, int si);

void chunkMeshHeapProbe();
void chunkDrawSection(const ChunkSection* s);
void chunkDrawWaterSection(const ChunkSection* s);
void chunkDrawLeavesSection(const ChunkSection* s);

enum { NOMIP_ALL = 0, NOMIP_NO_LAVA = 1, NOMIP_LAVA = 2 };
void chunkDrawNoMipSection(const ChunkSection* s, int part = NOMIP_ALL);
void chunkFreeMesh(ChunkMesh* c);

#endif
