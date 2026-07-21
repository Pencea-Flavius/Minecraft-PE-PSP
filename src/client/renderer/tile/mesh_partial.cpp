
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"

static const float kFaceCornerF[6][4][3] = {
     { {0,0,1},{0,1,1},{0,1,0},{0,0,0} },
     { {1,0,0},{1,1,0},{1,1,1},{1,0,1} },
     { {0,0,0},{1,0,0},{1,0,1},{0,0,1} },
     { {0,1,1},{1,1,1},{1,1,0},{0,1,0} },
     { {0,1,0},{1,1,0},{1,0,0},{0,0,0} },
     { {0,0,1},{1,0,1},{1,1,1},{0,1,1} },
};

int boxBoundaryMask(float x0, float y0, float z0, float x1, float y1, float z1) {
    int m = 0;
    if (x0 <= 0.0f) m |= (1 << F_LEFT);
    if (x1 >= 1.0f) m |= (1 << F_RIGHT);
    if (y0 <= 0.0f) m |= (1 << F_DOWN);
    if (y1 >= 1.0f) m |= (1 << F_TOP);
    if (z0 <= 0.0f) m |= (1 << F_BACK);
    if (z1 >= 1.0f) m |= (1 << F_FORWARD);
    return m;
}

int emitPartialBox(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data,
                   float x0, float y0, float z0, float x1, float y1, float z1,
                   int boundaryMask, int hiddenFaces, ChunkVertex* out, int n, bool fixUV,
                   bool fullTileUV) {
    for (int f = 0; f < 6; f++) {
        bool hide = false;

        if (hiddenFaces & (1 << f)) {
            hide = true;
        } else if (boundaryMask & (1 << f)) {
            int nx = gx + kFaceNeighbor[f][0];
            int ny = y  + kFaceNeighbor[f][1];
            int nz = gz + kFaceNeighbor[f][2];
            unsigned char nb = worldBlock(w, nx, ny, nz);

            hide = isOpaque(nb);

            if (isSlab(id) && nb == id) {
                unsigned char nbData = worldData(w, nx, ny, nz);
                if ((data & SLAB_TOP_SLOT_BIT) == (nbData & SLAB_TOP_SLOT_BIT)) {

                    if (f != F_TOP && f != F_DOWN) hide = true;
                }
            }

            if (isPane(id) && isPane(nb)) {
                if (f != F_TOP && f != F_DOWN) hide = true;
            }
        }

        if (hide) continue;

        if (out) {
            int col, row; unsigned int tint;
            tileForBlock(id, data, f, &col, &row, &tint);
            float u0 = col * TILE_UV, v0 = row * TILE_UV;

            int faceBr;
            unsigned int shade;

            if (isPane(id)) {
                shade = 0xFFFFFFFFu;
                faceBr = lightRawAt(w, gx, y, gz);
            }

            else if ((boundaryMask & (1 << f)) || f == F_DOWN) {
                int nx = gx + kFaceNeighbor[f][0];
                int ny = y  + kFaceNeighbor[f][1];
                int nz = gz + kFaceNeighbor[f][2];
                shade = kFaceShade[f];
                faceBr = lightRawAt(w, nx, ny, nz);
            } else {
                shade = kFaceShade[f];
                faceBr = lightRawAt(w, gx, y, gz);
            }
            if (lightEmit(id) > faceBr) faceBr = lightEmit(id);
            if (lightEmit(id) > 0) shade = 0xFFFFFFFFu;

            unsigned int color = mulColor(mulColor(shade, tint), g_brightColor[faceBr]);

            static const int tri[6] = { 0, 1, 2, 2, 3, 0 };
            for (int t = 0; t < 6; t++) {
                int k = tri[t];

                float cx = kFaceCornerF[f][k][0];
                float cy = kFaceCornerF[f][k][1];
                float cz = kFaceCornerF[f][k][2];

                float bx = cx * x1 + (1.0f - cx) * x0;
                float by = cy * y1 + (1.0f - cy) * y0;
                float bz = cz * z1 + (1.0f - cz) * z0;

                float ux0 = fullTileUV ? 0.0f : x0, ux1 = fullTileUV ? 1.0f : x1;
                float uy0 = fullTileUV ? 0.0f : y0, uy1 = fullTileUV ? 1.0f : y1;
                float uz0 = fullTileUV ? 0.0f : z0, uz1 = fullTileUV ? 1.0f : z1;
                float uv_u, uv_v;
                if (f == F_TOP || f == F_DOWN) {
                    uv_u = (cx == 0.0f) ? ux0 : ux1;
                    uv_v = (cz == 0.0f) ? uz0 : uz1;
                } else {
                    uv_v = 1.0f - ((cy == 0.0f) ? uy0 : uy1);

                    bool mirror = (f == F_RIGHT || f == F_BACK);
                    if (f == F_LEFT || f == F_RIGHT) {
                        uv_u = ((cz == 0.0f) != mirror) ? uz0 : uz1;
                    } else {
                        uv_u = ((cx == 0.0f) != mirror) ? ux0 : ux1;
                    }
                    if (uv_u < 0.0f) uv_u = 0.0f;
                    if (uv_u > 1.0f) uv_u = 1.0f;
                    if (uv_v < 0.0f) uv_v = 0.0f;
                    if (uv_v > 1.0f) uv_v = 1.0f;
                }

                if (id == BLOCK_TOPSNOW && f != F_TOP && f != F_DOWN) {
                    if (uv_v == 0) uv_v = 1.0f - y1;
                }

                if (fixUV) {

                    const float TE = TILE_UV / 128.0f;
                    float u_inset = (uv_u == 0.0f) ? TE : (uv_u == 1.0f) ? -TE : 0.0f;
                    float v_inset = (uv_v == 0.0f) ? TE : (uv_v == 1.0f) ? -TE : 0.0f;
                    out[n + t].u = u0 + uv_u * TILE_UV + u_inset;
                    out[n + t].v = v0 + uv_v * TILE_UV + v_inset;
                } else {
                    out[n + t].u = u0 + uv_u * TILE_UV;
                    out[n + t].v = v0 + uv_v * TILE_UV;
                }
                out[n + t].color = color;
                out[n + t].x = gx + bx;
                out[n + t].y = y + by;
                out[n + t].z = gz + bz;
            }
        }
        n += 6;
    }
    return n;
}

