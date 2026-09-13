#include <pspkernel.h>

#include "world/level/world.h"

#include <string.h>
#include <pspkernel.h>

static inline bool isSkyLitAt(const World* w, int x, int y, int z) {
    if (y >= WORLD_H) return true;
    if (y < 0 || !worldReady(w, x, z)) return false;
    return y >= w->heightmap[worldColumn(w, x, z)];
}

static const signed char kLN[6][3] = {{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};

static const signed char kHN[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};

static inline unsigned int lpack(const World* w, int x, int y, int z) {
    return ((unsigned)worldSlotIndex(w, x >> 4, z >> 4) << 15)
         | ((unsigned)(x & 15) << 11) | ((unsigned)(z & 15) << 7) | (unsigned)y;
}
static inline void lunpack(const World* w, unsigned int p, int* x, int* y, int* z) {
    const LevelChunk* c = &w->slots[(p >> 15) & 0xFF];
    *x = (c->x << 4) | (int)((p >> 11) & 15);
    *z = (c->z << 4) | (int)((p >> 7) & 15);
    *y = (int)(p & 0x7F);
}
static std::vector<unsigned int> g_lightBfs;

#define LIGHT_BFS_RESERVE   32768
#define LIGHT_QUEUE_RESERVE 32768

static inline unsigned int qpack(const World* w, int layer, int x, int y, int z) {
    return ((unsigned)layer << 23) | lpack(w, x, y, z);
}

static bool g_floodMarks = false;

static bool g_lightYield = true;
static inline void floodMark(World* w, int x, int y, int z) {
    if (!g_floodMarks) return;
    int cx = x >> 4, cz = z >> 4;
    if (!worldChunkReady(w, cx, cz)) return;
    worldMesh(w, cx, cz)->sec[y >> 4].dirty = true;
}

static void lightFlood(World* w, int layer) {
    size_t head = 0;
    while (head < g_lightBfs.size()) {

        if (g_lightYield && (head & 1023) == 0) sceKernelDelayThread(100);

        if (head >= 65536 && head * 2 >= g_lightBfs.size()) {
            g_lightBfs.erase(g_lightBfs.begin(), g_lightBfs.begin() + head);
            head = 0;
        }
        unsigned int p = g_lightBfs[head++];
        int x, y, z; lunpack(w, p, &x, &y, &z);
        int cur = (layer == 0) ? lightSkyGet(w, x, y, z) : lightBlockGet(w, x, y, z);
        if (cur <= 1) continue;
        for (int d = 0; d < 6; d++) {
            int nx = x + kLN[d][0], ny = y + kLN[d][1], nz = z + kLN[d][2];
            if (ny < 0 || ny >= WORLD_H || !worldReady(w, nx, nz)) continue;
            int op = lightOpacity(worldBlock(w, nx, ny, nz)); if (op < 1) op = 1;
            int nl = cur - op;
            if (nl <= 0) continue;
            int have = (layer == 0) ? lightSkyGet(w, nx, ny, nz) : lightBlockGet(w, nx, ny, nz);
            if (have < nl) {
                if (layer == 0) lightSkySet(w, nx, ny, nz, nl); else lightBlockSet(w, nx, ny, nz, nl);
                floodMark(w, nx, ny, nz);
                g_lightBfs.push_back(lpack(w, nx, ny, nz));
            }
        }
    }
    g_lightBfs.clear();
}

void worldRecalcHeightmap(World* w) {
    for (int x = 0; x < WORLD_W; x++) {
        if ((x & 31) == 0) sceKernelDelayThread(100);
        for (int z = 0; z < WORLD_D; z++) {
            int hy = 0;
            for (int y = WORLD_H - 1; y >= 0; y--) {
                if (lightOpacity(worldBlock(w, x, y, z)) > 0) { hy = y + 1; break; }
            }
            w->heightmap[worldColumn(w, x, z)] = (unsigned char)hy;
        }
    }
}

void lightQueuesReserve(World* w) {
    g_lightBfs.reserve(LIGHT_BFS_RESERVE);
    w->lightQueue.reserve(LIGHT_QUEUE_RESERVE);
}

static void lightBfsShrink() {
    std::vector<unsigned int>().swap(g_lightBfs);
    g_lightBfs.reserve(LIGHT_BFS_RESERVE);
}

void worldInitLight(World* w) {
    lightClearAll(w);

    worldRecalcHeightmap(w);
    lightInitSkyFromHeightmap(w);

    g_lightBfs.clear();
    for (int x = 0; x < WORLD_W; x++) {
        if ((x & 31) == 0) {
            sceKernelDelayThread(100);
            g_terrainProgress = 60 + (x * 10) / WORLD_W;
        }
        for (int z = 0; z < WORLD_D; z++)
        for (int y = 0; y < WORLD_H; y++) {
            int s = lightSkyGet(w, x, y, z);
            if (s <= 1) continue;
            for (int d = 0; d < 6; d++) {
                int nx = x + kLN[d][0], ny = y + kLN[d][1], nz = z + kLN[d][2];
                if (ny < 0 || ny >= WORLD_H || !worldReady(w, nx, nz)) continue;
                if (lightOpacity(worldBlock(w, nx, ny, nz)) >= 15) continue;
                if (lightSkyGet(w, nx, ny, nz) < s - 1) { g_lightBfs.push_back(lpack(w, x, y, z)); break; }
            }
        }

        if (g_lightBfs.size() >= 65536) lightFlood(w, 0);
    }
    lightFlood(w, 0);
    g_terrainProgress = 80;

    g_lightBfs.clear();
    for (int x = 0; x < WORLD_W; x++) {
        if ((x & 31) == 0) {
            sceKernelDelayThread(100);
            g_terrainProgress = 80 + (x * 5) / WORLD_W;
        }
        for (int z = 0; z < WORLD_D; z++)
        for (int y = 0; y < WORLD_H; y++) {
            int e = lightEmit(worldBlock(w, x, y, z));
            if (e > 0) { lightBlockSet(w, x, y, z, e); g_lightBfs.push_back(lpack(w, x, y, z)); }
        }
    }
    lightFlood(w, 1);

    lightBfsShrink();

    lightCompactAll(w);
}

void worldRecalcChunkHeightmap(World* w, int cx, int cz) {
    for (int lx = 0; lx < 16; lx++)
        for (int lz = 0; lz < 16; lz++) {
            int x = cx * 16 + lx, z = cz * 16 + lz, hy = 0;
            for (int y = WORLD_H - 1; y >= 0; y--)
                if (lightOpacity(worldBlock(w, x, y, z)) > 0) { hy = y + 1; break; }
            w->heightmap[worldColumn(w, x, z)] = (unsigned char)hy;
        }
}

void worldInitChunkLight(World* w, int cx, int cz) {
    const int x0 = cx * 16, z0 = cz * 16;

    g_floodMarks = true;
    g_lightYield = false;
    worldRecalcChunkHeightmap(w, cx, cz);

    lightInitSkyChunk(w, cx, cz);

    lightChunkClearBlock(w, cx, cz);

    g_lightBfs.clear();
    for (int lx = 0; lx < 16; lx++)
        for (int lz = 0; lz < 16; lz++) {
            int x = x0 + lx, z = z0 + lz;
            int h = w->heightmap[worldColumn(w, x, z)];
            int top = h;
            for (int d = 0; d < 4; d++) {
                int nx = x + kHN[d][0], nz = z + kHN[d][1];
                if (!worldReady(w, nx, nz)) continue;
                int nh = w->heightmap[worldColumn(w, nx, nz)];
                if (nh > top) top = nh;
            }

            if (h > 0 && lightOpacity(worldBlock(w, x, h - 1, z)) < 15 && h < WORLD_H)
                g_lightBfs.push_back(lpack(w, x, h, z));
            for (int y = h; y < top && y < WORLD_H; y++)
                g_lightBfs.push_back(lpack(w, x, y, z));
        }

    for (int e = 0; e < 4; e++)
        for (int k = 0; k < 16; k++) {
            int lx, lz, ix, iz;
            switch (e) {
                case 0: lx = -1; lz = k;  ix = 0;  iz = k;  break;
                case 1: lx = 16; lz = k;  ix = 15; iz = k;  break;
                case 2: lx = k;  lz = -1; ix = k;  iz = 0;  break;
                default: lx = k; lz = 16; ix = k;  iz = 15; break;
            }
            int x = x0 + lx, z = z0 + lz;
            if (!worldReady(w, x, z)) continue;
            int ih = w->heightmap[worldColumn(w, x0 + ix, z0 + iz)];
            for (int y = 0; y < ih && y < WORLD_H; y++)
                if (lightSkyGet(w, x, y, z) > 1) g_lightBfs.push_back(lpack(w, x, y, z));
        }
    lightFlood(w, 0);

    g_lightBfs.clear();
    for (int lx = 0; lx < 16; lx++)
        for (int lz = 0; lz < 16; lz++) {
            int x = x0 + lx, z = z0 + lz;
            for (int y = 0; y < WORLD_H; y++) {
                int e = lightEmit(worldBlock(w, x, y, z));
                if (e > 0) { lightBlockSet(w, x, y, z, e); g_lightBfs.push_back(lpack(w, x, y, z)); }
            }
        }

    for (int e = 0; e < 4; e++) {
        int lx = (e == 0) ? -1 : (e == 1) ? 16 : 0;
        int lz = (e == 2) ? -1 : (e == 3) ? 16 : 0;
        int x = x0 + lx, z = z0 + lz;
        if (!worldReady(w, x, z)) continue;
        for (int y = 0; y < WORLD_H; y++) {
            if (lightPlaneAllDark(w, 1, x, y, z)) continue;
            for (int k = 0; k < 16; k++) {
                int cxx = (e < 2) ? x : x0 + k;
                int czz = (e < 2) ? z0 + k : z;
                if (lightBlockGet(w, cxx, y, czz) > 1) g_lightBfs.push_back(lpack(w, cxx, y, czz));
            }
        }
    }
    lightFlood(w, 1);
    g_floodMarks = false;
    g_lightYield = true;
}

unsigned int g_lightQueueDrops = 0;
static void lightEnqueue(World* w, int layer, int x, int y, int z) {

    if (w->lightQueue.size() >= LIGHT_QUEUE_RESERVE) { g_lightQueueDrops++; return; }
    w->lightQueue.push_back(qpack(w, layer, x, y, z));
}

static void lightUpdateIfOther(World* w, int layer, int x, int y, int z, int expected) {
    if (y < 0 || y >= WORLD_H || !worldReady(w, x, z)) return;
    if (layer == 0) { if (isSkyLitAt(w, x, y, z)) expected = 15; }
    else { int e = lightEmit(worldBlock(w, x, y, z)); if (e > expected) expected = e; }
    int have = (layer == 0) ? lightSkyGet(w, x, y, z) : lightBlockGet(w, x, y, z);
    if (have != expected) lightEnqueue(w, layer, x, y, z);
}

static void lightUpdateCell(World* w, int layer, int x, int y, int z) {
    int old = (layer == 0) ? lightSkyGet(w, x, y, z) : lightBlockGet(w, x, y, z);
    unsigned char tile = worldBlock(w, x, y, z);
    int opac = lightOpacity(tile); if (opac == 0) opac = 1;
    int emit = 0;
    if (layer == 0) { if (isSkyLitAt(w, x, y, z)) emit = 15; }
    else emit = lightEmit(tile);

    int target;
    if (opac >= 15 && emit == 0) target = 0;
    else {
        int m = 0;
        for (int d = 0; d < 6; d++) {
            int nx = x + kLN[d][0], ny = y + kLN[d][1], nz = z + kLN[d][2];
            int v = (layer == 0) ? lightSkyGet(w, nx, ny, nz) : lightBlockGet(w, nx, ny, nz);
            if (v > m) m = v;
        }
        target = m - opac; if (target < 0) target = 0;
        if (emit > target) target = emit;
    }

    if (old != target) {
        if (layer == 0) lightSkySet(w, x, y, z, target); else lightBlockSet(w, x, y, z, target);
        worldMarkDirty(w, x, y, z);
        int t = target - 1; if (t < 0) t = 0;
        for (int d = 0; d < 6; d++)
            lightUpdateIfOther(w, layer, x + kLN[d][0], y + kLN[d][1], z + kLN[d][2], t);
    }
}

static bool lightDrain(World* w, unsigned int budgetUs) {
    unsigned int tStart = sceKernelGetSystemTimeLow();
    int steps = 0;
    while (!w->lightQueue.empty()) {
        unsigned int q = w->lightQueue.back();
        w->lightQueue.pop_back();
        int x, y, z; lunpack(w, q, &x, &y, &z);
        lightUpdateCell(w, (int)(q >> 23) & 1, x, y, z);

        if ((++steps & 63) == 0 && sceKernelGetSystemTimeLow() - tStart >= budgetUs) break;
    }
    return w->lightQueue.empty();
}

void worldUpdateLights(World* w) {
    static const unsigned int TIME_BUDGET_US = 1500;
    lightDrain(w, TIME_BUDGET_US);
}

void worldRemoveBlockLight(World* w, int x, int y, int z) {
    struct RN { int x, y, z, lvl; };
    static std::vector<RN> rem;
    rem.clear();
    g_lightBfs.clear();

    int start = lightBlockGet(w, x, y, z);
    if (start <= 0) return;
    lightBlockSet(w, x, y, z, 0);
    worldMarkDirty(w, x, y, z);
    rem.push_back({ x, y, z, start });

    size_t head = 0;
    while (head < rem.size()) {
        RN n = rem[head++];
        for (int d = 0; d < 6; d++) {
            int nx = n.x + kLN[d][0], ny = n.y + kLN[d][1], nz = n.z + kLN[d][2];
            if (ny < 0 || ny >= WORLD_H || !worldReady(w, nx, nz)) continue;
            int nl = lightBlockGet(w, nx, ny, nz);
            if (nl == 0) continue;
            if (nl < n.lvl) {

                int e = lightEmit(worldBlock(w, nx, ny, nz));
                lightBlockSet(w, nx, ny, nz, 0);
                worldMarkDirty(w, nx, ny, nz);
                if (e > 0) { lightBlockSet(w, nx, ny, nz, e); g_lightBfs.push_back(lpack(w, nx, ny, nz)); }
                else rem.push_back({ nx, ny, nz, nl });
            } else {

                g_lightBfs.push_back(lpack(w, nx, ny, nz));
            }
        }
    }
    lightFlood(w, 1);
}

void worldRelightBox(World* w, int x0, int y0, int z0, int x1, int y1, int z1) {
    if (!w->lightReady) return;
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (y0 < 0) y0 = 0;
    if (y1 >= WORLD_H) y1 = WORLD_H - 1;

    for (int x = x0; x <= x1; x++)
        for (int z = z0; z <= z1; z++) {
            if (!worldReady(w, x, z)) continue;
            const int col  = worldColumn(w, x, z);
            const int oldH = w->heightmap[col];
            int newH = 0;
            for (int y = WORLD_H - 1; y >= 0; y--)
                if (lightOpacity(worldBlock(w, x, y, z)) > 0) { newH = y + 1; break; }
            w->heightmap[col] = (unsigned char)newH;

            const int lo = oldH < newH ? oldH : newH;
            const int hi = oldH < newH ? newH : oldH;
            for (int y = lo; y < hi; y++) lightEnqueue(w, 0, x, y, z);

            for (int y = y0; y <= y1; y++) lightEnqueue(w, 1, x, y, z);
        }

    lightDrain(w, 150000);
}

void lightOnBlockChanged(World* w, int x, int y, int z) {

    int oldH = w->heightmap[worldColumn(w, x, z)];
    int newH = 0;
    for (int yy = WORLD_H - 1; yy >= 0; yy--) {
        if (lightOpacity(worldBlock(w, x, yy, z)) > 0) { newH = yy + 1; break; }
    }
    w->heightmap[worldColumn(w, x, z)] = (unsigned char)newH;

    int lo = y, hi = y + 1;
    if (oldH < lo) lo = oldH;
    if (newH < lo) lo = newH;
    if (oldH > hi) hi = oldH;
    if (newH > hi) hi = newH;
    if (lo < 0) lo = 0;
    if (hi > WORLD_H) hi = WORLD_H;
    for (int yy = lo; yy < hi; yy++) lightEnqueue(w, 0, x, yy, z);
    lightEnqueue(w, 1, x, y, z);
}
