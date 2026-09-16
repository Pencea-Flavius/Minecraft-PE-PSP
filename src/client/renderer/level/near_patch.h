
#ifndef MCPSP_NEAR_PATCH_H
#define MCPSP_NEAR_PATCH_H

struct World;

static const float NEAR_PATCH_Z = 0.12f;

bool nearPatchUpdate(const World* w, float ex, float ey, float ez, float fov);

void nearPatchRefresh(const World* w);

enum { NEAR_PATCH_OPAQUE, NEAR_PATCH_CUTOUT, NEAR_PATCH_LAVA, NEAR_PATCH_LEAVES,
       NEAR_PATCH_WATER, NEAR_PATCH_RANGES };
bool nearPatchHas(int range);
void nearPatchDraw(int range);

struct ChunkSection;

bool nearPatchOwnsWater(const ChunkSection* s, int ox, int oz);

void nearPatchSplitParams(float* minEdge, float* safePerEdge);

void nearPatchReserve(void);

void nearPatchFree(void);

void nearPatchStats(int* verts, int* cands, unsigned int* buildUs, unsigned int* frameUs, int* state);

#endif