int emitSlab(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {
    float y0 = (data & SLAB_TOP_SLOT_BIT) ? 0.5f : 0.0f;
    float y1 = (data & SLAB_TOP_SLOT_BIT) ? 1.0f : 0.5f;
    int mask = (1 << F_LEFT) | (1 << F_RIGHT) | (1 << F_BACK) | (1 << F_FORWARD);
    if (y0 == 0.0f) mask |= (1 << F_DOWN);
    if (y1 == 1.0f) mask |= (1 << F_TOP);
    return emitPartialBox(w, gx, y, gz, id, data, 0.0f, y0, 0.0f, 1.0f, y1, 1.0f, mask, 0, out, n, true);
}

int emitStairs(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {

    float b[3][6];
    int nb = stairShapeBoxes(w, gx, y, gz, data, b);
    bool upsideDown = (data & STAIR_UPSIDEDOWN_BIT) != 0;
    int dir = data & STAIR_DIR_MASK;

    for (int i = 0; i < nb; i++) {
        float x0 = b[i][0], y0 = b[i][1], z0 = b[i][2];
        float x1 = b[i][3], y1 = b[i][4], z1 = b[i][5];
        int mask = boxBoundaryMask(x0, y0, z0, x1, y1, z1);
        int hidden = 0;
        if (i > 0) {

            hidden = upsideDown ? (1 << F_TOP) : (1 << F_DOWN);
        }
        if (i == 2) {

            if      (dir == STAIR_DIR_EAST)  hidden |= (1 << F_RIGHT);
            else if (dir == STAIR_DIR_WEST)  hidden |= (1 << F_LEFT);
            else if (dir == STAIR_DIR_SOUTH) hidden |= (1 << F_FORWARD);
            else if (dir == STAIR_DIR_NORTH) hidden |= (1 << F_BACK);
        }
        n = emitPartialBox(w, gx, y, gz, id, data, x0, y0, z0, x1, y1, z1, mask, hidden, out, n, true);
    }
    return n;
}

static inline bool paneAttachsTo(unsigned char nb) {
    return isSolidPhys(nb) || isPane(nb);
}

int emitPane(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {
    bool north = paneAttachsTo(worldBlock(w, gx,     y, gz - 1));
    bool south = paneAttachsTo(worldBlock(w, gx,     y, gz + 1));
    bool west  = paneAttachsTo(worldBlock(w, gx - 1, y, gz    ));
    bool east  = paneAttachsTo(worldBlock(w, gx + 1, y, gz    ));
    bool isolated = !north && !south && !west && !east;

    bool hasX = isolated || west || east || (!north && !south);
    if (hasX) {
        float ax0, ax1;
        int bMask = (1 << F_DOWN) | (1 << F_TOP);
        if (west && east)       { ax0 = 0.0f; ax1 = 1.0f; bMask |= (1 << F_LEFT) | (1 << F_RIGHT); }
        else if (west && !east) { ax0 = 0.0f; ax1 = 0.5f; bMask |= (1 << F_LEFT); }
        else if (!west && east) { ax0 = 0.5f; ax1 = 1.0f; bMask |= (1 << F_RIGHT); }
        else                    { ax0 = 0.0f; ax1 = 1.0f; bMask |= (1 << F_LEFT) | (1 << F_RIGHT); }
        n = emitPartialBox(w, gx, y, gz, id, data,
                           ax0, 0.0f, 7.0f/16.0f, ax1, 1.0f, 9.0f/16.0f,
                           bMask, 0, out, n);
    }

    bool hasZ = isolated || north || south || (!west && !east);
    if (hasZ) {
        float az0, az1;
        int bMask = (1 << F_DOWN) | (1 << F_TOP);
        if (north && south)        { az0 = 0.0f; az1 = 1.0f; bMask |= (1 << F_BACK) | (1 << F_FORWARD); }
        else if (north && !south)  { az0 = 0.0f; az1 = 0.5f; bMask |= (1 << F_BACK); }
        else if (!north && south)  { az0 = 0.5f; az1 = 1.0f; bMask |= (1 << F_FORWARD); }
        else                       { az0 = 0.0f; az1 = 1.0f; bMask |= (1 << F_BACK) | (1 << F_FORWARD); }
        n = emitPartialBox(w, gx, y, gz, id, data,
                           7.0f/16.0f, 0.0f, az0, 9.0f/16.0f, 1.0f, az1,
                           bMask, 0, out, n);
    }

    return n;
}

int emitFence(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {
    bool north = connectsFence(worldBlock(w, gx,     y, gz - 1));
    bool south = connectsFence(worldBlock(w, gx,     y, gz + 1));
    bool west  = connectsFence(worldBlock(w, gx - 1, y, gz    ));
    bool east  = connectsFence(worldBlock(w, gx + 1, y, gz    ));

    if (y >= 128) {
        west = true;
        east = true;
    }

    int postBMask = (1<<F_DOWN) | (1<<F_TOP);
    n = emitPartialBox(w, gx, y, gz, id, data,
                       6.0f/16.0f, 0.0f, 6.0f/16.0f,
                       10.0f/16.0f, 1.0f, 10.0f/16.0f,
                       postBMask, 0, out, n);

    if (west) {
        int bMask = (1<<F_LEFT);
        n = emitPartialBox(w, gx, y, gz, id, data,
                           0.0f, 7.0f/16.0f, 7.0f/16.0f,
                           0.5f, 9.0f/16.0f, 9.0f/16.0f,
                           bMask, 0, out, n);
        n = emitPartialBox(w, gx, y, gz, id, data,
                           0.0f, 12.0f/16.0f, 7.0f/16.0f,
                           0.5f, 14.0f/16.0f, 9.0f/16.0f,
                           bMask, 0, out, n);
    }

    if (east) {
        int bMask = (1<<F_RIGHT);
        n = emitPartialBox(w, gx, y, gz, id, data,
                           0.5f, 7.0f/16.0f, 7.0f/16.0f,
                           1.0f, 9.0f/16.0f, 9.0f/16.0f,
                           bMask, 0, out, n);
        n = emitPartialBox(w, gx, y, gz, id, data,
                           0.5f, 12.0f/16.0f, 7.0f/16.0f,
                           1.0f, 14.0f/16.0f, 9.0f/16.0f,
                           bMask, 0, out, n);
    }

    if (north) {
        int bMask = (1<<F_BACK);
        n = emitPartialBox(w, gx, y, gz, id, data,
                           7.0f/16.0f, 7.0f/16.0f, 0.0f,
                           9.0f/16.0f, 9.0f/16.0f, 0.5f,
                           bMask, 0, out, n);
        n = emitPartialBox(w, gx, y, gz, id, data,
                           7.0f/16.0f, 12.0f/16.0f, 0.0f,
                           9.0f/16.0f, 14.0f/16.0f, 0.5f,
                           bMask, 0, out, n);
    }

    if (south) {
        int bMask = (1<<F_FORWARD);
        n = emitPartialBox(w, gx, y, gz, id, data,
                           7.0f/16.0f, 7.0f/16.0f, 0.5f,
                           9.0f/16.0f, 9.0f/16.0f, 1.0f,
                           bMask, 0, out, n);
        n = emitPartialBox(w, gx, y, gz, id, data,
                           7.0f/16.0f, 12.0f/16.0f, 0.5f,
                           9.0f/16.0f, 14.0f/16.0f, 1.0f,
                           bMask, 0, out, n);
    }

    return n;
}

int emitDoor(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {
    float r = 3.0f / 16.0f;
    float bx0 = 0.0f, bx1 = 1.0f, bz0 = 0.0f, bz1 = 1.0f;

    bool isUpper = (data & 8) != 0;
    int lowerData = isUpper ? worldData(w, gx, y - 1, gz) : data;
    int upperData = isUpper ? data : worldData(w, gx, y + 1, gz);
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

    if      (shapeDir == 0) { bx0 = 0; bx1 = 1;   bz0 = 0;   bz1 = r;   }
    else if (shapeDir == 1) { bx0 = 1-r; bx1 = 1; bz0 = 0;   bz1 = 1;   }
    else if (shapeDir == 2) { bx0 = 0; bx1 = 1;   bz0 = 1-r; bz1 = 1;   }
    else if (shapeDir == 3) { bx0 = 0; bx1 = r;   bz0 = 0;   bz1 = 1;   }

    static const int kFlipOpen[4]   = { F_BACK,  F_RIGHT,   F_FORWARD, F_LEFT };
    static const int kFlipClosed[4] = { F_RIGHT, F_FORWARD, F_LEFT,    F_BACK };
    int flipFace = open ? kFlipOpen[dir] : kFlipClosed[dir];

    bool flipNorth = (flipFace == F_BACK);
    bool flipSouth = (flipFace == F_FORWARD);
    bool flipWest  = (flipFace == F_LEFT);
    bool flipEast  = (flipFace == F_RIGHT);
    if (!open && rightHinge) {
        flipNorth = !flipNorth; flipSouth = !flipSouth;
        flipWest  = !flipWest;  flipEast  = !flipEast;
    }

    int selfBr = lightRawAt(w, gx, y, gz);
    if (lightEmit(id) > selfBr) selfBr = lightEmit(id);

    auto emitFace = [&](int f, unsigned int shade, bool isBoundary, int nx, int ny, int nz,
                        float p0x, float p0y, float p0z,
                        float p1x, float p1y, float p1z,
                        float p2x, float p2y, float p2z,
                        float p3x, float p3y, float p3z,
                        float ua, float va, float ub, float vb) {
        if (!out) { n += 6; return; }

        int col, row; unsigned int tint;
        tileForBlock(id, data, f, &col, &row, &tint);

        int br;
        if (isBoundary) {
            br = lightRawAt(w, nx, ny, nz);
        } else {
            br = selfBr;
        }
        if (lightEmit(id) > 0) { shade = 0xFFFFFFFFu; br = 15; }
        unsigned int color = mulColor(mulColor(shade, tint), g_brightColor[br]);

        float tu = col * TILE_UV;
        float tv = row * TILE_UV;

        const float TE = TILE_UV / 128.0f;
        float u_inset0 = (ua == 0.0f) ? TE : (ua == 1.0f) ? -TE : 0.0f;
        float u_inset1 = (ub == 0.0f) ? TE : (ub == 1.0f) ? -TE : 0.0f;
        float v_inset0 = (va == 0.0f) ? TE : (va == 1.0f) ? -TE : 0.0f;
        float v_inset1 = (vb == 0.0f) ? TE : (vb == 1.0f) ? -TE : 0.0f;

        float u0c = tu + ua * TILE_UV + u_inset0;
        float u1c = tu + ub * TILE_UV + u_inset1;
        float v0c = tv + va * TILE_UV + v_inset0;
        float v1c = tv + vb * TILE_UV + v_inset1;

        out[n+0] = { u0c, v0c, color, gx+p0x, y+p0y, gz+p0z };
        out[n+1] = { u1c, v0c, color, gx+p1x, y+p1y, gz+p1z };
        out[n+2] = { u1c, v1c, color, gx+p2x, y+p2y, gz+p2z };
        out[n+3] = { u1c, v1c, color, gx+p2x, y+p2y, gz+p2z };
        out[n+4] = { u0c, v1c, color, gx+p3x, y+p3y, gz+p3z };
        out[n+5] = { u0c, v0c, color, gx+p0x, y+p0y, gz+p0z };
        n += 6;
    };

    const float CAP_U0 = 1.0f / 16.0f, CAP_U1 = 15.0f / 16.0f;
    const float CAP_V0 = 1.0f / 16.0f, CAP_V1 = 4.0f / 16.0f;
    bool capThinX = (bx1 - bx0) < 0.5f;

    if (isUpper) {
        unsigned char abv = worldBlock(w, gx, y+1, gz);
        if (!isOpaque(abv)) {

            if (capThinX)
                emitFace(F_TOP, kFaceShade[F_TOP], true, gx, y+1, gz,
                    bx1,1,bz1, bx1,1,bz0, bx0,1,bz0, bx0,1,bz1,
                    CAP_U0, CAP_V0, CAP_U1, CAP_V1);
            else
                emitFace(F_TOP, kFaceShade[F_TOP], true, gx, y+1, gz,
                    bx0,1,bz1, bx1,1,bz1, bx1,1,bz0, bx0,1,bz0,
                    CAP_U0, CAP_V0, CAP_U1, CAP_V1);
        }
    }

    if (!isUpper) {
        unsigned char blw = worldBlock(w, gx, y-1, gz);
        if (!isOpaque(blw)) {
            if (capThinX)
                emitFace(F_DOWN, kFaceShade[F_DOWN], true, gx, y-1, gz,
                    bx1,0,bz0, bx1,0,bz1, bx0,0,bz1, bx0,0,bz0,
                    CAP_U0, CAP_V0, CAP_U1, CAP_V1);
            else
                emitFace(F_DOWN, kFaceShade[F_DOWN], true, gx, y-1, gz,
                    bx0,0,bz0, bx1,0,bz0, bx1,0,bz1, bx0,0,bz1,
                    CAP_U0, CAP_V0, CAP_U1, CAP_V1);
        }
    }

    const float CANT_R = 3.0f / 16.0f;
    bool thinX = (bx1 - bx0) < 0.5f;
    bool thinZ = (bz1 - bz0) < 0.5f;
    bool hingeAtHighX = !flipNorth;
    bool hingeAtHighZ =  flipWest;

    if (bz0 > 0.0f || !isOpaque(worldBlock(w, gx, y, gz-1))) {
        bool boundary = (bz0 == 0.0f);
        float uA = flipNorth ? bx0 : bx1;
        float uB = flipNorth ? bx1 : bx0;
        if (thinX) { bool hp = !hingeAtHighZ; uA = hp ? 0.0f : 1.0f - CANT_R; uB = hp ? CANT_R : 1.0f; }
        emitFace(F_BACK, kFaceShade[F_BACK], boundary, gx, y, gz-1,
            bx0,1,bz0, bx1,1,bz0, bx1,0,bz0, bx0,0,bz0,
            uA, 0.0f, uB, 1.0f);
    }

    if (bz1 < 1.0f || !isOpaque(worldBlock(w, gx, y, gz+1))) {
        bool boundary = (bz1 == 1.0f);
        float uA = flipSouth ? bx0 : bx1;
        float uB = flipSouth ? bx1 : bx0;
        if (thinX) { bool hp = hingeAtHighZ; uA = hp ? 0.0f : 1.0f - CANT_R; uB = hp ? CANT_R : 1.0f; }
        emitFace(F_FORWARD, kFaceShade[F_FORWARD], boundary, gx, y, gz+1,
            bx1,1,bz1, bx0,1,bz1, bx0,0,bz1, bx1,0,bz1,
            uA, 0.0f, uB, 1.0f);
    }

    if (bx0 > 0.0f || !isOpaque(worldBlock(w, gx-1, y, gz))) {
        bool boundary = (bx0 == 0.0f);

        float uA = flipWest ? bz0 : bz1;
        float uB = flipWest ? bz1 : bz0;
        if (thinZ) { bool hp = !hingeAtHighX; uA = hp ? 0.0f : 1.0f - CANT_R; uB = hp ? CANT_R : 1.0f; }
        emitFace(F_LEFT, kFaceShade[F_LEFT], boundary, gx-1, y, gz,
            bx0,1,bz1, bx0,1,bz0, bx0,0,bz0, bx0,0,bz1,
            uA, 0.0f, uB, 1.0f);
    }

    if (bx1 < 1.0f || !isOpaque(worldBlock(w, gx+1, y, gz))) {
        bool boundary = (bx1 == 1.0f);

        float uA = flipEast ? bz0 : bz1;
        float uB = flipEast ? bz1 : bz0;
        if (thinZ) { bool hp = hingeAtHighX; uA = hp ? 0.0f : 1.0f - CANT_R; uB = hp ? CANT_R : 1.0f; }
        emitFace(F_RIGHT, kFaceShade[F_RIGHT], boundary, gx+1, y, gz,
            bx1,1,bz0, bx1,1,bz1, bx1,0,bz1, bx1,0,bz0,
            uA, 0.0f, uB, 1.0f);
    }

    return n;
}

int emitTrapdoor(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {
    float r = 3.0f / 16.0f;
    float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f;
    float x1 = 1.0f, y1 = r, z1 = 1.0f;

    bool open = (data & 4) != 0;
    if (open) {
        if ((data & 3) == 0) { x0 = 0; x1 = 1; y0 = 0; y1 = 1; z0 = 1 - r; z1 = 1; }
        else if ((data & 3) == 1) { x0 = 0; x1 = 1; y0 = 0; y1 = 1; z0 = 0; z1 = r; }
        else if ((data & 3) == 2) { x0 = 1 - r; x1 = 1; y0 = 0; y1 = 1; z0 = 0; z1 = 1; }
        else if ((data & 3) == 3) { x0 = 0; x1 = r; y0 = 0; y1 = 1; z0 = 0; z1 = 1; }
    }

    return emitPartialBox(w, gx, y, gz, id, data, x0, y0, z0, x1, y1, z1,
                          boxBoundaryMask(x0, y0, z0, x1, y1, z1), 0, out, n, true);
}

static inline int emitGateBox(const World* w, int gx, int y, int gz,
                              unsigned char id, unsigned char data,
                              float x0, float y0, float z0, float x1, float y1, float z1,
                              ChunkVertex* out, int n) {
    return emitPartialBox(w, gx, y, gz, id, data, x0, y0, z0, x1, y1, z1,
                          boxBoundaryMask(x0, y0, z0, x1, y1, z1), 0, out, n);
}

int emitFenceGate(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {
    bool open = (data & 4) != 0;
    int dir = data & 3;
    bool ns = (dir == 1 || dir == 3);

    float h00 = 6.0f / 16.0f;
    float h01 = 9.0f / 16.0f;
    float h10 = 12.0f / 16.0f;
    float h11 = 15.0f / 16.0f;
    float h20 = 5.0f / 16.0f;
    float h21 = 16.0f / 16.0f;

    if (ns) {
        n = emitGateBox(w, gx, y, gz, id, data, 7.0f/16.0f, h20, 0.0f/16.0f, 9.0f/16.0f, h21, 2.0f/16.0f, out, n);
        n = emitGateBox(w, gx, y, gz, id, data, 7.0f/16.0f, h20, 14.0f/16.0f, 9.0f/16.0f, h21, 16.0f/16.0f, out, n);
    } else {
        n = emitGateBox(w, gx, y, gz, id, data, 0.0f/16.0f, h20, 7.0f/16.0f, 2.0f/16.0f, h21, 9.0f/16.0f, out, n);
        n = emitGateBox(w, gx, y, gz, id, data, 14.0f/16.0f, h20, 7.0f/16.0f, 16.0f/16.0f, h21, 9.0f/16.0f, out, n);
    }

    if (!open) {
        if (ns) {

            n = emitGateBox(w, gx, y, gz, id, data, 7.0f/16.0f, h00, 6.0f/16.0f, 9.0f/16.0f, h11, 8.0f/16.0f, out, n);
            n = emitGateBox(w, gx, y, gz, id, data, 7.0f/16.0f, h00, 8.0f/16.0f, 9.0f/16.0f, h11, 10.0f/16.0f, out, n);

            n = emitGateBox(w, gx, y, gz, id, data, 7.0f/16.0f, h00, 10.0f/16.0f, 9.0f/16.0f, h01, 14.0f/16.0f, out, n);
            n = emitGateBox(w, gx, y, gz, id, data, 7.0f/16.0f, h10, 10.0f/16.0f, 9.0f/16.0f, h11, 14.0f/16.0f, out, n);
            n = emitGateBox(w, gx, y, gz, id, data, 7.0f/16.0f, h00, 2.0f/16.0f, 9.0f/16.0f, h01, 6.0f/16.0f, out, n);
            n = emitGateBox(w, gx, y, gz, id, data, 7.0f/16.0f, h10, 2.0f/16.0f, 9.0f/16.0f, h11, 6.0f/16.0f, out, n);
        } else {

            n = emitGateBox(w, gx, y, gz, id, data, 6.0f/16.0f, h00, 7.0f/16.0f, 8.0f/16.0f, h11, 9.0f/16.0f, out, n);
            n = emitGateBox(w, gx, y, gz, id, data, 8.0f/16.0f, h00, 7.0f/16.0f, 10.0f/16.0f, h11, 9.0f/16.0f, out, n);

            n = emitGateBox(w, gx, y, gz, id, data, 10.0f/16.0f, h00, 7.0f/16.0f, 14.0f/16.0f, h01, 9.0f/16.0f, out, n);
            n = emitGateBox(w, gx, y, gz, id, data, 10.0f/16.0f, h10, 7.0f/16.0f, 14.0f/16.0f, h11, 9.0f/16.0f, out, n);
            n = emitGateBox(w, gx, y, gz, id, data, 2.0f/16.0f, h00, 7.0f/16.0f, 6.0f/16.0f, h01, 9.0f/16.0f, out, n);
            n = emitGateBox(w, gx, y, gz, id, data, 2.0f/16.0f, h10, 7.0f/16.0f, 6.0f/16.0f, h11, 9.0f/16.0f, out, n);
        }
    } else {

        bool pos = (dir == 3 || dir == 0);
        float postA = (pos ? 13.0f : 1.0f) / 16.0f;
        float postB = (pos ? 15.0f : 3.0f) / 16.0f;
        float railA = (pos ?  9.0f : 3.0f) / 16.0f;
        float railB = (pos ? 13.0f : 7.0f) / 16.0f;

        const float e0 = 0.0f, e1 = 2.0f/16.0f, e2 = 14.0f/16.0f, e3 = 16.0f/16.0f;
        for (int s = 0; s < 2; s++) {
            float p0 = s ? e2 : e0, p1 = s ? e3 : e1;
            if (ns) {
                n = emitGateBox(w, gx, y, gz, id, data, postA, h00, p0, postB, h11, p1, out, n);
                n = emitGateBox(w, gx, y, gz, id, data, railA, h00, p0, railB, h01, p1, out, n);
                n = emitGateBox(w, gx, y, gz, id, data, railA, h10, p0, railB, h11, p1, out, n);
            } else {
                n = emitGateBox(w, gx, y, gz, id, data, p0, h00, postA, p1, h11, postB, out, n);
                n = emitGateBox(w, gx, y, gz, id, data, p0, h00, railA, p1, h01, railB, out, n);
                n = emitGateBox(w, gx, y, gz, id, data, p0, h10, railA, p1, h11, railB, out, n);
            }
        }
    }
    return n;
}

#include <math.h>

int emitBed(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {

    static const float kFaceCornerF[6][4][3] = {
         { {0,0,1},{0,1,1},{0,1,0},{0,0,0} },
         { {1,0,0},{1,1,0},{1,1,1},{1,0,1} },
         { {0,0,0},{1,0,0},{1,0,1},{0,0,1} },
         { {0,1,1},{1,1,1},{1,1,0},{0,1,0} },
         { {0,1,0},{1,1,0},{1,0,0},{0,0,0} },
         { {0,0,1},{1,0,1},{1,1,1},{0,1,1} },
    };

    static const unsigned char kFaceUV[6][4][2] = {
         { {1,1},{1,0},{0,0},{0,1} },
         { {1,1},{1,0},{0,0},{0,1} },
         { {0,0},{1,0},{1,1},{0,1} },
         { {0,1},{1,1},{1,0},{0,0} },
         { {1,0},{0,0},{0,1},{1,1} },
         { {0,1},{1,1},{1,0},{0,0} },
    };

    const float BED_HEIGHT = 9.0f / 16.0f;

    static const int tri[6] = { 0, 1, 2, 2, 3, 0 };

    if (!out) return n + 36;

    for (int f = 0; f < 6; f++) {

        int nx = gx + kFaceNeighbor[f][0];
        int ny = y  + kFaceNeighbor[f][1];
        int nz = gz + kFaceNeighbor[f][2];
        unsigned char nb = worldBlock(w, nx, ny, nz);

        if (isOpaque(nb) && f != F_TOP && f != F_DOWN) continue;

        if (f == F_TOP && isOpaque(nb)) continue;

        if (id == BLOCK_BED) {
            int dir = data & 3;
            bool isHead = (data & 8) != 0;

            if (dir == 0) {
                if (isHead && f == F_BACK) continue;
                if (!isHead && f == F_FORWARD) continue;
            } else if (dir == 1) {
                if (isHead && f == F_RIGHT) continue;
                if (!isHead && f == F_LEFT) continue;
            } else if (dir == 2) {
                if (isHead && f == F_FORWARD) continue;
                if (!isHead && f == F_BACK) continue;
            } else if (dir == 3) {
                if (isHead && f == F_LEFT) continue;
                if (!isHead && f == F_RIGHT) continue;
            }
        }

        int col, row; unsigned int tint;
        tileForBlock(id, data, f, &col, &row, &tint);
        float u0 = col * TILE_UV, v0 = row * TILE_UV;

        unsigned int shade = kFaceShade[f];

        int faceBr = (f == F_DOWN) ? lightRawAt(w, gx, y, gz)
                                   : lightRawAt(w, nx, ny, nz);
        if (lightEmit(id) > faceBr) faceBr = lightEmit(id);
        unsigned int color = mulColor(mulColor(shade, tint), g_brightColor[faceBr]);

        for (int t = 0; t < 6; t++) {
            int k = tri[t];
            float cx = kFaceCornerF[f][k][0];
            float cy = kFaceCornerF[f][k][1];
            float cz = kFaceCornerF[f][k][2];

            float bx = cx;
            float by = (cy == 1.0f) ? BED_HEIGHT : 0.0f;
            if (id == BLOCK_BED && f == F_DOWN) {
                by = 3.0f / 16.0f;
            }
            float bz = cz;

            float uv_u, uv_v;
            float u_inset = 0.0f, v_inset = 0.0f;

            const float TE = TILE_UV / 128.0f;

            if (f == F_TOP || f == F_DOWN) {
                if (f == F_TOP && id == BLOCK_BED) {
                    int dir = data & 3;

                    static const float bedTopUV[4][4][2] = {

                        { {1, 0}, {1, 1}, {0, 1}, {0, 0} },

                        { {1, 1}, {0, 1}, {0, 0}, {1, 0} },

                        { {0, 1}, {0, 0}, {1, 0}, {1, 1} },

                        { {0, 0}, {1, 0}, {1, 1}, {0, 1} }
                    };
                    uv_u = bedTopUV[dir][k][0];
                    uv_v = bedTopUV[dir][k][1];
                } else {
                    uv_u = kFaceUV[f][k][0];
                    uv_v = kFaceUV[f][k][1];
                }
                u_inset = (uv_u == 0.0f) ? TE : -TE;
                v_inset = (uv_v == 0.0f) ? TE : -TE;
            } else {

                uv_u = (float)kFaceUV[f][k][0];
                float rawV = (float)kFaceUV[f][k][1];
                u_inset = (kFaceUV[f][k][0] == 0) ? TE : -TE;
                v_inset = (kFaceUV[f][k][1] == 0) ? TE : -TE;

                if (id == BLOCK_BED) {

                    static const int kBedFlipFace[4] = { F_RIGHT, F_FORWARD, F_LEFT, F_BACK };
                    if (f == kBedFlipFace[data & 3]) {
                        uv_u = 1.0f - uv_u;
                        u_inset = -u_inset;
                    }
                    uv_v = (1.0f - BED_HEIGHT) + rawV * BED_HEIGHT;
                } else {
                    uv_v = rawV * BED_HEIGHT;
                }
            }

            out[n + t].u = u0 + uv_u * TILE_UV + u_inset;
            out[n + t].v = v0 + uv_v * TILE_UV + v_inset;
            out[n + t].color = color;
            out[n + t].x = gx + bx;
            out[n + t].y = y  + by;
            out[n + t].z = gz + bz;
        }
        n += 6;
    }
    return n;
}

int emitTorch(const World* w, int gx, int y, int gz, unsigned char id, unsigned char data, ChunkVertex* out, int n) {
    if (!out) return n + 36;

    int startN = n;

    n = emitPartialBox(w, gx, y, gz, id, data,
                       7.0f/16.0f, 0.0f, 7.0f/16.0f,
                       9.0f/16.0f, 10.0f/16.0f, 9.0f/16.0f,
                       0, 0, out, n);

    float tdx = 0.5f, tdy = 0.0f, tdz = 0.5f;
    float tiltX = 0.0f, tiltZ = 0.0f;

    float angle = 0.45f;
    if (data == 1)      { tdx = 0.0f; tdy = 0.2f; tiltZ = -angle; }
    else if (data == 2) { tdx = 1.0f; tdy = 0.2f; tiltZ = angle; }
    else if (data == 3) { tdz = 0.0f; tdy = 0.2f; tiltX = angle; }
    else if (data == 4) { tdz = 1.0f; tdy = 0.2f; tiltX = -angle; }

    float cZ = cosf(tiltZ), sZ = sinf(tiltZ);
    float cX = cosf(tiltX), sX = sinf(tiltX);

    for (int i = startN; i < n; i++) {

        if (i >= startN + 12 && i < startN + 18) {
            out[i].v += (7.0f / 256.0f);
        }

        else if (i >= startN + 18 && i < startN + 24) {
            out[i].v -= (1.0f / 256.0f);
        }

        float lx = out[i].x - (gx + 0.5f);
        float ly = out[i].y - y;
        float lz = out[i].z - (gz + 0.5f);

        float rx = lx * cZ - ly * sZ;
        float ry1 = lx * sZ + ly * cZ;
        float ry2 = ry1 * cX - lz * sX;
        float rz = ry1 * sX + lz * cX;

        out[i].x = gx + tdx + rx;
        out[i].y = y + tdy + ry2;
        out[i].z = gz + tdz + rz;
    }

    return n;
}
