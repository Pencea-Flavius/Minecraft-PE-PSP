#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/fire.h"
#include <math.h>

static inline int wpFloor(float v) { int i = (int)v; return (v < 0 && v != i) ? i - 1 : i; }

struct SelectionAABB { float x0, y0, z0, x1, y1, z1; };

static int getSelectionAABBs(const World* w, int x, int y, int z, SelectionAABB out[3]) {
    unsigned char id = worldBlock(w, x, y, z);
    if (id == BLOCK_AIR || isLiquidId(id)) return 0;
    if (id == BLOCK_TOPSNOW) {
        out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 0.125f, z + 1.0f };
        return 1;
    } else if (id == BLOCK_WHEAT) {

        unsigned char data = worldData(w, x, y, z);
        float yy1 = (data + 1) / 8.0f;
        out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + yy1, z + 1.0f };
        return 1;
    } else if (id == BLOCK_MELON_STEM) {

        unsigned char data = worldData(w, x, y, z);
        float yy1 = (data * 2 + 2) / 16.0f;
        out[0] = { x + 0.375f, y + 0.0f, z + 0.375f, x + 0.625f, y + yy1, z + 0.625f };
        return 1;
    } else if (id == BLOCK_SIGN) {

        out[0] = { x + 0.25f, y + 0.0f, z + 0.25f, x + 0.75f, y + 1.0f, z + 0.75f };
        return 1;
    } else if (id == BLOCK_WALL_SIGN) {

        unsigned char face = worldData(w, x, y, z);
        const float h0 = 4.5f/16.0f, h1 = 12.5f/16.0f, d0 = 2.0f/16.0f;
        if (face == 2)      out[0] = { x + 0.0f, y + h0, z + 1.0f - d0, x + 1.0f, y + h1, z + 1.0f };
        else if (face == 3) out[0] = { x + 0.0f, y + h0, z + 0.0f,      x + 1.0f, y + h1, z + d0 };
        else if (face == 4) out[0] = { x + 1.0f - d0, y + h0, z + 0.0f, x + 1.0f, y + h1, z + 1.0f };
        else                out[0] = { x + 0.0f, y + h0, z + 0.0f,      x + d0,   y + h1, z + 1.0f };
        return 1;
    } else if (isBed(id)) {
        out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 9.0f/16.0f, z + 1.0f };
        return 1;
    } else if (isSlab(id)) {
        unsigned char data = worldData(w, x, y, z);
        float y0 = y + ((data & SLAB_TOP_SLOT_BIT) ? 0.5f : 0.0f);
        float y1 = y + ((data & SLAB_TOP_SLOT_BIT) ? 1.0f : 0.5f);
        out[0] = { x + 0.0f, y0, z + 0.0f, x + 1.0f, y1, z + 1.0f };
        return 1;
    } else if (isStairs(id)) {

        float b[3][6];
        int nb = stairShapeBoxes(w, x, y, z, worldData(w, x, y, z), b);
        for (int i = 0; i < nb; i++)
            out[i] = { x + b[i][0], y + b[i][1], z + b[i][2],
                       x + b[i][3], y + b[i][4], z + b[i][5] };
        return nb;
    } else if (isPane(id)) {

        auto att = [&](int bx, int by, int bz) -> bool {
            unsigned char nb = worldBlock(w, bx, by, bz);
            return isSolidPhys(nb) || isPane(nb) || isGlass(nb);
        };
        bool north = att(x, y, z - 1), south = att(x, y, z + 1);
        bool west  = att(x - 1, y, z), east  = att(x + 1, y, z);
        float minX = 7.0f/16.0f, maxX = 9.0f/16.0f, minZ = 7.0f/16.0f, maxZ = 9.0f/16.0f;
        if ((west && east) || (!west && !east && !north && !south)) { minX = 0.0f; maxX = 1.0f; }
        else if (west && !east) minX = 0.0f;
        else if (!west && east) maxX = 1.0f;
        if ((north && south) || (!west && !east && !north && !south)) { minZ = 0.0f; maxZ = 1.0f; }
        else if (north && !south) minZ = 0.0f;
        else if (!north && south) maxZ = 1.0f;
        out[0] = { x + minX, y + 0.0f, z + minZ, x + maxX, y + 1.0f, z + maxZ };
        return 1;
    } else if (isFence(id)) {

        bool fn = connectsFence(worldBlock(w, x, y, z - 1));
        bool fs = connectsFence(worldBlock(w, x, y, z + 1));
        bool fw = connectsFence(worldBlock(w, x - 1, y, z));
        bool fe = connectsFence(worldBlock(w, x + 1, y, z));
        float fx0 = fw ? (float)x : x + 6.0f/16.0f;
        float fx1 = fe ? x + 1.0f : x + 10.0f/16.0f;
        float fz0 = fn ? (float)z : z + 6.0f/16.0f;
        float fz1 = fs ? z + 1.0f : z + 10.0f/16.0f;

        out[0] = { fx0, (float)y, fz0, fx1, y + 1.0f, fz1 };
        return 1;
    } else if (isDoor(id)) {
        unsigned char data = worldData(w, x, y, z);
        bool isUpper = (data & 8) != 0;
        int lowerData = isUpper ? worldData(w, x, y - 1, z) : data;
        int upperData = isUpper ? data : worldData(w, x, y + 1, z);

        float r = 3.0f / 16.0f;
        int dir = lowerData & 3;
        bool open = (lowerData & 4) != 0;
        bool rightHinge = (upperData & 1) != 0;
        int shapeDir = 0;
        if (dir == 0) {
            if (open) {
                if (!rightHinge) shapeDir = 0;
                else shapeDir = 2;
            } else shapeDir = 3;
        } else if (dir == 1) {
            if (open) {
                if (!rightHinge) shapeDir = 1;
                else shapeDir = 3;
            } else shapeDir = 0;
        } else if (dir == 2) {
            if (open) {
                if (!rightHinge) shapeDir = 2;
                else shapeDir = 0;
            } else shapeDir = 1;
        } else if (dir == 3) {
            if (open) {
                if (!rightHinge) shapeDir = 3;
                else shapeDir = 1;
            } else shapeDir = 2;
        }

        float box_y0 = (float)y;
        float box_y1 = (float)(y + 1);

        if (shapeDir == 0) out[0] = { x + 0.0f, box_y0, z + 0.0f, x + 1.0f, box_y1, z + r };
        else if (shapeDir == 1) out[0] = { x + 1.0f - r, box_y0, z + 0.0f, x + 1.0f, box_y1, z + 1.0f };
        else if (shapeDir == 2) out[0] = { x + 0.0f, box_y0, z + 1.0f - r, x + 1.0f, box_y1, z + 1.0f };
        else if (shapeDir == 3) out[0] = { x + 0.0f, box_y0, z + 0.0f, x + r, box_y1, z + 1.0f };

        return 1;
    } else if (isTrapdoor(id)) {
        unsigned char data = worldData(w, x, y, z);
        bool open = (data & 4) != 0;

        float r = 3.0f / 16.0f;
        float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f;
        float x1 = 1.0f, y1 = r, z1 = 1.0f;

        if (open) {
            if ((data & 3) == 0) { x0 = 0; x1 = 1; y0 = 0; y1 = 1; z0 = 1 - r; z1 = 1; }
            else if ((data & 3) == 1) { x0 = 0; x1 = 1; y0 = 0; y1 = 1; z0 = 0; z1 = r; }
            else if ((data & 3) == 2) { x0 = 1 - r; x1 = 1; y0 = 0; y1 = 1; z0 = 0; z1 = 1; }
            else if ((data & 3) == 3) { x0 = 0; x1 = r; y0 = 0; y1 = 1; z0 = 0; z1 = 1; }
        }

        out[0] = { x + x0, y + y0, z + z0, x + x1, y + y1, z + z1 };
        return 1;
    } else if (isLadder(id)) {
        unsigned char data = worldData(w, x, y, z);
        float r = 2.0f / 16.0f;
        if (data == 2) out[0] = { x + 0.0f, y + 0.0f, z + 1.0f - r, x + 1.0f, y + 1.0f, z + 1.0f };
        else if (data == 3) out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + r };
        else if (data == 4) out[0] = { x + 1.0f - r, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f };
        else if (data == 5) out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + r, y + 1.0f, z + 1.0f };
        else out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f };
        return 1;
    } else if (isTorch(id)) {
        unsigned char data = worldData(w, x, y, z);
        float hw = 0.15f;
        if (data == 1) out[0] = { x + 0.0f, y + 0.2f, z + 0.5f - hw, x + hw * 2.0f, y + 0.8f, z + 0.5f + hw };
        else if (data == 2) out[0] = { x + 1.0f - hw * 2.0f, y + 0.2f, z + 0.5f - hw, x + 1.0f, y + 0.8f, z + 0.5f + hw };
        else if (data == 3) out[0] = { x + 0.5f - hw, y + 0.2f, z + 0.0f, x + 0.5f + hw, y + 0.8f, z + hw * 2.0f };
        else if (data == 4) out[0] = { x + 0.5f - hw, y + 0.2f, z + 1.0f - hw * 2.0f, x + 0.5f + hw, y + 0.8f, z + 1.0f };
        else out[0] = { x + 0.5f - hw, y + 0.0f, z + 0.5f - hw, x + 0.5f + hw, y + 0.6f, z + 0.5f + hw };
        return 1;
    } else if (isFenceGate(id)) {

        unsigned char data = worldData(w, x, y, z);
        int dir = data & 3;

        if (dir == 1 || dir == 3) {
            out[0] = { x + 6.0f/16.0f, y + 0.0f, z + 0.0f, x + 10.0f/16.0f, y + 1.0f, z + 1.0f };
        } else {
            out[0] = { x + 0.0f, y + 0.0f, z + 6.0f/16.0f, x + 1.0f, y + 1.0f, z + 10.0f/16.0f };
        }
        return 1;
    } else if (id == BLOCK_CACTUS) {

        float r = 1.0f/16.0f;
        out[0] = { x + r, y + 0.0f, z + r, x + 1.0f - r, y + 1.0f, z + 1.0f - r };
        return 1;
    } else if (id == BLOCK_REEDS) {

        const float ss = 6.0f/16.0f;
        out[0] = { x + 0.5f - ss, y + 0.0f, z + 0.5f - ss, x + 0.5f + ss, y + 1.0f, z + 0.5f + ss };
        return 1;
    } else if (id == BLOCK_CHEST) {

        out[0] = { x + 1.0f/16.0f, y + 0.0f, z + 1.0f/16.0f, x + 15.0f/16.0f, y + 14.0f/16.0f, z + 15.0f/16.0f };
        return 1;
    } else if (id == BLOCK_FARMLAND) {

        out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 15.0f/16.0f, z + 1.0f };
        return 1;
    } else if (id == BLOCK_FLOWER || id == BLOCK_ROSE) {

        const float ss = 0.2f;
        out[0] = { x + 0.5f - ss, y + 0.0f, z + 0.5f - ss, x + 0.5f + ss, y + ss * 3.0f, z + 0.5f + ss };
        return 1;
    } else if (id == BLOCK_MUSHROOM_BROWN || id == BLOCK_MUSHROOM_RED) {

        const float ss = 0.2f;
        out[0] = { x + 0.5f - ss, y + 0.0f, z + 0.5f - ss, x + 0.5f + ss, y + ss * 2.0f, z + 0.5f + ss };
        return 1;
    } else if (id == BLOCK_SAPLING || id == BLOCK_TALLGRASS) {

        const float ss = 0.4f;
        out[0] = { x + 0.5f - ss, y + 0.0f, z + 0.5f - ss, x + 0.5f + ss, y + 0.8f, z + 0.5f + ss };
        return 1;
    } else if (id == BLOCK_FIRE) {

        if (isSolidBlocking(worldBlock(w, x, y - 1, z)) || fireCanBurn(w, x, y - 1, z)) {
            out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f };
        } else {
            const float r = 0.2f;
            if      (fireCanBurn(w, x - 1, y, z)) out[0] = { x + 0.0f,     y + 0.0f, z + 0.0f,     x + r,      y + 1.0f, z + 1.0f };
            else if (fireCanBurn(w, x + 1, y, z)) out[0] = { x + 1.0f - r, y + 0.0f, z + 0.0f,     x + 1.0f,   y + 1.0f, z + 1.0f };
            else if (fireCanBurn(w, x, y, z - 1)) out[0] = { x + 0.0f,     y + 0.0f, z + 0.0f,     x + 1.0f,   y + 1.0f, z + r };
            else if (fireCanBurn(w, x, y, z + 1)) out[0] = { x + 0.0f,     y + 0.0f, z + 1.0f - r, x + 1.0f,   y + 1.0f, z + 1.0f };
            else                                  out[0] = { x + 0.0f,     y + 0.0f, z + 0.0f,     x + 1.0f,   y + 1.0f, z + 1.0f };
        }
        return 1;
    }

    out[0] = { x + 0.0f, y + 0.0f, z + 0.0f, x + 1.0f, y + 1.0f, z + 1.0f };
    return 1;
}

