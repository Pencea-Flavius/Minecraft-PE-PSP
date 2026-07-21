
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"

static const unsigned char kFaceUV[6][4][2] = {

     { {1,1},{1,0},{0,0},{0,1} },
     { {1,1},{1,0},{0,0},{0,1} },
     { {0,0},{1,0},{1,1},{0,1} },
     { {0,1},{1,1},{1,0},{0,0} },
     { {1,0},{0,0},{0,1},{1,1} },
     { {0,1},{1,1},{1,0},{0,0} },
};

static const float SEAM_INFLATE = 0.0f;

static inline float seamOff(int corner) { return corner ? SEAM_INFLATE : -SEAM_INFLATE; }

static const signed char kFaceCorner[6][4][3] = {
     { {0,0,1},{0,1,1},{0,1,0},{0,0,0} },
     { {1,0,0},{1,1,0},{1,1,1},{1,0,1} },
     { {0,0,0},{1,0,0},{1,0,1},{0,0,1} },
     { {0,1,1},{1,1,1},{1,1,0},{0,1,0} },
     { {0,1,0},{1,1,0},{1,0,0},{0,0,0} },
     { {0,0,1},{1,0,1},{1,1,1},{0,1,1} },
};

#define WATER_TOP 0.889f

static int writeQuadDouble(ChunkVertex* out, int n, const float P[4][3],
                           const float UV[4][2], unsigned int color) {
    static const int triF[6] = { 0, 1, 2, 2, 3, 0 };
    static const int triB[6] = { 0, 2, 1, 2, 0, 3 };
    for (int pass = 0; pass < 2; pass++) {
        const int* tri = pass ? triB : triF;
        for (int t = 0; t < 6; t++) {
            int k = tri[t];
            out[n].u = UV[k][0]; out[n].v = UV[k][1]; out[n].color = color;
            out[n].x = P[k][0]; out[n].y = P[k][1]; out[n].z = P[k][2];
            n++;
        }
    }
    return n;
}

int emitCross(ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id,
                     unsigned char data, unsigned int bright) {
    int col, row; unsigned int tint;
    tileForBlock(id, data, 0, &col, &row, &tint);

    const float HT = TILE_UV / 32.0f;
    float u0 = col * TILE_UV + HT, v0 = row * TILE_UV + HT;
    float u1 = (col + 1) * TILE_UV - HT, v1 = (row + 1) * TILE_UV - HT;

    unsigned int color = mulColor(bright, tint);

    float x0 = gx + 0.05f, x1 = gx + 0.95f;
    float z0 = gz + 0.05f, z1 = gz + 0.95f;
    float yb = (float)y, yt = (float)y + 1.0f;
    const float UV[4][2] = { {u0, v0}, {u0, v1}, {u1, v1}, {u1, v0} };

    float A[4][3] = { {x0, yt, z0}, {x0, yb, z0}, {x1, yb, z1}, {x1, yt, z1} };
    float B[4][3] = { {x0, yt, z1}, {x0, yb, z1}, {x1, yb, z0}, {x1, yt, z0} };
    n = writeQuadDouble(out, n, A, UV, color);
    n = writeQuadDouble(out, n, B, UV, color);
    return n;
}

int emitCropRows(ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id,
                  unsigned char data, unsigned int bright) {
    int col, row; unsigned int tint;
    tileForBlock(id, data, 0, &col, &row, &tint);
    const float HT = TILE_UV / 32.0f;
    float u0 = col * TILE_UV + HT, v0 = row * TILE_UV + HT;
    float u1 = (col + 1) * TILE_UV - HT, v1 = (row + 1) * TILE_UV - HT;
    unsigned int color = mulColor(bright, tint);
    const float UV[4][2] = { {u0, v0}, {u0, v1}, {u1, v1}, {u1, v0} };
    float yb = (float)y, yt = (float)y + 1.0f;

    for (int i = 0; i < 2; i++) {
        float px = gx + (i == 0 ? 0.25f : 0.75f);
        float P[4][3] = { {px, yt, (float)gz}, {px, yb, (float)gz}, {px, yb, gz + 1.0f}, {px, yt, gz + 1.0f} };
        n = writeQuadDouble(out, n, P, UV, color);
    }
    for (int i = 0; i < 2; i++) {
        float pz = gz + (i == 0 ? 0.25f : 0.75f);
        float P[4][3] = { {(float)gx, yt, pz}, {(float)gx, yb, pz}, {gx + 1.0f, yb, pz}, {gx + 1.0f, yt, pz} };
        n = writeQuadDouble(out, n, P, UV, color);
    }
    return n;
}

