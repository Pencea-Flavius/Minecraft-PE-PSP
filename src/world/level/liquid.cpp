
#include "world/entity/local_player.h"
#include "world/level/level.h"
#include "world/level/world.h"

extern Level g_level;
#include "world/level/tile/redstone_ore.h"
#include "world/level/tile/fire.h"
#include "world/inventory/inventory.h"
#include "client/renderer/particle.h"
#include "platform/audio/sound.h"

#include <stdlib.h>
#include <math.h>
#include <pspkernel.h>
#include "util/prof.h"

bool liquidStopsWater(unsigned char id) {
    return id == BLOCK_REEDS || isSign(id) || isLadder(id) || id == BLOCK_DOOR_WOOD;
}

static bool isWaterBlocking(const World* w, int x, int y, int z) {
    unsigned char id = worldBlock(w, x, y, z);
    if (id == BLOCK_AIR) return false;
    if (liquidStopsWater(id)) return true;

    return isSolidPhys(id);
}

static int getFlowDepth(const World* w, int x, int y, int z, unsigned char liquidId) {
    if (!sameLiquid(worldBlock(w, x, y, z), liquidId)) return -1;
    int d = worldData(w, x, y, z);
    if (d >= 8) d = 0;
    return d;
}

static inline bool blocksFlow(unsigned char id) {
    return !(id == BLOCK_AIR || isLiquidId(id) || isCrossShaped(id));
}

void liquidFlow(const World* w, int x, int y, int z, unsigned char id,
                float* fx, float* fy, float* fz) {
    float flowX = 0.0f, flowY = 0.0f, flowZ = 0.0f;
    int mid = getFlowDepth(w, x, y, z, id);
    for (int d = 0; d < 4; d++) {
        int xt = x, yt = y, zt = z;
        if (d == 0) xt--;
        if (d == 1) zt--;
        if (d == 2) xt++;
        if (d == 3) zt++;

        int t = getFlowDepth(w, xt, yt, zt, id);
        if (t < 0) {
            if (!blocksFlow(worldBlock(w, xt, yt, zt))) {
                t = getFlowDepth(w, xt, yt - 1, zt, id);
                if (t >= 0) {
                    int dir = t - (mid - 8);
                    flowX += (float)((xt - x) * dir);
                    flowY += (float)((yt - y) * dir);
                    flowZ += (float)((zt - z) * dir);
                }
            }
        } else {
            int dir = t - mid;
            flowX += (float)((xt - x) * dir);
            flowY += (float)((yt - y) * dir);
            flowZ += (float)((zt - z) * dir);
        }
    }
    float len = sqrtf(flowX * flowX + flowY * flowY + flowZ * flowZ);
    if (len > 0.0f) { flowX /= len; flowY /= len; flowZ /= len; }
    *fx = flowX; *fy = flowY; *fz = flowZ;
}

static int getDepth(const World* w, int x, int y, int z, unsigned char liquidId) {
    if (!sameLiquid(worldBlock(w, x, y, z), liquidId)) return -1;
    return worldData(w, x, y, z);
}

static int getHighest(const World* w, int x, int y, int z, int highest, int* maxCount, unsigned char liquidId) {
    int depth = getDepth(w, x, y, z, liquidId);
    if (depth < 0) return highest;
    if (depth == 0) (*maxCount)++;
    if (depth >= 8) depth = 0;

    if (highest < 0 || depth < highest) {
        return depth;
    }
    return highest;
}

static int getSlopeDistance(const World* w, int x, int y, int z, int pass, int from, unsigned char liquidId) {
    int lowest = 1000;
    for (int d = 0; d < 4; d++) {
        if (d == 0 && from == 1) continue;
        if (d == 1 && from == 0) continue;
        if (d == 2 && from == 3) continue;
        if (d == 3 && from == 2) continue;

        int xx = x, yy = y, zz = z;
        if (d == 0) xx--;
        if (d == 1) xx++;
        if (d == 2) zz--;
        if (d == 3) zz++;

        if (isWaterBlocking(w, xx, yy, zz)) {
            continue;
        } else if (sameLiquid(worldBlock(w, xx, yy, zz), liquidId) && worldData(w, xx, yy, zz) == 0) {
            continue;
        } else {
            if (!isWaterBlocking(w, xx, yy - 1, zz)) {
                return pass;
            } else {
                if (pass < 4) {
                    int v = getSlopeDistance(w, xx, yy, zz, pass + 1, d, liquidId);
                    if (v < lowest) lowest = v;
                }
            }
        }
    }
    return lowest;
}