int worldSelectionBoxes(const World* w, int x, int y, int z, float out[3][6]) {
    SelectionAABB b[3];
    int n = getSelectionAABBs(w, x, y, z, b);
    for (int i = 0; i < n; i++) {
        out[i][0] = b[i].x0; out[i][1] = b[i].y0; out[i][2] = b[i].z0;
        out[i][3] = b[i].x1; out[i][4] = b[i].y1; out[i][5] = b[i].z1;
    }
    return n;
}

static const float CLIP_EPS = 1e-4f;

static bool clipAxis(float p, float d, float minV, float maxV, int fMin, int fMax, float& tmin, float& tmax, int& faceMin) {
    if (fabsf(d) < 1e-6f) {
        if (p < minV || p > maxV) return false;
    } else {
        float t1 = (minV - p) / d;
        float t2 = (maxV - p) / d;
        int f1 = fMin, f2 = fMax;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; int fTmp = f1; f1 = f2; f2 = fTmp; }
        if (t1 > tmin) { tmin = t1; faceMin = f1; }
        if (t2 < tmax) tmax = t2;
        if (tmin > tmax + CLIP_EPS) return false;
        if (tmax < 0.0f) return false;
    }
    return true;
}

static bool clipAABB(float px, float py, float pz, float dx, float dy, float dz, float range, const SelectionAABB& box, float& outT, int& outFace) {
    float tmin = -1e9f, tmax = 1e9f;
    int faceMin = -1;

    if (!clipAxis(px, dx, box.x0, box.x1, F_LEFT, F_RIGHT, tmin, tmax, faceMin)) return false;
    if (!clipAxis(py, dy, box.y0, box.y1, F_DOWN, F_TOP, tmin, tmax, faceMin)) return false;
    if (!clipAxis(pz, dz, box.z0, box.z1, F_BACK, F_FORWARD, tmin, tmax, faceMin)) return false;

    if (tmin >= 0.0f && tmin <= range) {
        outT = tmin;
        outFace = faceMin;
        return true;
    }
    if (tmin < 0.0f && tmax >= 0.0f && tmax <= range) {
        outT = 0.0f;
        outFace = faceMin;
        return true;
    }
    return false;
}

