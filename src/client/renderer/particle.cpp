#include "client/renderer/particle.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "gpu/texture.h"

#include <pspgu.h>
#include <pspgum.h>
#include <cmath>
#include <cstdlib>

void tileForBlock(unsigned char id, unsigned char data, int f, int* col, int* row, unsigned int* tint);

extern unsigned int g_brightColor[16];

namespace {

enum Kind { K_TERRAIN, K_FLAME, K_SMOKE, K_BUBBLE, K_REDDUST, K_EXPLODE, K_LAVA, K_CRIT, K_SPLASH, K_HEART };

struct P {
    float x, y, z, xo, yo, zo, xd, yd, zd;
    float size, oSize, gravity;
    float r, g, b;
    int   age, lifetime;
    int   tex;
    float uo, vo;
    unsigned char kind;
    bool  active, onGround, noPhysics, terrainAtlas, fullBright;
    bool  itemsAtlas;
};

const int MAX_PARTICLES = 384;
P g_pool[MAX_PARTICLES];

inline float frand() { return (float)rand() / (float)RAND_MAX; }

P* alloc() {
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (!g_pool[i].active) { g_pool[i] = P(); g_pool[i].active = true; return &g_pool[i]; }
    return 0;
}

void baseInit(P* p, float x, float y, float z, float xa, float ya, float za) {
    p->x = p->xo = x; p->y = p->yo = y; p->z = p->zo = z;
    p->r = p->g = p->b = 1.0f;
    p->size = (frand() * 0.5f + 0.5f) * 2.0f;
    p->gravity = 0.0f;
    p->age = 0;
    p->uo = frand() * 3.0f; p->vo = frand() * 3.0f;
    p->lifetime = (int)(4.0f / (frand() * 0.9f + 0.1f));
    p->xd = xa + (frand() * 2 - 1) * 0.4f;
    p->yd = ya + (frand() * 2 - 1) * 0.4f;
    p->zd = za + (frand() * 2 - 1) * 0.4f;
    float speed = (frand() + frand() + 1) * 0.15f;
    float dd = sqrtf(p->xd * p->xd + p->yd * p->yd + p->zd * p->zd);
    if (dd < 1e-4f) dd = 1e-4f;
    float mul = 0.4f * speed / dd;
    p->xd = p->xd * mul;
    p->yd = p->yd * mul + 0.1f;
    p->zd = p->zd * mul;
}

void move(World* w, P* p) {
    if (p->noPhysics) { p->x += p->xd; p->y += p->yd; p->z += p->zd; p->onGround = false; return; }
    float nx = p->x + p->xd;
    if (isSolidPhys(worldBlock(w, (int)floorf(nx), (int)floorf(p->y), (int)floorf(p->z)))) p->xd = 0;
    else p->x = nx;
    float nz = p->z + p->zd;
    if (isSolidPhys(worldBlock(w, (int)floorf(p->x), (int)floorf(p->y), (int)floorf(nz)))) p->zd = 0;
    else p->z = nz;
    float ny = p->y + p->yd;
    if (isSolidPhys(worldBlock(w, (int)floorf(p->x), (int)floorf(ny), (int)floorf(p->z)))) {
        p->onGround = (p->yd < 0); p->yd = 0;
    } else { p->y = ny; p->onGround = false; }
}

void tickOne(World* w, P* p) {
    p->xo = p->x; p->yo = p->y; p->zo = p->z;
    if (p->age++ >= p->lifetime) { p->active = false; return; }

    switch (p->kind) {
    case K_FLAME:
        move(w, p);
        p->xd *= 0.96f; p->yd *= 0.96f; p->zd *= 0.96f;
        break;
    case K_SMOKE:
        p->tex = 7 - p->age * 8 / p->lifetime;
        p->yd += 0.004f;
        move(w, p);
        if (p->y == p->yo) { p->xd *= 1.1f; p->zd *= 1.1f; }
        p->xd *= 0.96f; p->yd *= 0.96f; p->zd *= 0.96f;
        break;
    case K_REDDUST:
        p->tex = 7 - p->age * 8 / p->lifetime;
        move(w, p);
        if (p->y == p->yo) { p->xd *= 1.1f; p->zd *= 1.1f; }
        p->xd *= 0.96f; p->yd *= 0.96f; p->zd *= 0.96f;
        break;
    case K_EXPLODE:
        p->tex = 7 - p->age * 8 / p->lifetime;
        p->yd += 0.004f;
        move(w, p);
        p->xd *= 0.90f; p->yd *= 0.90f; p->zd *= 0.90f;
        break;
    case K_LAVA: {
        float odds = p->age / (float)p->lifetime;
        if (frand() > odds) {
            P* s = alloc();
            if (s) { baseInit(s, p->x, p->y, p->z, p->xd, p->yd, p->zd);
                s->kind = K_SMOKE; s->r = s->g = s->b = frand() * 0.5f;
                s->size *= 0.75f; s->oSize = s->size;
                s->lifetime = (int)(8.0f / (frand() * 0.8f + 0.2f)); }
        }
        p->yd -= 0.03f;
        move(w, p);
        p->xd *= 0.999f; p->yd *= 0.999f; p->zd *= 0.999f;
        break;
    }
    case K_BUBBLE:
        p->yd += 0.002f;
        move(w, p);
        p->xd *= 0.85f; p->yd *= 0.85f; p->zd *= 0.85f;
        if (!isWaterId(worldBlock(w, (int)floorf(p->x), (int)floorf(p->y), (int)floorf(p->z))))
            p->active = false;
        break;
    case K_HEART:
        move(w, p);
        if (p->y == p->yo) { p->xd *= 1.1f; p->zd *= 1.1f; }
        p->xd *= 0.86f; p->yd *= 0.86f; p->zd *= 0.86f;
        break;
    case K_CRIT:
        move(w, p);
        p->g *= 0.96f; p->b *= 0.9f;
        p->xd *= 0.70f; p->yd *= 0.70f; p->zd *= 0.70f;
        p->yd -= 0.02f;
        break;
    case K_SPLASH: {
        p->yd -= 0.04f * p->gravity;
        move(w, p);
        p->xd *= 0.98f; p->yd *= 0.98f; p->zd *= 0.98f;
        int by = (int)floorf(p->y);
        if (isWaterId(worldBlock(w, (int)floorf(p->x), by, (int)floorf(p->z)))) {

            float y0 = by + 1.0f - (1.0f / 9.0f);
            if (p->y < y0) p->active = false;
        }
        break;
    }
    default:
        p->yd -= 0.04f * p->gravity;
        move(w, p);
        p->xd *= 0.98f; p->yd *= 0.98f; p->zd *= 0.98f;
        break;
    }
    if (p->onGround) { p->xd *= 0.7f; p->zd *= 0.7f; }
}

}

