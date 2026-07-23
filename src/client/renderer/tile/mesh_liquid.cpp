
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"

#include <math.h>

static inline bool isSolidForLiquid(unsigned char id) {
    if (id == BLOCK_AIR || isLiquidId(id) || isCrossShaped(id)) return false;
    return true;
}

static inline bool isSolidRender(unsigned char id) {
    return isOpaque(id);
}

static float getLiquidHeightRaw(int d) {
    if (d >= 8) d = 0;
    return (d + 1) / 9.0f;
}

static float getLiquidHeight(const World* w, int x, int y, int z, unsigned char liquidId) {
    int count = 0;
    float h = 0;
    for (int i = 0; i < 4; i++) {
        int xx = x - (i & 1);
        int yy = y;
        int zz = z - ((i >> 1) & 1);

        if (sameLiquid(worldBlock(w, xx, yy + 1, zz), liquidId)) {
            return 1.0f;
        }
        unsigned char tm = worldBlock(w, xx, yy, zz);
        if (sameLiquid(tm, liquidId)) {
            int d = worldData(w, xx, yy, zz);
            if (d >= 8 || d == 0) {
                h += getLiquidHeightRaw(d) * 10.0f;
                count += 10;
            }
            h += getLiquidHeightRaw(d);
            count++;
        } else if (!isSolidForLiquid(tm)) {

            h += 1.0f;
            count++;
        }
    }
    if (count == 0) return 0.0f;
    return 1.0f - h / (float)count;
}

static int getRenderedDepth(const World* w, int x, int y, int z, unsigned char liquidId) {
    if (!sameLiquid(worldBlock(w, x, y, z), liquidId)) return -1;
    int d = worldData(w, x, y, z);
    if (d >= 8) d = 0;
    return d;
}

static float getSlopeAngle(const World* w, int x, int y, int z, unsigned char liquidId) {
    float flowX = 0.0f, flowZ = 0.0f;
    int mid = getRenderedDepth(w, x, y, z, liquidId);
    for (int d = 0; d < 4; d++) {
        int xt = x, yt = y, zt = z;
        if (d == 0) xt--;
        if (d == 1) zt--;
        if (d == 2) xt++;
        if (d == 3) zt++;

        int t = getRenderedDepth(w, xt, yt, zt, liquidId);
        if (t < 0) {

            if (!isSolidForLiquid(worldBlock(w, xt, yt, zt))) {
                t = getRenderedDepth(w, xt, yt - 1, zt, liquidId);
                if (t >= 0) {
                    int dir = t - (mid - 8);
                    flowX += (float)((xt - x) * dir);
                    flowZ += (float)((zt - z) * dir);
                }
            }
        } else {
            int dir = t - mid;
            flowX += (float)((xt - x) * dir);
            flowZ += (float)((zt - z) * dir);
        }
    }
    if (flowX == 0.0f && flowZ == 0.0f) return -1000.0f;
    return atan2f(flowZ, flowX) - 3.14159265f * 0.5f;
}

static inline int pushLiquidQuad(ChunkVertex* out, int n, const ChunkVertex v[4], bool two) {
    out[n++] = v[0]; out[n++] = v[1]; out[n++] = v[2];
    out[n++] = v[2]; out[n++] = v[3]; out[n++] = v[0];
    if (two) {
        out[n++] = v[0]; out[n++] = v[2]; out[n++] = v[1];
        out[n++] = v[2]; out[n++] = v[0]; out[n++] = v[3];
    }
    return n;
}

