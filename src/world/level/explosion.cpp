
#include "world/level/world.h"
#include "world/level/level.h"
#include "world/entity/local_player.h"
#include "world/level/chunk/chunk.h"
#include "world/level/level.h"
#include "world/entity/primed_tnt.h"
#include "world/phys/aabb.h"
#include "client/player/player_state.h"
#include "client/renderer/particle.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

extern Level g_level;

static inline float frand() { return (float)rand() / (float)RAND_MAX; }

static float explosionResistance(unsigned char id) {
    if (id == BLOCK_AIR) return 0.0f;
    if (id == BLOCK_BEDROCK) return 1e7f;
    if (id == BLOCK_OBSIDIAN) return 6000.0f;
    if (id == BLOCK_GLOWING_OBSIDIAN) return 2000.0f;
    if (isLiquidId(id)) return 100.0f;
    return 0.0f;
}

static bool rayBlocked(World* w, float ax, float ay, float az, float bx, float by, float bz) {
    float dx = bx - ax, dy = by - ay, dz = bz - az;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 1e-4f) return false;
    int steps = (int)(len / 0.3f) + 1;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        if (isSolidPhys(worldBlock(w, (int)floorf(ax + dx * t), (int)floorf(ay + dy * t), (int)floorf(az + dz * t))))
            return true;
    }
    return false;
}

static float seenPercent(World* w, float cx, float cy, float cz, const AABB& bb) {
    float xs = 1.0f / ((bb.x1 - bb.x0) * 2 + 1);
    float ys = 1.0f / ((bb.y1 - bb.y0) * 2 + 1);
    float zs = 1.0f / ((bb.z1 - bb.z0) * 2 + 1);
    int hits = 0, count = 0;
    for (float xx = 0; xx <= 1.0f; xx += xs)
    for (float yy = 0; yy <= 1.0f; yy += ys)
    for (float zz = 0; zz <= 1.0f; zz += zs) {
        float px = bb.x0 + (bb.x1 - bb.x0) * xx;
        float py = bb.y0 + (bb.y1 - bb.y0) * yy;
        float pz = bb.z0 + (bb.z1 - bb.z0) * zz;
        if (!rayBlocked(w, px, py, pz, cx, cy, cz)) hits++;
        count++;
    }
    return count ? hits / (float)count : 1.0f;
}

