
#include "world/level/chunk/chunk.h"
#include "world/level/chunk/mesh_sink.h"
#include "client/renderer/tile/mesh_light.h"
#include "world/level/world.h"
#include "world/level/tile/fire.h"
#include <string.h>

static const unsigned char kFaceUV[6][4][2] = {

     { {1,1},{1,0},{0,0},{0,1} },
     { {1,1},{1,0},{0,0},{0,1} },
     { {0,0},{1,0},{1,1},{0,1} },
     { {0,1},{1,1},{1,0},{0,0} },
     { {1,0},{0,0},{0,1},{1,1} },
     { {0,1},{1,1},{1,0},{0,0} },
};

#define FENCE_GATE_VERTS 288

#define WALL_VERTS (5 * 36)

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
                           const float UV[4][2], unsigned int color,
                           bool mirrorBack = true) {
    static const int triF[6] = { 0, 1, 2, 2, 3, 0 };
    static const int triB[6] = { 0, 2, 1, 2, 0, 3 };
    for (int pass = 0; pass < 2; pass++) {
        const int* tri = pass ? triB : triF;
        for (int t = 0; t < 6; t++) {
            int k = tri[t];

            const int uk = (pass && mirrorBack) ? (3 - k) : k;
            out[n].u = UV[uk][0]; out[n].v = UV[uk][1]; out[n].color = color;
            out[n].x = P[k][0]; out[n].y = P[k][1]; out[n].z = P[k][2];
            n++;
        }
    }
    return n;
}

static int writeQuadSingle(ChunkVertex* out, int n, const float P[4][3],
                           const float UV[4][2], unsigned int color) {
    static const int tri[6] = { 0, 1, 2, 2, 3, 0 };
    for (int t = 0; t < 6; t++) {
        int k = tri[t];
        out[n].u = UV[k][0]; out[n].v = UV[k][1]; out[n].color = color;
        out[n].x = P[k][0]; out[n].y = P[k][1]; out[n].z = P[k][2];
        n++;
    }
    return n;
}

static inline int fireQuad(ChunkVertex* out, int n, unsigned int color,
    float ax,float ay,float az,float au,float av, float bx,float by,float bz,float bu,float bv,
    float cx,float cy,float cz,float cu,float cv, float dx,float dy,float dz,float du,float dv) {
    if (!out) return n + 6;
    const float P[4][3]  = { {ax,ay,az}, {bx,by,bz}, {cx,cy,cz}, {dx,dy,dz} };
    const float UV[4][2] = { {au,av},    {bu,bv},    {cu,cv},    {du,dv} };
    return writeQuadSingle(out, n, P, UV, color);
}