int emitLiquid(const World* w, int gx, int y, int gz, unsigned char id, ChunkVertex* out, int n) {
    bool twoSided = isLavaId(id);
    float h00 = getLiquidHeight(w, gx, y, gz, id);
    float h01 = getLiquidHeight(w, gx, y, gz + 1, id);
    float h11 = getLiquidHeight(w, gx + 1, y, gz + 1, id);
    float h10 = getLiquidHeight(w, gx + 1, y, gz, id);

    int col, row; unsigned int tint;
    float x = (float)gx;
    float yf = (float)y;
    float z = (float)gz;

    for (int f = 0; f < 6; f++) {
        int nx = gx + kFaceNeighbor[f][0];
        int ny = y  + kFaceNeighbor[f][1];
        int nz = gz + kFaceNeighbor[f][2];
        unsigned char nb = worldBlock(w, nx, ny, nz);

        if (sameLiquid(nb, id)) continue;
        if (nb == BLOCK_ICE) continue;

        if (nb == BLOCK_FARMLAND) continue;
        if (f != F_TOP && isSolidRender(nb)) continue;

        if (!out) {
            n += twoSided ? 12 : 6;
            continue;
        }

        tileForBlock(id, 0, f, &col, &row, &tint);
        float u0 = col * TILE_UV;
        float v0 = row * TILE_UV;
        float u1 = u0 + TILE_UV;
        float v1 = v0 + TILE_UV;

        const float TE = TILE_UV / 128.0f;
        u0 += TE; u1 -= TE;
        v0 += TE; v1 -= TE;

        unsigned int shade = (lightEmit(id) > 0) ? 0xFFFFFFFFu : kFaceShade[f];
        int faceBr = lightRawAt(w, nx, ny, nz);
        if (lightEmit(id) > faceBr) faceBr = lightEmit(id);
        unsigned int color = mulColor(mulColor(shade, tint),
                                      g_brightColor[faceBr]);

        if (f == F_TOP) {

            float angle = getSlopeAngle(w, gx, y, gz, id);
            float uc, vc;
            if (angle < -999.0f) {
                angle = 0.0f;
                uc = col * TILE_UV + 0.5f * TILE_UV;
                vc = row * TILE_UV + 0.5f * TILE_UV;
            } else {
                uc = (col + 1) * TILE_UV + TILE_UV;
                vc = row * TILE_UV + TILE_UV;
            }

            float s = sinf(angle) * 7.5f / 256.5f;
            float cc = cosf(angle) * 7.5f / 256.5f;
            ChunkVertex vtx[4] = {
                {uc - cc + s, vc + cc + s, color, x,     yf + h01, z + 1},
                {uc + cc + s, vc + cc - s, color, x + 1, yf + h11, z + 1},
                {uc + cc - s, vc - cc - s, color, x + 1, yf + h10, z    },
                {uc - cc - s, vc - cc + s, color, x,     yf + h00, z    }
            };
            n = pushLiquidQuad(out, n, vtx, twoSided);
        } else if (f == F_DOWN) {
            ChunkVertex vtx[4] = {
                {u0, v0, color, x,     yf, z    },
                {u1, v0, color, x + 1, yf, z    },
                {u1, v1, color, x + 1, yf, z + 1},
                {u0, v1, color, x,     yf, z + 1}
            };
            n = pushLiquidQuad(out, n, vtx, twoSided);
        } else if (f == F_LEFT) {
            float vv1 = v0 + (1.0f - h01) * TILE_UV;
            float vv0 = v0 + (1.0f - h00) * TILE_UV;
            ChunkVertex vtx[4] = {
                {u1, v1,  color, x, yf,       z + 1},
                {u1, vv1, color, x, yf + h01, z + 1},
                {u0, vv0, color, x, yf + h00, z    },
                {u0, v1,  color, x, yf,       z    }
            };
            n = pushLiquidQuad(out, n, vtx, twoSided);
        } else if (f == F_RIGHT) {
            float vv3 = v0 + (1.0f - h10) * TILE_UV;
            float vv2 = v0 + (1.0f - h11) * TILE_UV;
            ChunkVertex vtx[4] = {
                {u0, v1,  color, x + 1, yf,       z    },
                {u0, vv3, color, x + 1, yf + h10, z    },
                {u1, vv2, color, x + 1, yf + h11, z + 1},
                {u1, v1,  color, x + 1, yf,       z + 1}
            };
            n = pushLiquidQuad(out, n, vtx, twoSided);
        } else if (f == F_BACK) {
            float vv0 = v0 + (1.0f - h00) * TILE_UV;
            float vv3 = v0 + (1.0f - h10) * TILE_UV;
            ChunkVertex vtx[4] = {
                {u0, vv0, color, x,     yf + h00, z},
                {u1, vv3, color, x + 1, yf + h10, z},
                {u1, v1,  color, x + 1, yf,       z},
                {u0, v1,  color, x,     yf,       z}
            };
            n = pushLiquidQuad(out, n, vtx, twoSided);
        } else if (f == F_FORWARD) {
            float vv1 = v0 + (1.0f - h01) * TILE_UV;
            float vv2 = v0 + (1.0f - h11) * TILE_UV;
            ChunkVertex vtx[4] = {
                {u0, v1,  color, x,     yf,       z + 1},
                {u1, v1,  color, x + 1, yf,       z + 1},
                {u1, vv2, color, x + 1, yf + h11, z + 1},
                {u0, vv1, color, x,     yf + h01, z + 1}
            };
            n = pushLiquidQuad(out, n, vtx, twoSided);
        }
    }
    return n;
}