void particlesReset() {
    for (int i = 0; i < MAX_PARTICLES; i++) g_pool[i].active = false;
}

inline void rotX(float& x, float& y, float& z, float a) {
    float c = cosf(a), s = sinf(a); float ny = y * c + z * s, nz = z * c - y * s; (void)x; y = ny; z = nz;
}
inline void rotY(float& x, float& y, float& z, float a) {
    float c = cosf(a), s = sinf(a); float nx = x * c + z * s, nz = z * c - x * s; (void)y; x = nx; z = nz;
}

void particlesDestroyBlock(World* w, int x, int y, int z, unsigned char id, unsigned char data) {
    if (id == BLOCK_AIR) return;
    int col = 1, row = 0; unsigned int tint = 0xFFFFFFFFu;
    tileForBlock(id, data, 1 , &col, &row, &tint);
    int tex = row * 16 + col;

    float tr = 0.6f * ((tint) & 0xff) / 255.0f;
    float tg = 0.6f * ((tint >> 8) & 0xff) / 255.0f;
    float tb = 0.6f * ((tint >> 16) & 0xff) / 255.0f;
    const int SD = 3;
    for (int xx = 0; xx < SD; xx++)
    for (int yy = 1; yy < SD; yy++)
    for (int zz = 0; zz < SD; zz++) {
        float xp = x + (xx + 0.5f) / SD, yp = y + (yy + 0.5f) / SD, zp = z + (zz + 0.5f) / SD;
        P* p = alloc(); if (!p) return;
        baseInit(p, xp, yp, zp, 2 * (xp - x - 0.5f), 2 * (yp - y - 0.5f), 2 * (zp - z - 0.5f));
        p->kind = K_TERRAIN; p->terrainAtlas = true;
        p->tex = tex; p->gravity = 1.0f; p->size /= 2.0f;
        p->r = tr; p->g = tg; p->b = tb;
    }
}