int emitFire(ChunkVertex* out, int n, const World* w, int gx, int y, int gz, unsigned int c) {
    int col, row; unsigned int tint;
    tileForBlock(BLOCK_FIRE, 0, 0, &col, &row, &tint);
    const float HT = TILE_UV / 32.0f;
    float u0 = col * TILE_UV + HT, v0 = row * TILE_UV + HT;
    float u1 = (col + 1) * TILE_UV - HT, v1 = (row + 1) * TILE_UV - HT;
    float x = (float)gx, z = (float)gz, yb = (float)y, h = 1.4f;
#define FQ(a...) n = fireQuad(out, n, c, a)

    if (isSolidBlocking(worldBlock(w, gx, y - 1, gz)) || fireCanBurn(w, gx, y - 1, gz)) {

        float x0 = x+0.7f, x1 = x+0.3f, z0 = z+0.7f, z1 = z+0.3f;
        float x0_ = x+0.2f, x1_ = x+0.8f, z0_ = z+0.2f, z1_ = z+0.8f;
        FQ(x0_,yb+h,z+1,u1,v0,  x0,yb,z+1,u1,v1,  x0,yb,z,u0,v1,   x0_,yb+h,z,u0,v0);
        FQ(x1_,yb+h,z,u1,v0,    x1,yb,z,u1,v1,    x1,yb,z+1,u0,v1, x1_,yb+h,z+1,u0,v0);
        FQ(x+1,yb+h,z1_,u1,v0,  x+1,yb,z1,u1,v1,  x,yb,z1,u0,v1,   x,yb+h,z1_,u0,v0);
        FQ(x,yb+h,z0_,u1,v0,    x,yb,z0,u1,v1,    x+1,yb,z0,u0,v1, x+1,yb+h,z0_,u0,v0);
        x0 = x; x1 = x+1; z0 = z; z1 = z+1;
        x0_ = x+0.1f; x1_ = x+0.9f; z0_ = z+0.1f; z1_ = z+0.9f;
        FQ(x0_,yb+h,z,u0,v0,    x0,yb,z,u0,v1,    x0,yb,z+1,u1,v1, x0_,yb+h,z+1,u1,v0);
        FQ(x1_,yb+h,z+1,u0,v0,  x1,yb,z+1,u0,v1,  x1,yb,z,u1,v1,   x1_,yb+h,z,u1,v0);
        FQ(x,yb+h,z1_,u0,v0,    x,yb,z1,u0,v1,    x+1,yb,z1,u1,v1, x+1,yb+h,z1_,u1,v0);
        FQ(x+1,yb+h,z0_,u0,v0,  x+1,yb,z0,u0,v1,  x,yb,z0,u1,v1,   x,yb+h,z0_,u1,v0);
    } else {

        const float r = 0.2f, yo = 1.0f / 16.0f;
        float b = yb + yo, t = yb + h + yo;
        if (fireCanBurn(w, gx - 1, y, gz)) {
            FQ(x+r,t,z+1,u1,v0,  x,b,z+1,u1,v1,  x,b,z,u0,v1,    x+r,t,z,u0,v0);
            FQ(x+r,t,z,u0,v0,    x,b,z,u0,v1,    x,b,z+1,u1,v1,  x+r,t,z+1,u1,v0);
        }
        if (fireCanBurn(w, gx + 1, y, gz)) {
            FQ(x+1-r,t,z,u0,v0,  x+1,b,z,u0,v1,  x+1,b,z+1,u1,v1, x+1-r,t,z+1,u1,v0);
            FQ(x+1-r,t,z+1,u1,v0,x+1,b,z+1,u1,v1,x+1,b,z,u0,v1,   x+1-r,t,z,u0,v0);
        }
        if (fireCanBurn(w, gx, y, gz - 1)) {
            FQ(x,t,z+r,u1,v0,    x,b,z,u1,v1,    x+1,b,z,u0,v1,   x+1,t,z+r,u0,v0);
            FQ(x+1,t,z+r,u0,v0,  x+1,b,z,u0,v1,  x,b,z,u1,v1,     x,t,z+r,u1,v0);
        }
        if (fireCanBurn(w, gx, y, gz + 1)) {
            FQ(x+1,t,z+1-r,u0,v0,x+1,b,z+1,u0,v1,x,b,z+1,u1,v1,   x,t,z+1-r,u1,v0);
            FQ(x,t,z+1-r,u1,v0,  x,b,z+1,u1,v1,  x+1,b,z+1,u0,v1, x+1,t,z+1-r,u0,v0);
        }
        if (fireCanBurn(w, gx, y + 1, gz)) {
            float yy = yb + 1.0f, hh = -0.2f, tt = yy + hh;
            FQ(x,tt,z,u1,v0,     x+1,yy,z,u1,v1,   x+1,yy,z+1,u0,v1, x,tt,z+1,u0,v0);
            FQ(x+1,tt,z+1,u1,v0, x,yy,z+1,u1,v1,   x,yy,z,u0,v1,     x+1,tt,z,u0,v0);
        }
    }
#undef FQ
    return n;
}

int emitCross(ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id,
                     unsigned char data, unsigned int bright, bool jitter) {
    int col, row; unsigned int tint;
    tileForBlock(id, data, 0, &col, &row, &tint);

    const float HT = TILE_UV / 32.0f;
    float u0 = col * TILE_UV + HT, v0 = row * TILE_UV + HT;
    float u1 = (col + 1) * TILE_UV - HT, v1 = (row + 1) * TILE_UV - HT;

    unsigned int color = mulColor(bright, tint);

    float dx = 0.0f, dy = 0.0f, dz = 0.0f;
    if (jitter) crossJitter81(id, gx, y, gz, &dx, &dy, &dz);

    float x0 = gx + dx + 0.05f, x1 = gx + dx + 0.95f;
    float z0 = gz + dz + 0.05f, z1 = gz + dz + 0.95f;
    float yb = (float)y + dy, yt = (float)y + dy + 1.0f;
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

    const float SINK = 1.0f / 16.0f;
    float yb = (float)y - SINK, yt = (float)y + 1.0f - SINK;

    for (int i = 0; i < 2; i++) {
        float px = gx + (i == 0 ? 0.25f : 0.75f);
        float za = (i == 0) ? (float)gz : gz + 1.0f;
        float zb = (i == 0) ? gz + 1.0f : (float)gz;
        float P[4][3] = { {px, yt, za}, {px, yb, za}, {px, yb, zb}, {px, yt, zb} };
        n = writeQuadDouble(out, n, P, UV, color);
    }
    for (int i = 0; i < 2; i++) {
        float pz = gz + (i == 0 ? 0.25f : 0.75f);
        float xa = (i == 0) ? (float)gx : gx + 1.0f;
        float xb = (i == 0) ? gx + 1.0f : (float)gx;
        float P[4][3] = { {xa, yt, pz}, {xa, yb, pz}, {xb, yb, pz}, {xb, yt, pz} };
        n = writeQuadDouble(out, n, P, UV, color);
    }
    return n;
}