void worldExplode(World* w, float x, float y, float z, float r) {
    const int cx = (int)floorf(x), cy = (int)floorf(y), cz = (int)floorf(z);

    const int R = 10, W = 2 * R + 1;
    static unsigned char visited[21 * 21 * 21];
    memset(visited, 0, sizeof(visited));
    #define VIS(lx, ly, lz) visited[((lx) * W + (ly)) * W + (lz)]

    const int size = 16;
    const float stepSize = 0.3f;
    for (int xx = 0; xx < size; xx++)
    for (int yy = 0; yy < size; yy++)
    for (int zz = 0; zz < size; zz++) {
        if ((xx != 0 && xx != size - 1) && (yy != 0 && yy != size - 1) && (zz != 0 && zz != size - 1)) continue;
        float dx = xx / (size - 1.0f) * 2 - 1;
        float dy = yy / (size - 1.0f) * 2 - 1;
        float dz = zz / (size - 1.0f) * 2 - 1;
        float d = sqrtf(dx * dx + dy * dy + dz * dz);
        if (d < 1e-6f) continue;
        dx /= d; dy /= d; dz /= d;

        float power = r * (0.7f + frand() * 0.6f);
        float xp = x, yp = y, zp = z;
        while (power > 0.0f) {
            int xt = (int)floorf(xp), yt = (int)floorf(yp), zt = (int)floorf(zp);
            unsigned char t = worldBlock(w, xt, yt, zt);
            if (t != BLOCK_AIR) power -= (explosionResistance(t) + 0.3f) * stepSize;
            if (power > 0.0f) {
                int lx = xt - cx + R, ly = yt - cy + R, lz = zt - cz + R;
                if (lx >= 0 && lx < W && ly >= 0 && ly < W && lz >= 0 && lz < W) VIS(lx, ly, lz) = 1;
            }
            xp += dx * stepSize; yp += dy * stepSize; zp += dz * stepSize;
            power -= stepSize * 0.75f;
        }
    }

    for (int lx = 0; lx < W; lx++)
    for (int ly = 0; ly < W; ly++)
    for (int lz = 0; lz < W; lz++) {
        if (!VIS(lx, ly, lz)) continue;
        int bx = cx - R + lx, by = cy - R + ly, bz = cz - R + lz;
        unsigned char id = worldBlock(w, bx, by, bz);
        if (id == BLOCK_AIR) continue;
        if (id == BLOCK_TNT) {
            worldPrimeTnt(w, bx, by, bz, rand() % 20 + 10);
        } else {

            if (frand() < 0.3f)
                worldSpawnResources(w, bx, by, bz, id, worldData(w, bx, by, bz));
            worldSetBlockAndData(w, bx, by, bz, BLOCK_AIR, 0);
            worldNotifyNeighborsChanged(w, bx, by, bz);
        }
    }
    worldUpdateLights(w);

    const float r2 = r * 2.0f;
    for (size_t i = 0; i < g_level.entities.size(); i++) {
        Entity* e = g_level.entities[i];
        if (!e || e->removed) continue;
        float dx = e->x - x, dy = e->y - y, dz = e->z - z;
        float dd = sqrtf(dx * dx + dy * dy + dz * dz);
        float dist = dd / r2;
        if (dist > 1.0f) continue;
        float inv = dd > 1e-4f ? 1.0f / dd : 0.0f;
        dx *= inv; dy *= inv; dz *= inv;
        float seen = seenPercent(w, x, y, z, e->bb);
        float pw = (1.0f - dist) * seen;
        e->hurt(0, (int)((pw * pw + pw) / 2.0f * 8.0f * r + 1.0f));
        e->xd += dx * pw; e->yd += dy * pw; e->zd += dz * pw;
    }

    {
        float dx = g_level.player->x - x, dy = g_level.player->y - y, dz = g_level.player->z - z;
        float dd = sqrtf(dx * dx + dy * dy + dz * dz);
        float dist = dd / r2;
        if (dist <= 1.0f) {
            float feet = g_level.player->y - PLAYER_EYE;
            AABB pbb(g_level.player->x - PLAYER_W * 0.5f, feet, g_level.player->z - PLAYER_W * 0.5f,
                     g_level.player->x + PLAYER_W * 0.5f, feet + PLAYER_H, g_level.player->z + PLAYER_W * 0.5f);
            float seen = seenPercent(w, x, y, z, pbb);
            float inv = dd > 1e-4f ? 1.0f / dd : 0.0f;
            float pw = (1.0f - dist) * seen;
            g_level.player->hurt(0, (int)((pw * pw + pw) / 2.0f * 8.0f * r + 1.0f));
            g_level.player->xd += dx * inv * pw; g_level.player->yd += dy * inv * pw; g_level.player->zd += dz * inv * pw;
        }
    }

    particlesExplosion(w, x, y, z);

    {
        float r1 = rand() / (float)RAND_MAX, r2 = rand() / (float)RAND_MAX;
        g_level.playSound(x, y, z, "random.explode", 4.0f, (1.0f + (r1 - r2) * 0.2f) * 0.7f);
    }
}

void worldPrimeTnt(World* w, int x, int y, int z, int fuseTicks) {
    if (worldBlock(w, x, y, z) != BLOCK_TNT) return;
    worldSetBlockAndData(w, x, y, z, BLOCK_AIR, 0);
    worldNotifyNeighborsChanged(w, x, y, z);
    worldUpdateLights(w);
    worldRebuildAroundNow(w, x, y, z);
    g_level.addEntity(new PrimedTnt(&g_level, x + 0.5f, y + 0.5f, z + 0.5f, fuseTicks));
    g_level.playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.fuse", 1.0f, 1.0f);
}