void particlesCrackHit(World* w, int x, int y, int z, unsigned char id, unsigned char data, int face) {
    (void)w;
    if (id == BLOCK_AIR) return;
    int col = 1, row = 0; unsigned int tint = 0xFFFFFFFFu;
    tileForBlock(id, data, face, &col, &row, &tint);
    int tex = row * 16 + col;
    float tr = 0.6f * ((tint) & 0xff) / 255.0f;
    float tg = 0.6f * ((tint >> 8) & 0xff) / 255.0f;
    float tb = 0.6f * ((tint >> 16) & 0xff) / 255.0f;
    if (face < 0 || face > 5) face = F_TOP;
    const signed char* nrm = kFaceNeighbor[face];

    const float r = 0.1f;
    float xp = x + (nrm[0] ? (nrm[0] > 0 ? 1.0f + r : -r) : r + (1.0f - 2 * r) * frand());
    float yp = y + (nrm[1] ? (nrm[1] > 0 ? 1.0f + r : -r) : r + (1.0f - 2 * r) * frand());
    float zp = z + (nrm[2] ? (nrm[2] > 0 ? 1.0f + r : -r) : r + (1.0f - 2 * r) * frand());
    P* p = alloc(); if (!p) return;
    baseInit(p, xp, yp, zp, 0, 0, 0);
    p->xd *= 0.2f; p->yd = (p->yd - 0.1f) * 0.2f + 0.1f; p->zd *= 0.2f;
    p->kind = K_TERRAIN; p->terrainAtlas = true;
    p->tex = tex; p->gravity = 1.0f; p->size *= 0.6f;
    p->r = tr; p->g = tg; p->b = tb;
}

void particlesEat(float px, float py, float pz, float yawDeg, float pitchDeg,
                  int iconCell, int count) {
    const float D2R = 3.14159265f / 180.0f;

    float xx = pitchDeg * D2R, yy = yawDeg * D2R;
    for (int i = 0; i < count; i++) {

        float dx = (frand() - 0.5f) * 0.1f, dy = frand() * 0.1f + 0.1f, dz = 0.0f;
        rotX(dx, dy, dz, xx); rotY(dx, dy, dz, yy);

        float ox = (frand() - 0.5f) * 0.3f, oy = -frand() * 0.6f - 0.3f, oz = 0.6f;
        rotX(ox, oy, oz, xx); rotY(ox, oy, oz, yy);
        P* p = alloc(); if (!p) return;
        baseInit(p, px + ox, py + oy, pz + oz, 0, 0, 0);

        p->xd = dx; p->yd = dy + 0.05f; p->zd = dz;
        p->kind = K_TERRAIN;
        p->itemsAtlas = true;
        p->tex = iconCell;
        p->gravity = 1.0f;
        p->size /= 2.0f;
    }
}

void particlesThrowPoof(float x, float y, float z, int iconCell) {
    if (iconCell < 0) return;
    for (int i = 0; i < 8; i++) {
        P* p = alloc(); if (!p) return;
        baseInit(p, x, y, z, (frand() - 0.5f) * 0.4f, frand() * 0.3f, (frand() - 0.5f) * 0.4f);
        p->kind = K_TERRAIN;
        p->itemsAtlas = true;
        p->tex = iconCell;
        p->gravity = 1.0f;
        p->size /= 2.0f;
    }
}

void particlesExplosion(World* w, float x, float y, float z) {
    for (int i = 0; i < 40; i++) {
        P* p = alloc(); if (!p) return;
        baseInit(p, x + (frand() * 4 - 2), y + (frand() * 4 - 2), z + (frand() * 4 - 2),
                 (frand() * 2 - 1) * 0.5f, (frand() * 2 - 1) * 0.5f, (frand() * 2 - 1) * 0.5f);
        p->kind = K_EXPLODE;
        p->r = p->g = p->b = frand() * 0.3f + 0.7f;
        p->size = frand() * frand() * 6.0f + 1.0f;
        p->lifetime = (int)(16.0f / (frand() * 0.8f + 0.2f)) + 2;
    }
    for (int i = 0; i < 12; i++)
        particlesLargeSmoke(x + (frand() * 3 - 1.5f), y + (frand() * 2 - 0.5f), z + (frand() * 3 - 1.5f));
}