static void getSpread(const World* w, int x, int y, int z, unsigned char liquidId, bool* result) {
    int dist[4];
    for (int d = 0; d < 4; d++) {
        dist[d] = 1000;
        int xx = x, yy = y, zz = z;
        if (d == 0) xx--;
        if (d == 1) xx++;
        if (d == 2) zz--;
        if (d == 3) zz++;

        if (isWaterBlocking(w, xx, yy, zz)) {
            continue;
        } else if (sameLiquid(worldBlock(w, xx, yy, zz), liquidId) && worldData(w, xx, yy, zz) == 0) {
            continue;
        } else {
            if (!isWaterBlocking(w, xx, yy - 1, zz)) {
                dist[d] = 0;
            } else {
                dist[d] = getSlopeDistance(w, xx, yy, zz, 1, d, liquidId);
            }
        }
    }

    int lowest = dist[0];
    for (int d = 1; d < 4; d++) {
        if (dist[d] < lowest) lowest = dist[d];
    }

    for (int d = 0; d < 4; d++) {
        result[d] = (dist[d] == lowest);
    }
}

static bool canSpreadTo(const World* w, int x, int y, int z, unsigned char liquidId) {
    unsigned char id = worldBlock(w, x, y, z);
    if (sameLiquid(id, liquidId)) return false;
    if (isLavaId(id)) return false;
    return !isWaterBlocking(w, x, y, z);
}

static void setStatic(World* w, int x, int y, int z, unsigned char id) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return;
    blockPut(w, x, y, z, calmOf(id));
}

static void wakeLiquid(World* w, int x, int y, int z, unsigned char liquidId) {
    unsigned char nb = worldBlock(w, x, y, z);
    if (!sameLiquid(nb, liquidId)) return;
    unsigned char dyn = dynOf(nb);
    if (nb != dyn) blockPut(w, x, y, z, dyn);
    worldScheduleTick(w, x, y, z, dyn, isWaterId(dyn) ? 5 : 30);
}