int emitMelonStem(const World* w, ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id, unsigned char data, unsigned int bright) {
    int col, row; unsigned int tint;

    const unsigned char fruit = stemFruit(id);
    tileForBlock(id, data, 0, &col, &row, &tint);
    unsigned int color = mulColor(bright, tint);

    int connectDir = -1;
    if (data >= 7) {
        static const signed char dir[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
        for (int i = 0; i < 4; i++)
            if (worldBlock(w, gx + dir[i][0], y, gz + dir[i][1]) == fruit) { connectDir = i; break; }
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

    n = writeQuadDouble(out, n, A, UV, color, false);
    n = writeQuadDouble(out, n, B, UV, color, false);

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
        n = writeQuadDouble(out, n, P, CUV, color, false);
    }
    return n;
}

int emitLadder(ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id, unsigned char data, unsigned int bright) {
    int col, row; unsigned int tint;
    tileForBlock(id, data, 0, &col, &row, &tint);

    const float HT = TILE_UV / 128.0f;
    float u0 = col * TILE_UV + HT, v0 = row * TILE_UV + HT;
    float u1 = (col + 1) * TILE_UV - HT, v1 = (row + 1) * TILE_UV - HT;
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

int emitRail(ChunkVertex* out, int n, int gx, int y, int gz, unsigned char id,
             unsigned char data, unsigned int bright) {
    int col, row; unsigned int tint;
    tileForBlock(id, data, 0, &col, &row, &tint);
    const float HT = TILE_UV / 128.0f;
    const float u0 = col * TILE_UV + HT,       v0t = row * TILE_UV + HT;
    const float u1 = (col + 1) * TILE_UV - HT, v1t = (row + 1) * TILE_UV - HT;

    const int dir = railDir(id, data);

    const float CX[4] = { gx + 1.0f, gx + 1.0f, gx + 0.0f, gx + 0.0f };
    const float CZ[4] = { gz + 0.0f, gz + 1.0f, gz + 1.0f, gz + 0.0f };

    int rot;
    switch (dir) {
        case 1: case 2: case 3: case 7: rot = 1; break;
        case 8:                         rot = 2; break;
        case 9:                         rot = 3; break;
        default:                        rot = 0; break;
    }

    const bool lift03 = (dir == 2 || dir == 4);
    const bool lift12 = (dir == 3 || dir == 5);

    const float yb = (float)y + 0.0625f;
    float P[4][3];
    for (int k = 0; k < 4; k++) {
        const int c = (k + rot) & 3;
        P[k][0] = CX[c];
        P[k][2] = CZ[c];
        P[k][1] = yb + (((k == 0 || k == 3) && lift03) || ((k == 1 || k == 2) && lift12) ? 1.0f : 0.0f);
    }
    const float UV[4][2] = { {u1, v0t}, {u1, v1t}, {u0, v1t}, {u0, v0t} };

    return writeQuadDouble(out, n, P, UV, bright, false);
}

static inline bool isNoMipLayerId(unsigned char id) {

    return isCrossShaped(id)
        || id == BLOCK_FIRE || isLadder(id) || isRail(id) || id == BLOCK_BED || isTorch(id)
        || isPane(id) || isDoor(id) || isTrapdoor(id)
        || id == BLOCK_CACTUS || isGlass(id) || id == BLOCK_CAKE;
}

bool sectionCannotEmit(const World* w, int ox, int oz, int si) {
    unsigned char id = 0;
    return blockSectionUniform(w, ox, si * SECTION_SY, oz, &id) && id == BLOCK_AIR;
}

int meshPass(const World* w, int ox, int oz, int y0, int y1, ChunkVertex* out, int layer, int cap, bool leavesOpaque, bool leavesCull, int* nLava) {
    int n = 0;
    bool sawLava = false;

    unsigned char lc[18 * 18 * 18];
    unsigned char llc[18 * 18 * 18];
    if (layer != 1) {
        for (int dx = 0; dx < 18; dx++)
        for (int dz = 0; dz < 18; dz++)
        for (int dy = 0; dy < 18; dy++) {
            int i = (dx * 18 + dz) * 18 + dy;
            lc[i] = worldBlock(w, ox - 1 + dx, y0 - 1 + dy, oz - 1 + dz);
        }
        memset(llc, 0xFF, sizeof llc);
    }
    #define LCB(X, Y, Z) lc[((((X) - ox + 1) * 18 + ((Z) - oz + 1)) * 18) + ((Y) - y0 + 1)]
    #define LLB(X, Y, Z) lightLazy(w, llc, \
        ((((X) - ox + 1) * 18 + ((Z) - oz + 1)) * 18) + ((Y) - y0 + 1), (X), (Y), (Z))

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
                if (layer != 3) continue;
                sawLava = true;
                continue;
            }
        }

        if (layer == 0 && isOpaque(id)
            && isOpaque(LCB(gx - 1, y, gz)) && isOpaque(LCB(gx + 1, y, gz))
            && isOpaque(LCB(gx, y - 1, gz)) && isOpaque(LCB(gx, y + 1, gz))
            && isOpaque(LCB(gx, y, gz - 1)) && isOpaque(LCB(gx, y, gz + 1)))
            continue;

        if (isSign(id) || id == BLOCK_CHEST) continue;

        if (layer == 0 && isNoMipLayerId(id)) continue;

        if (layer == 3 && isStemTile(id)) {
            if (out && n + 36 > cap) return -1;
            if (out) n = emitMelonStem(w, out, n, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            else n += 36;
            continue;
        }

        if (layer == 3 && isCropTile(id)) {
            if (out && n + 48 > cap) return -1;
            if (out) emitCropRows(out, n, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            n += 48;
            continue;
        }

        if (layer == 3 && isCrossShaped(id)) {
            if (out && n + 24 > cap) return -1;
            if (out) emitCross(out, n, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            n += 24;
            continue;
        }

        if (layer == 3 && id == BLOCK_FIRE) {
            if (out && n + 60 > cap) return -1;
            n = emitFire(out, n, w, gx, y, gz, out ? g_brightColor[LLB(gx, y, gz)] : 0);
            continue;
        }

        if (layer == 3 && isLadder(id)) {
            if (out && n + 12 > cap) return -1;
            if (out) n = emitLadder(out, n, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            else n += 12;
            continue;
        }

        if (layer == 3 && isRail(id)) {
            if (out && n + 12 > cap) return -1;
            if (out) n = emitRail(out, n, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[LLB(gx, y, gz)]);
            else n += 12;
            continue;
        }

        if (layer == 3 && id == BLOCK_BED) {
            if (out && n + 36 > cap) return -1;
            if (out) n = emitBed(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            else n += 36;
            continue;
        }

        if (layer == 3 && isTorch(id)) {
            if (out && n + 36 > cap) return -1;
            if (out) n = emitTorch(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            else n += 36;
            continue;
        }

        if (layer == 0 && isCarpet(id)) {
            if (out && n + 36 > cap) return -1;
            if (out) n = emitCarpet(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            else n += 36;
            continue;
        }

        if (layer == 0 && (isSlab(id) || isStairs(id))) {
            if (out && n + 108 > cap) return -1;
            n = isSlab(id) ? emitSlab(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n)
                           : emitStairs(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 3 && isPane(id)) {

            if (out && n + 108 > cap) return -1;
            n = emitPane(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 0 && isFence(id)) {
            if (out && n + 324 > cap) return -1;
            n = emitFence(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 0 && isWall(id)) {
            if (out && n + WALL_VERTS > cap) return -1;
            n = emitWall(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
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

        if (layer == 3 && id == BLOCK_CAKE) {
            if (out && n + 36 > cap) return -1;
            n = emitCake(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 0 && isFenceGate(id)) {
            if (out && n + FENCE_GATE_VERTS > cap) return -1;
            n = emitFenceGate(w, gx, y, gz, id, worldData(w, gx, y, gz), out, n);
            continue;
        }

        if (layer == 3 && id == BLOCK_GRASS && LCB(gx, y + 1, gz) != BLOCK_TOPSNOW) {
            if (out && n + 24 > cap) return -1;
            static const int gtri[6] = { 0, 1, 2, 2, 3, 0 };
            const unsigned int GRASS_TINT = 0xFF6BBD7Cu;
            const float ou0 = 6 * TILE_UV, ov0 = 2 * TILE_UV;

            const float TE = TILE_UV / 128.0f, TILE_INNER = TILE_UV - 2.0f * TE;
            for (int f = 0; f < 6; f++) {
                if (f == F_TOP || f == F_DOWN) continue;
                int nx = gx + kFaceNeighbor[f][0];
                int ny = y  + kFaceNeighbor[f][1];
                int nz = gz + kFaceNeighbor[f][2];
                if (isOpaque(LCB(nx, ny, nz))) continue;
                if (!out) { n += 6; continue; }
                int col, row; unsigned int tint;
                tileForBlock(id, worldData(w, gx, y, gz), f, &col, &row, &tint);
                unsigned int cc[2][2];
                faceCornerColors(w, lc, llc,
                                 ((((nx - ox + 1) * 18 + (nz - oz + 1)) * 18) + (ny - y0 + 1)),
                                 nx, ny, nz, f, id, tint, kFaceShade[f], cc);
                const int ca = f >> 1, ca1 = (ca + 1) % 3, ca2 = (ca + 2) % 3;
                for (int t = 0; t < 6; t++) {
                    int k = gtri[t];
                    const signed char* c = kFaceCorner[f][k];
                    out[n + t].u = ou0 + (TE + kFaceUV[f][k][0] * TILE_INNER);
                    out[n + t].v = ov0 + (TE + kFaceUV[f][k][1] * TILE_INNER);
                    out[n + t].color = mulColor(cc[c[ca1]][c[ca2]], GRASS_TINT);
                    out[n + t].x = (float)(gx + c[0]);
                    out[n + t].y = (c[1] == 1) ? ((float)y + 1.0f) : (float)(y + c[1]);
                    out[n + t].z = (float)(gz + c[2]);
                }
                n += 6;
            }
            continue;
        }

        bool noMipCube = (id == BLOCK_CACTUS) || isGlass(id);

        if (layer == 3 && !noMipCube) continue;

        if (out && n + 36 > cap) return -1;

        int blockData = -1;

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
                if (blockData < 0) blockData = worldData(w, gx, y, gz);
                tileForBlock(id, (unsigned char)blockData, f, &col, &row, &tint);
                if (id == BLOCK_GRASS && LCB(gx, y + 1, gz) == BLOCK_TOPSNOW) {
                    if (f != F_TOP && f != F_DOWN) { col = 4; row = 4; tint = 0xFFFFFFFFu; }
                }

                if (layer == 2 && leavesOpaque) col += 1;
                float u0 = col * TILE_UV, v0 = row * TILE_UV;

                unsigned int cc[2][2];
                faceCornerColors(w, lc, llc,
                                 ((((nx - ox + 1) * 18 + (nz - oz + 1)) * 18) + (ny - y0 + 1)),
                                 nx, ny, nz, f, id, tint, kFaceShade[f], cc);
                const int ca = f >> 1, ca1 = (ca + 1) % 3, ca2 = (ca + 2) % 3;

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

                int fr = pillarFaceRotation(id, (unsigned char)blockData, f);
                if (fr == 0) fr = faceRotation(rotFaceMask(id), f, gx, y, gz);
                for (int t = 0; t < 6; t++) {
                    int k = tri[t];
                    const signed char* c = kFaceCorner[f][k];
                    float uv_u = kFaceUV[f][k][0];
                    float uv_v = kFaceUV[f][k][1];

                    if (id == BLOCK_TOPSNOW && f != F_TOP && f != F_DOWN) {
                        if (uv_v == 0) uv_v = 1.0f - th;
                    }

                    if (fr) applyFaceRot(fr, f, &uv_u, &uv_v);

                    float bx = (float)(gx + c[0]) + ix + seamOff(c[0]);
                    float by = ((c[1] == 1) ? ((float)y + th) : (float)(y + c[1])) + seamOff(c[1]);
                    float bz = (float)(gz + c[2]) + iz + seamOff(c[2]);

                    const float TE = TILE_UV / 128.0f;
                    out[n + t].u = u0 + (TE + uv_u * (TILE_UV - 2.0f * TE));
                    out[n + t].v = v0 + (TE + uv_v * (TILE_UV - 2.0f * TE));
                    out[n + t].color = cc[c[ca1]][c[ca2]];

                    out[n + t].x = bx;
                    out[n + t].y = by;
                    out[n + t].z = bz;
                }
            }
            n += 6;
        }
    }

    if (nLava) *nLava = n;
    if (sawLava) {
        for (int lx = 0; lx < CHUNK_SX; lx++)
        for (int lz = 0; lz < CHUNK_SZ; lz++)
        for (int y = y0; y < y1; y++) {
            int gx = ox + lx, gz = oz + lz;
            unsigned char id = worldBlock(w, gx, y, gz);
            if (!isLavaId(id)) continue;
            if (out && n + 36 > cap) return -1;
            n = emitLiquid(w, gx, y, gz, id, out, n);
        }
    }
    #undef LCB
    #undef LLB
    return n;
}

#if MESH_RESERVE_CHECK

int g_sinkReserved[4] = { -1, -1, -1, -1 };
int g_sinkStart[4]    = { 0, 0, 0, 0 };
unsigned int g_sinkOverruns = 0;
#endif

int meshSectionSink(const World* w, int ox, int oz, int y0, int y1,
                    MeshSink* skp, int* nLava, bool leavesOpaque, bool leavesCull) {
    MeshSink& sk = *skp;

    int& no = sk.n[0]; int& nw = sk.n[1]; int& nn = sk.n[3];
    bool sawLava = false;

    unsigned char lc[18 * 18 * 18];
    unsigned char llc[18 * 18 * 18];

    int li = 0;
    for (int dx = 0; dx < 18; dx++)
    for (int dz = 0; dz < 18; dz++)
    for (int dy = 0; dy < 18; dy++)
        lc[li++] = worldBlock(w, ox - 1 + dx, y0 - 1 + dy, oz - 1 + dz);
    memset(llc, 0xFF, sizeof llc);
    #define LCB(X, Y, Z) lc[((((X) - ox + 1) * 18 + ((Z) - oz + 1)) * 18) + ((Y) - y0 + 1)]
    #define LLB(X, Y, Z) lightLazy(w, llc, \
        ((((X) - ox + 1) * 18 + ((Z) - oz + 1)) * 18) + ((Y) - y0 + 1), (X), (Y), (Z))

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

        switch (id) {

            case BLOCK_SIGN: case BLOCK_WALL_SIGN: case BLOCK_CHEST:
                continue;

            case BLOCK_WATER: case BLOCK_CALM_WATER:
                if (!sinkReserve(&sk, 1, 36)) return -1;
                nw = emitLiquid(w, gx, y, gz, id, sk.buf[1], nw);
                continue;

            case BLOCK_LAVA: case BLOCK_CALM_LAVA:
                sawLava = true;
                continue;

            case BLOCK_MELON_STEM: case BLOCK_PUMPKIN_STEM:
                if (!sinkReserve(&sk, 3, 36)) return -1;
                nn = emitMelonStem(w, sk.buf[3], nn, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[lightLazy(w, llc, base, gx, y, gz)]);
                continue;

            case BLOCK_WHEAT: case BLOCK_CARROTS: case BLOCK_POTATOES: case BLOCK_BEETROOT:
                if (!sinkReserve(&sk, 3, 48)) return -1;
                nn = emitCropRows(sk.buf[3], nn, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[lightLazy(w, llc, base, gx, y, gz)]);
                continue;

            case BLOCK_FLOWER: case BLOCK_ROSE:
            case BLOCK_MUSHROOM_BROWN: case BLOCK_MUSHROOM_RED:
            case BLOCK_REEDS: case BLOCK_SAPLING:
            case BLOCK_TALLGRASS: case BLOCK_COBWEB:
                if (!sinkReserve(&sk, 3, 24)) return -1;
                emitCross(sk.buf[3], nn, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[lightLazy(w, llc, base, gx, y, gz)]);
                nn += 24;
                continue;

            case BLOCK_FIRE:
                if (!sinkReserve(&sk, 3, 60)) return -1;
                nn = emitFire(sk.buf[3], nn, w, gx, y, gz, g_brightColor[lightLazy(w, llc, base, gx, y, gz)]);
                continue;

            case BLOCK_LADDER:
                if (!sinkReserve(&sk, 3, 12)) return -1;
                nn = emitLadder(sk.buf[3], nn, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[lightLazy(w, llc, base, gx, y, gz)]);
                continue;

            case BLOCK_RAIL: case BLOCK_GOLDEN_RAIL:
                if (!sinkReserve(&sk, 3, 12)) return -1;
                nn = emitRail(sk.buf[3], nn, gx, y, gz, id, worldData(w, gx, y, gz), g_brightColor[lightLazy(w, llc, base, gx, y, gz)]);
                continue;

            case BLOCK_BED:
                if (!sinkReserve(&sk, 3, 36)) return -1;
                nn = emitBed(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[3], nn);
                continue;

            case BLOCK_TORCH:
                if (!sinkReserve(&sk, 3, 36)) return -1;
                nn = emitTorch(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[3], nn);
                continue;

            case BLOCK_CARPET:
                if (!sinkReserve(&sk, 0, 36)) return -1;
                no = emitCarpet(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[0], no);
                continue;

            case BLOCK_SLAB: case BLOCK_WOOD_SLAB:
            case BLOCK_STAIRS_PLANKS: case BLOCK_STAIRS_COBBLESTONE:
            case BLOCK_STAIRS_SPRUCE: case BLOCK_STAIRS_BIRCH: case BLOCK_STAIRS_JUNGLE:
            case BLOCK_STAIRS_BRICK:  case BLOCK_STAIRS_STONE_BRICK:
            case BLOCK_STAIRS_SANDSTONE: case BLOCK_STAIRS_NETHER_BRICK:
            case BLOCK_STAIRS_QUARTZ:
                if (!sinkReserve(&sk, 0, 108)) return -1;
                no = isSlab(id) ? emitSlab(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[0], no)
                                : emitStairs(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[0], no);
                continue;

            case BLOCK_GLASS_PANE: case BLOCK_IRON_BARS:
                if (!sinkReserve(&sk, 3, 108)) return -1;
                nn = emitPane(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[3], nn);
                continue;

            case BLOCK_FENCE:
                if (!sinkReserve(&sk, 0, 324)) return -1;
                no = emitFence(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[0], no);
                continue;

            case BLOCK_COBBLE_WALL:
                if (!sinkReserve(&sk, 0, WALL_VERTS)) return -1;
                no = emitWall(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[0], no);
                continue;

            case BLOCK_DOOR_WOOD: case BLOCK_DOOR_IRON:
                if (!sinkReserve(&sk, 3, 36)) return -1;
                nn = emitDoor(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[3], nn);
                continue;

            case BLOCK_TRAPDOOR:
                if (!sinkReserve(&sk, 3, 36)) return -1;
                nn = emitTrapdoor(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[3], nn);
                continue;

            case BLOCK_CAKE:
                if (!sinkReserve(&sk, 3, 36)) return -1;
                nn = emitCake(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[3], nn);
                continue;

            case BLOCK_FENCE_GATE:
                if (!sinkReserve(&sk, 0, FENCE_GATE_VERTS)) return -1;
                no = emitFenceGate(w, gx, y, gz, id, worldData(w, gx, y, gz), sk.buf[0], no);
                continue;
        }

        bool leaf = isLeaf(id);
        bool leafTransparent = leaf && !leavesOpaque;
        bool leafOpaqueDst = leaf && !leafTransparent;
        bool noMip = (id == BLOCK_CACTUS) || isGlass(id) || leafTransparent;
        int layer = leafOpaqueDst ? 2 : noMip ? 3 : 0;

        bool grassSide = (id == BLOCK_GRASS) && (lc[base + 1] != BLOCK_TOPSNOW);

        if (grassSide && !sinkReserve(&sk, 3, 24)) return -1;

        if (!sinkReserve(&sk, layer, 36)) return -1;
        ChunkVertex* dst = sk.buf[layer];
        int nd = sk.n[layer];
        ChunkVertex* ovl = sk.buf[3];
        int nOvl = sk.n[3];

        unsigned char blockData = worldData(w, gx, y, gz);

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
            tileForBlock(id, blockData, f, &col, &row, &tint);
            if (id == BLOCK_GRASS && lc[base + 1] == BLOCK_TOPSNOW) {
                if (f != F_TOP && f != F_DOWN) { col = 4; row = 4; tint = 0xFFFFFFFFu; }
            }

            if (leafOpaqueDst) col += 1;
            float u0 = col * TILE_UV, v0 = row * TILE_UV;

            unsigned int cc[2][2];
            faceCornerColors(w, lc, llc, nbi, gx + kFaceNeighbor[f][0],
                                              y  + kFaceNeighbor[f][1],
                                              gz + kFaceNeighbor[f][2], f, id, tint, kFaceShade[f], cc);
            const int ca = f >> 1, ca1 = (ca + 1) % 3, ca2 = (ca + 2) % 3;

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

            int fr = pillarFaceRotation(id, (unsigned char)blockData, f);
            if (fr == 0) fr = faceRotation(rotFaceMask(id), f, gx, y, gz);
            for (int t = 0; t < 6; t++) {
                int k = tri[t];
                const signed char* c = kFaceCorner[f][k];
                float uv_u = kFaceUV[f][k][0];
                float uv_v = kFaceUV[f][k][1];
                if (id == BLOCK_TOPSNOW && f != F_TOP && f != F_DOWN) {
                    if (uv_v == 0) uv_v = 1.0f - th;
                }
                if (fr) applyFaceRot(fr, f, &uv_u, &uv_v);
                float bx = (float)(gx + c[0]) + ix + seamOff(c[0]);
                float by = ((c[1] == 1) ? ((float)y + th) : (float)(y + c[1])) + seamOff(c[1]);
                float bz = (float)(gz + c[2]) + iz + seamOff(c[2]);
                dst[nd + t].u = u0 + (cTE + uv_u * cInner);
                dst[nd + t].v = v0 + (cTE + uv_v * cInner);
                dst[nd + t].color = cc[c[ca1]][c[ca2]];
                dst[nd + t].x = bx;
                dst[nd + t].y = by;
                dst[nd + t].z = bz;
            }

            if (grassSide && f != F_TOP && f != F_DOWN) {
                const unsigned int GRASS_TINT = 0xFF6BBD7Cu;
                float ou0 = 6 * TILE_UV, ov0 = 2 * TILE_UV;
                for (int t = 0; t < 6; t++) {
                    int k = tri[t];
                    const signed char* c = kFaceCorner[f][k];
                    ovl[nOvl + t].u = ou0 + (cTE + kFaceUV[f][k][0] * cInner);
                    ovl[nOvl + t].v = ov0 + (cTE + kFaceUV[f][k][1] * cInner);
                    ovl[nOvl + t].color = mulColor(cc[c[ca1]][c[ca2]], GRASS_TINT);
                    ovl[nOvl + t].x = dst[nd + t].x;
                    ovl[nOvl + t].y = dst[nd + t].y;
                    ovl[nOvl + t].z = dst[nd + t].z;
                }
                nOvl += 6;
            }
            nd += 6;
        }
        sk.n[layer] = nd;
        if (grassSide) sk.n[3] = nOvl;
    }

    int nLavaStart = sinkCount(&sk, 3);
    if (sawLava) {
        for (int lx = 0; lx < CHUNK_SX; lx++)
        for (int lz = 0; lz < CHUNK_SZ; lz++)
        for (int y = y0; y < y1; y++) {
            unsigned char id = lc[(((lx + 1) * 18 + (lz + 1)) * 18) + (y - y0 + 1)];
            if (!isLavaId(id)) continue;
            if (!sinkReserve(&sk, 3, 36)) return -1;
            nn = emitLiquid(w, ox + lx, y, oz + lz, id, sk.buf[3], nn);
        }
    }
    #undef LCB
    #undef LLB

    for (int L = 0; L < 4; L++) sinkCheckPrev(&sk, L);
    if (sk.flush)
        for (int L = 0; L < 4; L++)
            if (sk.n[L] && !sk.flush(&sk, L)) return -1;
    *nLava = nLavaStart;
    return 0;
}

int meshSection(const World* w, int ox, int oz, int y0, int y1,
                ChunkVertex* out0, ChunkVertex* out1, ChunkVertex* out2, ChunkVertex* out3,
                int cap0, int cap1, int cap2, int cap3, int* n0, int* n1, int* n2, int* n3,
                int* nLava, bool leavesOpaque, bool leavesCull) {
    MeshSink sk;
    sk.buf[0] = out0; sk.buf[1] = out1; sk.buf[2] = out2; sk.buf[3] = out3;
    sk.cap[0] = cap0; sk.cap[1] = cap1; sk.cap[2] = cap2; sk.cap[3] = cap3;
    for (int L = 0; L < 4; L++) { sk.n[L] = 0; sk.total[L] = 0; }
    sk.flush = 0; sk.ctx = 0;
    int rc = meshSectionSink(w, ox, oz, y0, y1, &sk, nLava, leavesOpaque, leavesCull);
    *n0 = sinkCount(&sk, 0); *n1 = sinkCount(&sk, 1);
    *n2 = sinkCount(&sk, 2); *n3 = sinkCount(&sk, 3);
    return rc;
}