void particlesMobDeath(float x, float y, float z, float w, float h) {
    for (int i = 0; i < 20; i++) {
        P* p = alloc(); if (!p) return;
        baseInit(p, x + (frand() * 2 - 1) * w, y + frand() * h, z + (frand() * 2 - 1) * w,
                 (frand() * 2 - 1) * 0.02f, (frand() * 2 - 1) * 0.02f, (frand() * 2 - 1) * 0.02f);
        p->kind = K_EXPLODE;
        p->r = p->g = p->b = frand() * 0.3f + 0.7f;
        p->size = frand() * frand() * 4.0f + 2.0f;
        p->oSize = p->size;
        p->lifetime = (int)(16.0f / (frand() * 0.8f + 0.2f)) + 2;
    }
}

void particlesRedstonePoof(World* w, int x, int y, int z) {
    const float r = 1 / 16.0f;
    for (int i = 0; i < 6; i++) {
        float xx = x + frand(), yy = y + frand(), zz = z + frand();
        if (i == 0 && !isSolidBlocking(worldBlock(w, x, y + 1, z))) yy = y + 1 + r;
        if (i == 1 && !isSolidBlocking(worldBlock(w, x, y - 1, z))) yy = y + 0 - r;
        if (i == 2 && !isSolidBlocking(worldBlock(w, x, y, z + 1))) zz = z + 1 + r;
        if (i == 3 && !isSolidBlocking(worldBlock(w, x, y, z - 1))) zz = z + 0 - r;
        if (i == 4 && !isSolidBlocking(worldBlock(w, x + 1, y, z))) xx = x + 1 + r;
        if (i == 5 && !isSolidBlocking(worldBlock(w, x - 1, y, z))) xx = x + 0 - r;
        if (xx < x || xx > x + 1 || yy < 0 || yy > y + 1 || zz < z || zz > z + 1) {
            P* p = alloc(); if (!p) return;
            baseInit(p, xx, yy, zz, 0, 0, 0);
            p->kind = K_REDDUST; p->xd *= 0.1f; p->yd *= 0.1f; p->zd *= 0.1f;
            p->r = 1.0f; p->g = p->b = frand() * 0.1f;
            p->size *= 0.75f; p->oSize = p->size;
            p->lifetime = (int)(8.0f / (frand() * 0.8f + 0.2f));
        }
    }
}

static void flameSmokeAt(float fx, float fy, float fz) {
    P* p = alloc();
    if (p) { baseInit(p, fx, fy, fz, 0, 0, 0);
        p->kind = K_SMOKE;
        p->xd *= 0.1f; p->yd *= 0.1f; p->zd *= 0.1f;
        p->r = p->g = p->b = frand() * 0.5f; p->size *= 0.75f; p->oSize = p->size;
        p->lifetime = (int)(8.0f / (frand() * 0.8f + 0.2f)); }
    p = alloc();
    if (p) { baseInit(p, fx, fy, fz, 0, 0, 0);

        p->kind = K_FLAME; p->noPhysics = true; p->fullBright = true;
        p->xd *= 0.01f; p->yd *= 0.01f; p->zd *= 0.01f;
        p->x += (frand() - frand()) * 0.05f;
        p->y += (frand() - frand()) * 0.05f;
        p->z += (frand() - frand()) * 0.05f;
        p->xo = p->x; p->yo = p->y; p->zo = p->z;
        p->oSize = p->size; p->tex = 48;
        p->lifetime = (int)(8.0f / (frand() * 0.8f + 0.2f)) + 4; }
}

static void torchPoof(World* w, int x, int y, int z, unsigned char dir) {
    float fx = x + 0.5f, fy = y + 0.7f, fz = z + 0.5f;
    const float h = 0.22f, r = 0.27f;
    if      (dir == 1) { fx -= r; fy += h; }
    else if (dir == 2) { fx += r; fy += h; }
    else if (dir == 3) { fz -= r; fy += h; }
    else if (dir == 4) { fz += r; fy += h; }
    flameSmokeAt(fx, fy, fz);
}