int emitMelonStem(const World* w, ChunkVertex* out, int n, int gx, int y, int gz, unsigned char data, unsigned int bright) {
    int col, row; unsigned int tint;
    tileForBlock(BLOCK_MELON_STEM, data, 0, &col, &row, &tint);
    unsigned int color = mulColor(bright, tint);

    int connectDir = -1;
    if (data >= 7) {
        static const signed char dir[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
        for (int i = 0; i < 4; i++)
            if (worldBlock(w, gx + dir[i][0], y, gz + dir[i][1]) == BLOCK_MELON) { connectDir = i; break; }
    }

    float yy1 = (data * 2 + 2) / 16.0f;
    float hCross = (connectDir >= 0) ? 0.5f : yy1;
    float yb = (float)y - 1.0f / 16.0f, yt = yb + hCross;
    float x0 = gx + 0.05f, x1 = gx + 0.95f;
    float z0 = gz + 0.05f, z1 = gz + 0.95f;

    const float HT = TILE_UV / 32.0f;
    float u0 = col * TILE_UV + HT, u1 = (col + 1) * TILE_UV - HT;
    float v0 = row * TILE_UV + HT, v1 = row * TILE_UV + hCross * TILE_UV;
    const float UV[4][2] = { {u0, v0}, {u0, v1}, {u1, v1}, {u1, v0} };

    float A[4][3] = { {x0, yt, z0}, {x0, yb, z0}, {x1, yb, z1}, {x1, yt, z1} };
    float B[4][3] = { {x0, yt, z1}, {x0, yb, z1}, {x1, yb, z0}, {x1, yt, z0} };
    n = writeQuadDouble(out, n, A, UV, color);
    n = writeQuadDouble(out, n, B, UV, color);

    if (connectDir >= 0) {
        float cyb = (float)y - 1.0f / 16.0f, cyt = cyb + yy1;
        int crow = row + 1;
        float cu0 = col * TILE_UV + HT, cu1 = (col + 1) * TILE_UV - HT;
        float cv0 = crow * TILE_UV + HT, cv1 = crow * TILE_UV + yy1 * TILE_UV;
        if (connectDir == 1 || connectDir == 2) { float t = cu0; cu0 = cu1; cu1 = t; }
        const float CUV[4][2] = { {cu0, cv0}, {cu0, cv1}, {cu1, cv1}, {cu1, cv0} };
        float gxf = (float)gx, gzf = (float)gz;
        float P[4][3];
        if (connectDir < 2) {
            float zm = gzf + 0.5f;
            P[0][0] = gxf;        P[0][1] = cyt; P[0][2] = zm;
            P[1][0] = gxf;        P[1][1] = cyb; P[1][2] = zm;
            P[2][0] = gxf + 1.0f; P[2][1] = cyb; P[2][2] = zm;
            P[3][0] = gxf + 1.0f; P[3][1] = cyt; P[3][2] = zm;
        } else {
            float xm = gxf + 0.5f;
            P[0][0] = xm; P[0][1] = cyt; P[0][2] = gzf + 1.0f;
            P[1][0] = xm; P[1][1] = cyb; P[1][2] = gzf + 1.0f;
            P[2][0] = xm; P[2][1] = cyb; P[2][2] = gzf;
            P[3][0] = xm; P[3][1] = cyt; P[3][2] = gzf;
        }
        n = writeQuadDouble(out, n, P, CUV, color);
    }
    return n;
}

int emitLadder(ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id, unsigned char data, unsigned int bright) {
    int col, row; unsigned int tint;
    tileForBlock(id, data, 0, &col, &row, &tint);
    float u0 = col * TILE_UV, v0 = row * TILE_UV;
    float u1 = (col + 1) * TILE_UV, v1 = (row + 1) * TILE_UV;
    float x0 = gx, x1 = gx + 1.0f;
    float z0 = gz, z1 = gz + 1.0f;
    float yb = (float)y, yt = (float)y + 1.0f;
    float r = 0.05f;
    float P[4][3];
    if (data == 2) {
        P[0][0]=x1; P[0][1]=yt; P[0][2]=z1-r; P[1][0]=x1; P[1][1]=yb; P[1][2]=z1-r; P[2][0]=x0; P[2][1]=yb; P[2][2]=z1-r; P[3][0]=x0; P[3][1]=yt; P[3][2]=z1-r;
    } else if (data == 3) {
        P[0][0]=x0; P[0][1]=yt; P[0][2]=z0+r; P[1][0]=x0; P[1][1]=yb; P[1][2]=z0+r; P[2][0]=x1; P[2][1]=yb; P[2][2]=z0+r; P[3][0]=x1; P[3][1]=yt; P[3][2]=z0+r;
    } else if (data == 4) {
        P[0][0]=x1-r; P[0][1]=yt; P[0][2]=z0; P[1][0]=x1-r; P[1][1]=yb; P[1][2]=z0; P[2][0]=x1-r; P[2][1]=yb; P[2][2]=z1; P[3][0]=x1-r; P[3][1]=yt; P[3][2]=z1;
    } else if (data == 5) {
        P[0][0]=x0+r; P[0][1]=yt; P[0][2]=z1; P[1][0]=x0+r; P[1][1]=yb; P[1][2]=z1; P[2][0]=x0+r; P[2][1]=yb; P[2][2]=z0; P[3][0]=x0+r; P[3][1]=yt; P[3][2]=z0;
    } else return n;
    const float UV[4][2] = { {u0, v0}, {u0, v1}, {u1, v1}, {u1, v0} };
    return writeQuadDouble(out, n, P, UV, bright);
}

int meshPass(const World* w, int ox, int oz, int y0, int y1, ChunkVertex* out, int layer, int cap, bool leavesOpaque, bool leavesCull) {
    int n = 0;

    unsigned char lc[18 * 18 * 18];
    unsigned char llc[18 * 18 * 18];
    if (layer != 1) {
        bool wantLight = (out != 0);
        for (int dx = 0; dx < 18; dx++)
        for (int dz = 0; dz < 18; dz++)
        for (int dy = 0; dy < 18; dy++) {
            int i = (dx * 18 + dz) * 18 + dy;
            int x = ox - 1 + dx, y = y0 - 1 + dy, z = oz - 1 + dz;
            lc[i] = worldBlock(w, x, y, z);
            if (wantLight) llc[i] = (unsigned char)lightRawAt(w, x, y, z);
        }
    }
    #define LCB(X, Y, Z) lc[((((X) - ox + 1) * 18 + ((Z) - oz + 1)) * 18) + ((Y) - y0 + 1)]
    #define LLB(X, Y, Z) llc[((((X) - ox + 1) * 18 + ((Z) - oz + 1)) * 18) + ((Y) - y0 + 1)]

    for (int lx = 0; lx < CHUNK_SX; lx++)
    for (int lz = 0; lz < CHUNK_SZ; lz++)
    for (int y = y0; y < y1; y++) {
        int gx = ox + lx, gz = oz + lz;
        unsigned char id = (layer == 1) ? worldBlock(w, gx, y, gz) : LCB(gx, y, gz);

        if (layer == 1) {
            if (!isWaterId(id)) continue;
            if (out && n + 36 > cap) return -1;
            n = emitLiquid(w, gx, y, gz, id, out, n);
            continue;
        }
        else if (layer == 2) {
            if (!isLeaf(id)) continue;
        }
        else {
            if (id == BLOCK_AIR || isWaterId(id) || isLeaf(id)) continue;
            if (isLavaId(id)) {
                if (out && n + 72 > cap) return -1;
                n = emitLiquid(w, gx, y, gz, id, out, n);
                continue;
            }
        }

        if (layer == 0 && isOpaque(id)
            && isOpaque(LCB(gx - 1, y, gz)) && isOpaque(LCB(gx + 1, y, gz))
            && isOpaque(LCB(gx, y - 1, gz)) && isOpaque(LCB(gx, y + 1, gz))
            && isOpaque(LCB(gx, y, gz - 1)) && isOpaque(LCB(gx, y, gz + 1)))
            continue;

        if (isSign(id)) continue;

        if (layer == 0 && id == BLOCK_MELON_STEM) {
            if (out && n + 36 > cap) return -1;
            if (out) n = emitMelonStem(w, out, n, gx, y, gz, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            else n += 36;
            continue;
        }

        if (layer == 0 && id == BLOCK_WHEAT) {
            if (out && n + 48 > cap) return -1;
            if (out) emitCropRows(out, n, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            n += 48;
            continue;
        }

        if (layer == 0 && isPlant(id)) {
            if (out && n + 24 > cap) return -1;
            if (out) emitCross(out, n, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            n += 24;
            continue;
        }

        if (layer == 0 && isLadder(id)) {
            if (out && n + 12 > cap) return -1;
            if (out) n = emitLadder(out, n, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            else n += 12;
            continue;
        }

        if (layer == 0 && id == BLOCK_BED) {
            if (out && n + 36 > cap) return -1;
            if (out) n = emitBed(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            else n += 36;
            continue;
        }

        if (layer == 0 && isTorch(id)) {
            if (out && n + 36 > cap) return -1;
            if (out) n = emitTorch(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            else n += 36;
            continue;
        }

        if (layer == 0 && (isSlab(id) || isStairs(id))) {
            if (out && n + 108 > cap) return -1;
            n = isSlab(id) ? emitSlab(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n)
                           : emitStairs(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 0 && isPane(id)) {
            if (out && n + 72 > cap) return -1;
            n = emitPane(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 0 && isFence(id)) {
            if (out && n + 324 > cap) return -1;
            n = emitFence(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 3 && isDoor(id)) {
            if (out && n + 36 > cap) return -1;
            n = emitDoor(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 3 && isTrapdoor(id)) {
            if (out && n + 36 > cap) return -1;
            n = emitTrapdoor(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 0 && isFenceGate(id)) {
            if (out && n + 144 > cap) return -1;
            n = emitFenceGate(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 0 && id == BLOCK_CHEST) {
            if (out && n + 36 > cap) return -1;
            n = emitPartialBox(w, gx, y, gz, id, worldData(w, gx, y, gz),
                               1.0f/16.0f, 0.0f, 1.0f/16.0f, 15.0f/16.0f, 14.0f/16.0f, 15.0f/16.0f,
                               (1 << F_DOWN), 0, out, n, true, true);
            continue;
        }

        if (out && n + 36 > cap) return -1;

        for (int f = 0; f < 6; f++) {

            int nx = gx + kFaceNeighbor[f][0];
            int ny = y  + kFaceNeighbor[f][1];
            int nz = gz + kFaceNeighbor[f][2];
            unsigned char nb = LCB(nx, ny, nz);

            bool hide = isOpaque(nb);
            if (id == BLOCK_TOPSNOW && f == F_TOP) hide = false;
            if (nb == BLOCK_TOPSNOW && f == F_TOP) hide = true;

            if (hide || (isLeaf(id) && isLeaf(nb) && (leavesOpaque || leavesCull)) ||
                (id == BLOCK_TOPSNOW && nb == BLOCK_TOPSNOW) ||
                (id == BLOCK_CACTUS && nb == BLOCK_CACTUS) ||
                (id == BLOCK_GLASS && nb == BLOCK_GLASS) ||
                (id == BLOCK_ICE && nb == BLOCK_ICE)) continue;

            if (out) {
                int col, row; unsigned int tint;
                tileForBlock(id, worldData(w, gx, y, gz), f, &col, &row, &tint);
                if (id == BLOCK_GRASS && LCB(gx, y + 1, gz) == BLOCK_TOPSNOW) {
                    if (f != F_TOP && f != F_DOWN) { col = 4; row = 4; tint = 0xFFFFFFFFu; }
                }

                if (layer == 2 && leavesOpaque) col += 1;
                float u0 = col * TILE_UV, v0 = row * TILE_UV;

                unsigned int shade = kFaceShade[f];
                int faceBr = LLB(nx, ny, nz);
                if (lightEmit(id) > faceBr) faceBr = lightEmit(id);
                unsigned int color = mulColor(mulColor(shade, tint),
                                              g_brightColor[faceBr]);

                float th = (id == BLOCK_TOPSNOW) ? 0.125f
                         : (id == BLOCK_FARMLAND) ? 0.9375f
                         : 1.0f;

                float ix = 0.0f, iz = 0.0f;
                if (id == BLOCK_CACTUS) {
                    if (f == F_LEFT)         ix =  0.0625f;
                    else if (f == F_RIGHT)   ix = -0.0625f;
                    else if (f == F_BACK)    iz =  0.0625f;
                    else if (f == F_FORWARD) iz = -0.0625f;
                }

                static const int tri[6] = { 0, 1, 2, 2, 3, 0 };
                for (int t = 0; t < 6; t++) {
                    int k = tri[t];
                    const signed char* c = kFaceCorner[f][k];
                    float uv_v = kFaceUV[f][k][1];

                    if (id == BLOCK_TOPSNOW && f != F_TOP && f != F_DOWN) {
                        if (uv_v == 0) uv_v = 1.0f - th;
                    }

                    float bx = (float)(gx + c[0]) + ix + seamOff(c[0]);
                    float by = ((c[1] == 1) ? ((float)y + th) : (float)(y + c[1])) + seamOff(c[1]);
                    float bz = (float)(gz + c[2]) + iz + seamOff(c[2]);

                    const float TE = TILE_UV / 128.0f;
                    out[n + t].u = u0 + (TE + kFaceUV[f][k][0] * (TILE_UV - 2.0f * TE));
                    out[n + t].v = v0 + (TE + uv_v * (TILE_UV - 2.0f * TE));
                    out[n + t].color = color;

                    out[n + t].x = bx;
                    out[n + t].y = by;
                    out[n + t].z = bz;
                }
            }
            n += 6;
        }
    }
    #undef LCB
    #undef LLB
    return n;
}

int meshSection(const World* w, int ox, int oz, int y0, int y1,
                ChunkVertex* out0, ChunkVertex* out1, ChunkVertex* out2, ChunkVertex* out3,
                int cap0, int cap1, int cap2, int cap3, int* n0, int* n1, int* n2, int* n3,
                bool leavesOpaque, bool leavesCull) {
    int no = 0, nw = 0, nl = 0, nn = 0;

    unsigned char lc[18 * 18 * 18];
    unsigned char llc[18 * 18 * 18];
    for (int dx = 0; dx < 18; dx++)
    for (int dz = 0; dz < 18; dz++)
    for (int dy = 0; dy < 18; dy++) {
        int i = (dx * 18 + dz) * 18 + dy;
        int x = ox - 1 + dx, y = y0 - 1 + dy, z = oz - 1 + dz;
        lc[i] = worldBlock(w, x, y, z);
        llc[i] = (unsigned char)lightRawAt(w, x, y, z);
    }
    #define LCB(X, Y, Z) lc[((((X) - ox + 1) * 18 + ((Z) - oz + 1)) * 18) + ((Y) - y0 + 1)]
    #define LLB(X, Y, Z) llc[((((X) - ox + 1) * 18 + ((Z) - oz + 1)) * 18) + ((Y) - y0 + 1)]

    static const int kFaceStride[6] = { -324, 324, -1, 1, -18, 18 };

    const float TE = TILE_UV / 128.0f, TILE_INNER = TILE_UV - 2.0f * TE;

    for (int lx = 0; lx < CHUNK_SX; lx++)
    for (int lz = 0; lz < CHUNK_SZ; lz++)
    for (int y = y0; y < y1; y++) {
        int gx = ox + lx, gz = oz + lz;
        int base = (((lx + 1) * 18 + (lz + 1)) * 18) + (y - y0 + 1);
        unsigned char id = lc[base];
        if (id == BLOCK_AIR) continue;

        if (isOpaque(id)
            && isOpaque(lc[base + kFaceStride[0]]) && isOpaque(lc[base + kFaceStride[1]])
            && isOpaque(lc[base + kFaceStride[2]]) && isOpaque(lc[base + kFaceStride[3]])
            && isOpaque(lc[base + kFaceStride[4]]) && isOpaque(lc[base + kFaceStride[5]]))
            continue;
        if (isSign(id)) continue;

        if (isWaterId(id)) {
            if (nw + 36 > cap1) return -1;
            nw = emitLiquid(w, gx, y, gz, id, out1, nw);
            continue;
        }
        if (isLavaId(id)) {
            if (nn + 72 > cap3) return -1;
            nn = emitLiquid(w, gx, y, gz, id, out3, nn);
            continue;
        }
        if (id == BLOCK_MELON_STEM) {
            if (nn + 36 > cap3) return -1;
            nn = emitMelonStem(w, out3, nn, gx, y, gz, worldData(w, gx, y, gz), g_brightColor[llc[base]]);
            continue;
        }
        if (id == BLOCK_WHEAT) {
            if (nn + 48 > cap3) return -1;
            nn = emitCropRows(out3, nn, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[llc[base]]);
            continue;
        }
        if (isPlant(id)) {
            if (nn + 24 > cap3) return -1;
            emitCross(out3, nn, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[llc[base]]);
            nn += 24;
            continue;
        }

        if (isLadder(id)) {
            if (nn + 12 > cap3) return -1;
            nn = emitLadder(out3, nn, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[llc[base]]);
            continue;
        } else if (id == BLOCK_BED) {
            if (nn + 36 > cap3) return -1;
            nn = emitBed(w, gx, y, gz, id, worldData(w, gx, y, gz), out3, nn);
            continue;
        } else if (isTorch(id)) {
            if (nn + 36 > cap3) return -1;
            nn = emitTorch(w, gx, y, gz, id, worldData(w, gx, y, gz), out3, nn);
            continue;
        }

        if (isSlab(id) || isStairs(id)) {
            if (no + 108 > cap0) return -1;

            no = isSlab(id) ? emitSlab(w, gx, y, gz, id, worldData(w, gx, y, gz), out0, no)
                            : emitStairs(w, gx, y, gz, id, worldData(w, gx, y, gz), out0, no);
            continue;
        }

        if (isPane(id)) {
            if (nn + 72 > cap3) return -1;
            nn = emitPane(w, gx, y, gz, id, worldData(w, gx, y, gz), out3, nn);
            continue;
        }

        if (isFence(id)) {
            if (no + 324 > cap0) return -1;
            no = emitFence(w, gx, y, gz, id, worldData(w, gx, y, gz), out0, no);
            continue;
        }

        if (isDoor(id)) {
            if (nn + 36 > cap3) return -1;
            nn = emitDoor(w, gx, y, gz, id, worldData(w, gx, y, gz), out3, nn);
            continue;
        }

        if (isTrapdoor(id)) {
            if (nn + 36 > cap3) return -1;
            nn = emitTrapdoor(w, gx, y, gz, id, worldData(w, gx, y, gz), out3, nn);
            continue;
        }

        if (isFenceGate(id)) {
            if (no + 144 > cap0) return -1;
            no = emitFenceGate(w, gx, y, gz, id, worldData(w, gx, y, gz), out0, no);
            continue;
        }

        if (id == BLOCK_CHEST) {
            if (no + 36 > cap0) return -1;
            no = emitPartialBox(w, gx, y, gz, id, worldData(w, gx, y, gz),
                                1.0f/16.0f, 0.0f, 1.0f/16.0f, 15.0f/16.0f, 14.0f/16.0f, 15.0f/16.0f,
                                (1 << F_DOWN), 0, out0, no, true, true);
            continue;
        }

        bool leaf = isLeaf(id);
        bool leafTransparent = leaf && !leavesOpaque;
        bool leafOpaqueDst = leaf && !leafTransparent;
        bool noMip = (id == BLOCK_CACTUS) || isGlass(id) || leafTransparent;
        ChunkVertex* dst = leafOpaqueDst ? out2 : noMip ? out3 : out0;
        int nd = leafOpaqueDst ? nl : noMip ? nn : no;
        int cap = leafOpaqueDst ? cap2 : noMip ? cap3 : cap0;
        if (nd + 36 > cap) return -1;

        for (int f = 0; f < 6; f++) {
            int nbi = base + kFaceStride[f];
            unsigned char nb = lc[nbi];

            bool hide = isOpaque(nb);
            if (id == BLOCK_TOPSNOW && f == F_TOP) hide = false;
            if (nb == BLOCK_TOPSNOW && f == F_TOP) hide = true;

            if (hide || (isLeaf(id) && isLeaf(nb) && (leavesOpaque || leavesCull)) ||
                (id == BLOCK_TOPSNOW && nb == BLOCK_TOPSNOW) ||
                (id == BLOCK_CACTUS && nb == BLOCK_CACTUS) ||
                (id == BLOCK_GLASS && nb == BLOCK_GLASS) ||
                (id == BLOCK_ICE && nb == BLOCK_ICE)) continue;

            int col, row; unsigned int tint;
            tileForBlock(id, worldData(w, gx, y, gz), f, &col, &row, &tint);
            if (id == BLOCK_GRASS && lc[base + 1] == BLOCK_TOPSNOW) {
                if (f != F_TOP && f != F_DOWN) { col = 4; row = 4; tint = 0xFFFFFFFFu; }
            }

            if (leafOpaqueDst) col += 1;
            float u0 = col * TILE_UV, v0 = row * TILE_UV;

            unsigned int shade = kFaceShade[f];
            int faceBr = llc[nbi];
            if (lightEmit(id) > faceBr) faceBr = lightEmit(id);
            unsigned int color = mulColor(mulColor(shade, tint), g_brightColor[faceBr]);

            float th = (id == BLOCK_TOPSNOW) ? 0.125f
                     : (id == BLOCK_FARMLAND) ? 0.9375f
                     : 1.0f;
            float ix = 0.0f, iz = 0.0f;
            if (id == BLOCK_CACTUS) {
                if (f == F_LEFT)         ix =  0.0625f;
                else if (f == F_RIGHT)   ix = -0.0625f;
                else if (f == F_BACK)    iz =  0.0625f;
                else if (f == F_FORWARD) iz = -0.0625f;
            }

            static const int tri[6] = { 0, 1, 2, 2, 3, 0 };

            float cTE = TE, cInner = TILE_INNER;
            for (int t = 0; t < 6; t++) {
                int k = tri[t];
                const signed char* c = kFaceCorner[f][k];
                float uv_v = kFaceUV[f][k][1];
                if (id == BLOCK_TOPSNOW && f != F_TOP && f != F_DOWN) {
                    if (uv_v == 0) uv_v = 1.0f - th;
                }
                float bx = (float)(gx + c[0]) + ix + seamOff(c[0]);
                float by = ((c[1] == 1) ? ((float)y + th) : (float)(y + c[1])) + seamOff(c[1]);
                float bz = (float)(gz + c[2]) + iz + seamOff(c[2]);
                dst[nd + t].u = u0 + (cTE + kFaceUV[f][k][0] * cInner);
                dst[nd + t].v = v0 + (cTE + uv_v * cInner);
                dst[nd + t].color = color;
                dst[nd + t].x = bx;
                dst[nd + t].y = by;
                dst[nd + t].z = bz;
            }
            nd += 6;
        }
        if (leafOpaqueDst) nl = nd; else if (noMip) nn = nd; else no = nd;
    }
    #undef LCB
    #undef LLB
    *n0 = no; *n1 = nw; *n2 = nl; *n3 = nn;
    return 0;
}