extern bool g_worldBuilt;
static void fizz(int x, int y, int z) {

    if (g_worldBuilt) {
        float pitch = 2.6f + ((rand() % 1000) / 1000.0f - (rand() % 1000) / 1000.0f) * 0.8f;

        g_level.playSound((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, "random.fizz", 0.5f, pitch);
    }
    for (int i = 0; i < 8; i++)
        particlesLargeSmoke((float)x + (rand() % 100) / 100.0f, (float)y + 1.2f, (float)z + (rand() % 100) / 100.0f);
}

static void hardenLava(World* w, int x, int y, int z) {
    unsigned char id = worldBlock(w, x, y, z);
    if (!isLavaId(id)) return;
    bool water = isWaterId(worldBlock(w, x - 1, y, z)) || isWaterId(worldBlock(w, x + 1, y, z))
              || isWaterId(worldBlock(w, x, y, z - 1)) || isWaterId(worldBlock(w, x, y, z + 1))
              || isWaterId(worldBlock(w, x, y + 1, z));
    if (!water) return;
    int d = worldData(w, x, y, z);
    if (d == 0)            worldSetBlockAndData(w, x, y, z, BLOCK_OBSIDIAN, 0);
    else if (d >= 1 && d <= 4) worldSetBlockAndData(w, x, y, z, BLOCK_COBBLESTONE, 0);
    else return;
    fizz(x, y, z);

    static const signed char nb6[6][3] = {{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + nb6[i][0], ny = y + nb6[i][1], nz = z + nb6[i][2];
        wakeLiquid(w, nx, ny, nz, BLOCK_LAVA);
        wakeLiquid(w, nx, ny, nz, BLOCK_WATER);
    }
}

static void checkHarden(World* w, int x, int y, int z) {
    static const signed char nb6[7][3] = {{0,0,0},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
    for (int d = 0; d < 7; d++)
        hardenLava(w, x + nb6[d][0], y + nb6[d][1], z + nb6[d][2]);
}

static void neighborChanged(World* w, int x, int y, int z) {
    unsigned char nb = worldBlock(w, x, y, z);
    if (isLiquidId(nb)) wakeLiquid(w, x, y, z, dynOf(nb));
    tileNeighborChanged(w, x, y, z);
}

void worldUpdateNeighbors(World* w, int x, int y, int z, unsigned char liquidId) {
    (void)liquidId;
    neighborChanged(w, x - 1, y, z);
    neighborChanged(w, x + 1, y, z);
    neighborChanged(w, x, y - 1, z);
    neighborChanged(w, x, y + 1, z);
    neighborChanged(w, x, y, z - 1);
    neighborChanged(w, x, y, z + 1);
    checkHarden(w, x, y, z);
}

void worldNotifyNeighborsChanged(World* w, int x, int y, int z) {
    neighborChanged(w, x - 1, y, z);
    neighborChanged(w, x + 1, y, z);
    neighborChanged(w, x, y - 1, z);
    neighborChanged(w, x, y + 1, z);
    neighborChanged(w, x, y, z - 1);
    neighborChanged(w, x, y, z + 1);
    leafFlagNeighbors(w, x, y, z);

    unsigned char here = worldBlock(w, x, y, z);
    if (isHeavyTile(here)) worldScheduleTick(w, x, y, z, here, 2);
    checkHarden(w, x, y, z);
}

bool worldSetTileUpdate(World* w, int x, int y, int z, unsigned char id, unsigned char data) {
    if (!worldSetBlockAndData(w, x, y, z, id, data)) return false;
    worldNotifyNeighborsChanged(w, x, y, z);
    return true;
}

static void trySpreadTo(World* w, int x, int y, int z, int neighbor, unsigned char liquidId) {
    if (canSpreadTo(w, x, y, z, liquidId)) {

        unsigned char flooded = worldBlock(w, x, y, z);
        if (flooded == BLOCK_FIRE && isWaterId(liquidId))
            fireExtinguishFx(x, y, z);
        else if (flooded != BLOCK_AIR && !isLiquidId(flooded))
            worldSpawnResources(w, x, y, z, flooded, worldData(w, x, y, z));
        worldSetBlockAndData(w, x, y, z, liquidId, neighbor);
        worldScheduleTick(w, x, y, z, liquidId, isWaterId(liquidId) ? 5 : 30);
        worldUpdateNeighbors(w, x, y, z, liquidId);
    }
}

static void tickLiquid(World* w, int x, int y, int z, unsigned char id) {
    int depth = getDepth(w, x, y, z, id);
    if (depth < 0) return;

    int dropOff = 1;
    if (isLavaId(id)) dropOff = 2;

    if (depth > 0) {
        int highest = -100;
        int maxCount = 0;
        highest = getHighest(w, x - 1, y, z, highest, &maxCount, id);
        highest = getHighest(w, x + 1, y, z, highest, &maxCount, id);
        highest = getHighest(w, x, y, z - 1, highest, &maxCount, id);
        highest = getHighest(w, x, y, z + 1, highest, &maxCount, id);

        int newDepth = highest + dropOff;
        if (newDepth >= 8 || highest < 0) {
            newDepth = -1;
        }

        int above = getDepth(w, x, y + 1, z, id);
        if (above >= 0) {
            if (above >= 8) newDepth = above;
            else newDepth = above + 8;
        }

        if (maxCount >= 2 && isWaterId(id)) {
            if (isWaterBlocking(w, x, y - 1, z)) {
                newDepth = 0;
            } else if (sameLiquid(worldBlock(w, x, y - 1, z), id) && worldData(w, x, y, z) == 0) {
                newDepth = 0;
            }
        }

        int tickDelay = liquidTickDelay(id);
        if (isLavaId(id) && depth <= 7 && newDepth <= 7 && newDepth > depth) {
            if (rand() % 4 != 0) tickDelay *= 4;
        }

        if (newDepth != depth) {
            depth = newDepth;
            if (depth < 0) {

                worldSetBlockAndData(w, x, y, z, BLOCK_AIR, 0);
                worldUpdateNeighbors(w, x, y, z, id);
            } else {
                worldSetData(w, x, y, z, depth);
                worldScheduleTick(w, x, y, z, id, tickDelay);
                worldUpdateNeighbors(w, x, y, z, id);
            }
        } else {
            setStatic(w, x, y, z, id);
        }
    } else {
        setStatic(w, x, y, z, id);
    }

    if (depth < 0) return;

    if (canSpreadTo(w, x, y - 1, z, id)) {

        if (isLavaId(id) && isWaterId(worldBlock(w, x, y - 1, z)) && worldData(w, x, y - 1, z) == 0) {
            worldSetBlockAndData(w, x, y - 1, z, BLOCK_STONE, 0);
            fizz(x, y - 1, z);

            worldUpdateNeighbors(w, x, y - 1, z, BLOCK_WATER);
            return;
        }
        if (depth >= 8) {
            worldSetBlockAndData(w, x, y - 1, z, id, depth);
            worldScheduleTick(w, x, y - 1, z, id, isWaterId(id) ? 5 : 30);
            worldUpdateNeighbors(w, x, y - 1, z, id);
        } else {
            worldSetBlockAndData(w, x, y - 1, z, id, depth + 8);
            worldScheduleTick(w, x, y - 1, z, id, isWaterId(id) ? 5 : 30);
            worldUpdateNeighbors(w, x, y - 1, z, id);
        }
    } else if (depth >= 0 && (depth == 0 || isWaterBlocking(w, x, y - 1, z))) {
        bool spreads[4];
        getSpread(w, x, y, z, id, spreads);
        int neighbor = depth + dropOff;
        if (depth >= 8) {
            neighbor = 1;
        }
        if (neighbor >= 8) return;

        if (spreads[0]) trySpreadTo(w, x - 1, y, z, neighbor, id);
        if (spreads[1]) trySpreadTo(w, x + 1, y, z, neighbor, id);
        if (spreads[2]) trySpreadTo(w, x, y, z - 1, neighbor, id);
        if (spreads[3]) trySpreadTo(w, x, y, z + 1, neighbor, id);
    }
}

void worldSettleLiquids(World* w) {
    const int MAX_PASSES = 8192;
    std::vector<TickNextTickData> batch;
    for (int pass = 0; pass < MAX_PASSES; pass++) {
        if (w->tickNextTickList.empty()) break;
        batch.clear();
        int keep = 0;
        for (size_t i = 0; i < w->tickNextTickList.size(); i++) {
            TickNextTickData& td = w->tickNextTickList[i];
            if (isLiquidId(td.tileId)) batch.push_back(td);
            else w->tickNextTickList[keep++] = td;
        }
        w->tickNextTickList.resize(keep);
        if (batch.empty()) break;

        for (size_t i = 0; i < batch.size(); i++)
            w->tickSet.erase((unsigned int)worldIndex(w, batch[i].x, batch[i].y, batch[i].z));
        for (size_t i = 0; i < batch.size(); i++) {
            TickNextTickData& td = batch[i];
            if (worldBlock(w, td.x, td.y, td.z) == td.tileId)
                tickLiquid(w, td.x, td.y, td.z, td.tileId);
        }
    }
}

static void scheduleFire(World* w, int x, int y, int z) {
    worldScheduleTick(w, x, y, z, BLOCK_FIRE, FIRE_TICK_DELAY + rand() % 10);
}

void worldScheduleChunkTicks(World* w, int cx, int cz) {

    bool hasTick[N_SECTIONS];
    for (int si = 0; si < N_SECTIONS; si++) {
        unsigned char u;
        hasTick[si] = !blockSectionUniform(w, cx * 16, si * SECTION_SY, cz * 16, &u)
                      || isLiquidId(u) || u == BLOCK_FIRE
                      || u == BLOCK_ORE_REDSTONE_LIT;
    }
    for (int lx = 0; lx < 16; lx++)
    for (int lz = 0; lz < 16; lz++) {
        int x = cx * 16 + lx, z = cz * 16 + lz;
        for (int y = 0; y < WORLD_H; y++) {
            if (!hasTick[y >> 4]) { y |= (SECTION_SY - 1); continue; }
            unsigned char id = worldBlock(w, x, y, z);
            if (id == BLOCK_FIRE) { scheduleFire(w, x, y, z); continue; }
            if (id == BLOCK_ORE_REDSTONE_LIT) { worldScheduleTick(w, x, y, z, id, REDSTONE_LIT_DELAY); continue; }
            if (!isLiquidId(id)) continue;
            if (worldData(w, x, y, z) == 0) continue;
            unsigned char dyn = dynOf(id);
            if (id != dyn) blockPut(w, x, y, z, dyn);
            worldScheduleTick(w, x, y, z, dyn, isWaterId(dyn) ? 5 : 30);
        }
    }
}

void worldScheduleLoadedTicks(World* w) {
    for (int x = 0; x < WORLD_W; x++)
    for (int z = 0; z < WORLD_D; z++)
    for (int y = 0; y < WORLD_H; y++) {
        unsigned char id = worldBlock(w, x, y, z);
        if (id == BLOCK_FIRE) { scheduleFire(w, x, y, z); continue; }
        if (id == BLOCK_ORE_REDSTONE_LIT) { worldScheduleTick(w, x, y, z, id, REDSTONE_LIT_DELAY); continue; }
        if (!isLiquidId(id)) continue;
        if (worldData(w, x, y, z) == 0) continue;
        unsigned char dyn = dynOf(id);
        if (id != dyn) blockPut(w, x, y, z, dyn);
        worldScheduleTick(w, x, y, z, dyn, isWaterId(dyn) ? 5 : 30);
    }
}

void worldTick(World* w) {
    w->time++;

    if (worldNightModeTick(w)) {}
    else if (!g_level.player->inventory->isCreative()) w->dayTime++;
    else if (w->dayTime != CREATIVE_STOP_TIME) w->dayTime = CREATIVE_STOP_TIME;
    worldUpdateSkyDarken(w);

    profBegin(PROF_TRAND);
    tileRandomTick(w);
    profEnd(PROF_TRAND);

    profAdd(PROFC_PENDLIST, (int)w->tickNextTickList.size());
    if (w->tickNextTickList.empty()) return;

    const unsigned int TICK_BUDGET_US = 4000;

    static size_t cursor = 0;
    profBegin(PROF_TPEND);
    unsigned int tStart = sceKernelGetSystemTimeLow();
    size_t pass = w->tickNextTickList.size();
    for (size_t scanned = 0; scanned < pass; scanned++) {
        if (cursor >= w->tickNextTickList.size()) cursor = 0;
        if (w->tickNextTickList.empty()) break;
        TickNextTickData td = w->tickNextTickList[cursor];
        if (td.delay > w->time) { cursor++; continue; }

        w->tickNextTickList[cursor] = w->tickNextTickList.back();
        w->tickNextTickList.pop_back();

        w->tickSet.erase((unsigned int)worldIndex(w, td.x, td.y, td.z));

        unsigned char currentId = worldBlock(w, td.x, td.y, td.z);
        if (currentId == td.tileId) {
            if (isLeaf(currentId))         leafDecayTick(w, td.x, td.y, td.z);
            else if (isHeavyTile(currentId)) heavyTileTick(w, td.x, td.y, td.z, currentId);
            else if (currentId == BLOCK_ORE_REDSTONE_LIT) redstoneOreRevert(w, td.x, td.y, td.z);
            else if (currentId == BLOCK_FIRE) fireTileTick(w, td.x, td.y, td.z);
            else                           tickLiquid(w, td.x, td.y, td.z, currentId);
        }
        if (sceKernelGetSystemTimeLow() - tStart >= TICK_BUDGET_US) break;
    }
    profEnd(PROF_TPEND);
}