void particlesFurnaceFire(int x, int y, int z, unsigned char dir) {
    extern const signed char kFaceNeighbor[6][3];
    const float r = 0.52f;
    float fx = x + 0.5f + kFaceNeighbor[dir][0] * r;
    float fy = y + frand() * 6.0f / 16.0f;
    float fz = z + 0.5f + kFaceNeighbor[dir][2] * r;
    float ss = frand() * 0.6f - 0.3f;
    if (kFaceNeighbor[dir][0] != 0) fz += ss; else fx += ss;
    flameSmokeAt(fx, fy, fz);
}

void particlesBubble(float x, float y, float z, float xa, float ya, float za) {
    P* p = alloc(); if (!p) return;
    baseInit(p, x, y, z, 0, 0, 0);
    p->kind = K_BUBBLE; p->tex = 32;
    p->r = p->g = p->b = 1.0f;
    p->size = p->size * (frand() * 0.6f + 0.2f);
    p->xd = xa * 0.2f + (frand() * 2 - 1) * 0.02f;
    p->yd = ya * 0.2f + (frand() * 2 - 1) * 0.02f;
    p->zd = za * 0.2f + (frand() * 2 - 1) * 0.02f;
    p->lifetime = (int)(8.0f / (frand() * 0.8f + 0.2f));
}

void particlesHeart(float x, float y, float z, float xa, float ya, float za) {
    P* p = alloc(); if (!p) return;
    baseInit(p, x, y, z, xa, ya, za);
    p->kind = K_HEART; p->tex = 80;
    p->r = p->g = p->b = 1.0f;

    p->xd *= 0.01f; p->zd *= 0.01f;
    p->yd = p->yd * 0.01f + 0.1f;
    p->size *= 1.5f;
    p->oSize = p->size;
    p->lifetime = 16;

}

void particlesHeartBurst(float x, float feetY, float z, float w, float h) {
    for (int i = 0; i < 7; i++) {

        float ja = (frand() * 2 - 1) * 0.02f;
        float jb = (frand() * 2 - 1) * 0.02f;
        float jc = (frand() * 2 - 1) * 0.02f;
        particlesHeart(x + frand() * w * 2.0f - w,
                       feetY + 0.5f + frand() * h,
                       z + frand() * w * 2.0f - w, ja, jb, jc);
    }
}

void particlesSplash(float x, float y, float z, float xa, float ya, float za) {
    P* p = alloc(); if (!p) return;
    baseInit(p, x, y, z, 0, 0, 0);
    p->kind = K_SPLASH; p->tex = 20;
    p->r = p->g = p->b = 1.0f;

    p->xd *= 0.3f; p->zd *= 0.3f;
    p->yd = frand() * 0.2f + 0.1f;

    p->gravity = 1.0f;
    if (ya == 0 && (xa != 0 || za != 0)) { p->xd = xa; p->yd = ya + 0.1f; p->zd = za; }
    p->lifetime = (int)(8.0f / (frand() * 0.8f + 0.2f));
}

void particlesCrit(float x, float y, float z, float xa, float ya, float za) {
    P* p = alloc(); if (!p) return;
    baseInit(p, x, y, z, 0, 0, 0);
    p->kind = K_CRIT; p->tex = 16 * 4 + 1;
    p->xd = p->xd * 0.1f + xa * 0.4f;
    p->yd = p->yd * 0.1f + ya * 0.4f;
    p->zd = p->zd * 0.1f + za * 0.4f;
    p->r = p->g = p->b = frand() * 0.3f + 0.6f;
    p->size *= 0.75f; p->oSize = p->size;
    p->lifetime = (int)(6.0f / (frand() * 0.8f + 0.6f));
}

void particlesSmoke(float x, float y, float z) {
    P* p = alloc(); if (!p) return;
    baseInit(p, x, y, z, 0, 0, 0);
    p->kind = K_SMOKE;
    p->xd *= 0.1f; p->yd *= 0.1f; p->zd *= 0.1f;
    p->r = p->g = p->b = frand() * 0.5f; p->size *= 0.75f; p->oSize = p->size;
    p->lifetime = (int)(8.0f / (frand() * 0.8f + 0.2f));
}