static bool pickCell(const World* w, int cx, int cy, int cz,
                     float px, float py, float pz, float dx, float dy, float dz,
                     float range, BlockHit& out) {
    unsigned char blk = worldBlock(w, cx, cy, cz);
    if (blk == BLOCK_AIR || isLiquidId(blk)) return false;
    SelectionAABB boxes[3];
    int numBoxes = getSelectionAABBs(w, cx, cy, cz, boxes);
    float bestT = 1e9f;
    int bestFace = -1;
    for (int i = 0; i < numBoxes; i++) {
        float t; int f;
        if (clipAABB(px, py, pz, dx, dy, dz, range, boxes[i], t, f)) {
            if (t < bestT) { bestT = t; bestFace = f; }
        }
    }
    if (bestFace == -1) return false;
    float hx = px + dx * bestT, hy = py + dy * bestT, hz = pz + dz * bestT;
    BlockHit hit = { true, cx, cy, cz, bestFace, hx - cx, hy - cy, hz - cz };
    out = hit;
    return true;
}

BlockHit worldPick(const World* w, float px, float py, float pz, float yaw, float pitch, float range) {
    const float DEG2RAD = 3.14159265f / 180.0f;
    float cy = cosf(yaw * DEG2RAD), sy = sinf(yaw * DEG2RAD);
    float cp = cosf(pitch * DEG2RAD), sp = sinf(pitch * DEG2RAD);
    float dx = cp * sy, dy = sp, dz = cp * cy;

    float ax = px, ay = py, az = pz;
    float bx = px + dx * range, by = py + dy * range, bz = pz + dz * range;

    int xTile0 = wpFloor(ax), yTile0 = wpFloor(ay), zTile0 = wpFloor(az);
    int xTile1 = wpFloor(bx), yTile1 = wpFloor(by), zTile1 = wpFloor(bz);

    BlockHit miss = {false, 0, 0, 0, 0};

    BlockHit first;
    if (pickCell(w, xTile0, yTile0, zTile0, px, py, pz, dx, dy, dz, range, first))
        return first;

    int maxIterations = 200;
    while (maxIterations-- >= 0) {
        if (xTile0 == xTile1 && yTile0 == yTile1 && zTile0 == zTile1) return miss;

        float xClip = 1e9f, yClip = 1e9f, zClip = 1e9f;
        if (xTile1 > xTile0) xClip = xTile0 + 1.0f;
        if (xTile1 < xTile0) xClip = xTile0 + 0.0f;
        if (yTile1 > yTile0) yClip = yTile0 + 1.0f;
        if (yTile1 < yTile0) yClip = yTile0 + 0.0f;
        if (zTile1 > zTile0) zClip = zTile0 + 1.0f;
        if (zTile1 < zTile0) zClip = zTile0 + 0.0f;

        float xd = bx - ax, yd = by - ay, zd = bz - az;
        float xDist = 1e9f, yDist = 1e9f, zDist = 1e9f;
        if (xClip < 1e9f) xDist = (xClip - ax) / xd;
        if (yClip < 1e9f) yDist = (yClip - ay) / yd;
        if (zClip < 1e9f) zDist = (zClip - az) / zd;

        int sx = 0, sy = 0, sz = 0;
        if (xDist < yDist && xDist < zDist) {
            ax = xClip; ay += yd * xDist; az += zd * xDist;
            sx = xTile1 > xTile0 ? 1 : -1;
        } else if (yDist < zDist) {
            ax += xd * yDist; ay = yClip; az += zd * yDist;
            sy = yTile1 > yTile0 ? 1 : -1;
        } else {
            ax += xd * zDist; ay += yd * zDist; az = zClip;
            sz = zTile1 > zTile0 ? 1 : -1;
        }

        xTile0 += sx;
        yTile0 += sy;
        zTile0 += sz;

        BlockHit hit;
        if (pickCell(w, xTile0, yTile0, zTile0, px, py, pz, dx, dy, dz, range, hit))
            return hit;
    }
    return miss;
}