void particlesLargeSmoke(float x, float y, float z) {
    P* p = alloc(); if (!p) return;
    const float scale = 2.5f;
    baseInit(p, x, y, z, 0, 0, 0);
    p->kind = K_SMOKE;
    p->xd *= 0.1f; p->yd *= 0.1f; p->zd *= 0.1f;
    p->r = p->g = p->b = frand() * 0.5f;
    p->size *= 0.75f * scale; p->oSize = p->size;
    p->lifetime = (int)(scale * 8.0f / (frand() * 0.8f + 0.2f));
}

static void emitAmbient(World* w, float px, float py, float pz) {
    int cx = (int)floorf(px), cy = (int)floorf(py), cz = (int)floorf(pz);
    const int r = 16;
    for (int i = 0; i < 100; i++) {
        int bx = cx + (rand() % r) - (rand() % r);
        int by = cy + (rand() % r) - (rand() % r);
        int bz = cz + (rand() % r) - (rand() % r);
        unsigned char id = worldBlock(w, bx, by, bz);
        if (id == BLOCK_TORCH) {
            torchPoof(w, bx, by, bz, worldData(w, bx, by, bz));
        } else if (id == BLOCK_FURNACE_LIT) {
            particlesFurnaceFire(bx, by, bz, worldData(w, bx, by, bz) & 7);
        } else if (id == BLOCK_ORE_REDSTONE_LIT) {
            particlesRedstonePoof(w, bx, by, bz);
        } else if (isLavaId(id)) {

            if ((isWaterId(worldBlock(w, bx - 1, by, bz)) || isWaterId(worldBlock(w, bx + 1, by, bz)) ||
                 isWaterId(worldBlock(w, bx, by, bz - 1)) || isWaterId(worldBlock(w, bx, by, bz + 1)) ||
                 isWaterId(worldBlock(w, bx, by + 1, bz))) && rand() % 4 == 0) {
                particlesLargeSmoke(bx + frand(), by + 1.2f, bz + frand());
            } else if (worldBlock(w, bx, by + 1, bz) == BLOCK_AIR && rand() % 100 == 0) {
                P* p = alloc(); if (!p) continue;
                baseInit(p, bx + frand(), by + 1.0f, bz + frand(), 0, 0, 0);
                p->kind = K_LAVA; p->fullBright = true; p->tex = 49;
                p->xd *= 0.8f; p->zd *= 0.8f;
                p->yd = frand() * 0.4f + 0.05f;
                p->size *= (frand() * 2.0f + 0.2f); p->oSize = p->size;
                p->lifetime = (int)(16.0f / (frand() * 0.8f + 0.2f));
            }
        }

    }
}

void particlesTick(World* w, float px, float py, float pz) {
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (g_pool[i].active) tickOne(w, &g_pool[i]);
    emitAmbient(w, px, py, pz);
}

namespace {
struct PVertex { float u, v; unsigned int color; float x, y, z; };
}

void particlesRender(World* w, float yawDeg, float pitchDeg, float alpha,
                     const Texture* terrain, const Texture* misc, const Texture* items) {

    int nMisc = 0, nTerr = 0, nItems = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g_pool[i].active) continue;
        if (g_pool[i].itemsAtlas) nItems++;
        else if (g_pool[i].terrainAtlas) nTerr++;
        else nMisc++;
    }
    if (nMisc == 0 && nTerr == 0 && nItems == 0) return;

    const float D2R = 3.14159265f / 180.0f;
    float cy = cosf(yawDeg * D2R), sy = sinf(yawDeg * D2R);
    float cp = cosf(pitchDeg * D2R), sp = sinf(pitchDeg * D2R);
    float fx = cp * sy, fy = sp, fz = cp * cy;
    float rx = cy, ry = 0.0f, rz = -sy;
    float ux = fy * rz, uy = fz * rx - fx * rz, uz = -fy * rx;

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    { ScePspFVector3 b = { -g_relBaseX, -g_relBaseY, -g_relBaseZ }; sceGumTranslate(&b); }
    sceGuDisable(GU_CULL_FACE);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);

    for (int pass = 0; pass < 3; pass++) {
        bool terr = (pass == 1);
        int count = pass == 0 ? nMisc : pass == 1 ? nTerr : nItems;
        if (count == 0) continue;
        const Texture* tx = pass == 0 ? misc : pass == 1 ? terrain : items;
        if (tx) textureBindNoMip(tx); else continue;

        PVertex* v = (PVertex*)sceGuGetMemory(count * 6 * sizeof(PVertex));
        int n = 0;
        for (int i = 0; i < MAX_PARTICLES; i++) {
            P* p = &g_pool[i];
            if (!p->active) continue;
            int pp = p->itemsAtlas ? 2 : p->terrainAtlas ? 1 : 0;
            if (pp != pass) continue;

            float sz = p->size;
            float sfrac = (p->age + alpha) / (float)p->lifetime;
            if (p->kind == K_FLAME) sz = p->oSize * (1 - sfrac * sfrac * 0.5f);
            else if (p->kind == K_LAVA) sz = p->oSize * (1 - sfrac * sfrac);
            else if (p->kind == K_SMOKE || p->kind == K_REDDUST || p->kind == K_CRIT) {
                float l = sfrac * 32.0f; if (l > 1) l = 1; if (l < 0) l = 0; sz = p->oSize * l;
            }
            float r = 0.1f * sz;

            float u0, u1, v0, v1;
            if (p->itemsAtlas) {

                const float INV = 1.0f / 512.0f, HT = INV / 2.0f;
                float ub = ((p->tex & 31) * 16.0f + p->uo * 4.0f) * INV;
                float vb = (432.0f + (p->tex >> 5) * 16.0f + p->vo * 4.0f) * INV;
                u0 = ub + HT; u1 = ub + 4.0f * INV - HT;
                v0 = vb + HT; v1 = vb + 4.0f * INV - HT;
            } else if (terr) {
                const float HTT = (1.0f / 16.0f) / 32.0f;
                float ub = ((p->tex & 15) + p->uo / 4.0f) / 16.0f;
                float vb = ((p->tex >> 4) + p->vo / 4.0f) / 16.0f;
                u0 = ub + HTT; u1 = ub + 1.0f / 16.0f / 4 - HTT;
                v0 = vb + HTT; v1 = vb + 1.0f / 16.0f / 4 - HTT;
            } else {
                const float HTM = (1.0f / 16.0f) / 16.0f;
                float ub = (p->tex % 16) / 16.0f, vb = (p->tex / 16) / 16.0f;
                u0 = ub + HTM; u1 = ub + 1.0f / 16.0f - HTM;
                v0 = vb + HTM; v1 = vb + 1.0f / 16.0f - HTM;
            }

            unsigned int M = 255;
            if (!p->fullBright) {
                int br = lightRawAt(w, (int)floorf(p->x), (int)floorf(p->y), (int)floorf(p->z));
                M = g_brightColor[br] & 0xFF;
            }
            unsigned int R = (unsigned int)(p->r * M); if (R > 255) R = 255;
            unsigned int G = (unsigned int)(p->g * M); if (G > 255) G = 255;
            unsigned int B = (unsigned int)(p->b * M); if (B > 255) B = 255;
            unsigned int col = 0xFF000000u | (B << 16) | (G << 8) | R;

            float x = p->xo + (p->x - p->xo) * alpha;
            float y = p->yo + (p->y - p->yo) * alpha;
            float z = p->zo + (p->z - p->zo) * alpha;

            float TLx = x - rx*r + ux*r, TLy = y - ry*r + uy*r, TLz = z - rz*r + uz*r;
            float TRx = x + rx*r + ux*r, TRy = y + ry*r + uy*r, TRz = z + rz*r + uz*r;
            float BRx = x + rx*r - ux*r, BRy = y + ry*r - uy*r, BRz = z + rz*r - uz*r;
            float BLx = x - rx*r - ux*r, BLy = y - ry*r - uy*r, BLz = z - rz*r - uz*r;

            v[n++] = (PVertex){ u0, v0, col, TLx, TLy, TLz };
            v[n++] = (PVertex){ u1, v0, col, TRx, TRy, TRz };
            v[n++] = (PVertex){ u1, v1, col, BRx, BRy, BRz };
            v[n++] = (PVertex){ u0, v0, col, TLx, TLy, TLz };
            v[n++] = (PVertex){ u1, v1, col, BRx, BRy, BRz };
            v[n++] = (PVertex){ u0, v1, col, BLx, BLy, BLz };
        }
        if (n)
            sceGumDrawArray(GU_TRIANGLES,
                GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                n, 0, v);
    }

    sceGuEnable(GU_CULL_FACE);
}
