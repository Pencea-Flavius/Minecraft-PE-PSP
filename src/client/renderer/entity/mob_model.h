
#ifndef MCPSP_CLIENT_ENTITY_MOB_MODEL_H
#define MCPSP_CLIENT_ENTITY_MOB_MODEL_H

#include <math.h>

class Mob;
struct Texture;

struct MobVertex { float u, v; float x, y, z; };

#define MOB_LIGHTING 1

inline void mobFaceNormal(int face, float* n) {
    static const signed char F[6][3] = {
        {  0, -1,  0 },
        { -1,  0,  0 },
        {  1,  0,  0 },
        {  0,  1,  0 },
        {  0,  0, -1 },
        {  0,  0,  1 },
    };
    n[0] = F[face][0];
    n[1] = F[face][1];
    n[2] = F[face][2];
}

#define MOB_LIGHT_DIFFUSE 0.6f
#define MOB_LIGHT_AMBIENT 0.4f

#ifndef MOB_LIGHTING
#error "MOB_LIGHTING must be defined"
#endif

inline unsigned int mobFaceLitColor(const float* m, int face, unsigned int brCol) {
#if !MOB_LIGHTING
    (void)m; (void)face; return brCol;
#else
    static const float LDIR[2][3] = {
        {  0.161690f, 0.808452f, -0.565916f },
        { -0.161690f, 0.808452f,  0.565916f },
    };
    float n[3]; mobFaceNormal(face, n);

    float wx = m[0] * n[0] + m[4] * n[1] + m[8]  * n[2];
    float wy = m[1] * n[0] + m[5] * n[1] + m[9]  * n[2];
    float wz = m[2] * n[0] + m[6] * n[1] + m[10] * n[2];

    float len = sqrtf(wx * wx + wy * wy + wz * wz);
    if (len > 1e-6f) { wx /= len; wy /= len; wz /= len; }

    float d = MOB_LIGHT_AMBIENT;
    for (int i = 0; i < 2; i++) {
        float nl = wx * LDIR[i][0] + wy * LDIR[i][1] + wz * LDIR[i][2];
        if (nl > 0.0f) d += MOB_LIGHT_DIFFUSE * nl;
    }

    if (d > 1.0f) d = 1.0f;

    unsigned int q = (unsigned int)(d * 256.0f);
    unsigned int r = ((brCol        & 0xFFu) * q) >> 8;
    unsigned int g = (((brCol >> 8)  & 0xFFu) * q) >> 8;
    unsigned int b = (((brCol >> 16) & 0xFFu) * q) >> 8;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (brCol & 0xFF000000u) | (b << 16) | (g << 8) | r;
#endif
}

void mobDrawPartLit(const MobVertex* base, unsigned int brCol);

struct SkinVertex { float u, v; unsigned int color; float x, y, z; };

struct MobAnim {
    float bodyRot;
    float headYaw;
    float pitch;
    float speed;
    float pos;
};
MobAnim mobAnimSetup(Mob* mob, float rot, float a);

#define MOB_MAX_PARTS 16

struct MobPart { MobVertex base[36]; float px, py, pz; float xRot, yRot, zRot; bool head; };

void mobBuildBox(MobVertex* out, float x0, float y0, float z0,
                 float x1, float y1, float z1, int tx, int ty, int w, int h, int d,
                 bool mirror, float grow, float texW = 64.0f, float texH = 32.0f);

inline void mobBoxToColoured(SkinVertex* dst, const MobVertex* src, int n, unsigned int col) {
    for (int i = 0; i < n; i++) {
        dst[i].u = src[i].u; dst[i].v = src[i].v;
        dst[i].x = src[i].x; dst[i].y = src[i].y; dst[i].z = src[i].z;
        dst[i].color = col;
    }
}

void mobRenderParts(Mob* mob, MobPart* parts, int count, Texture* tex,
                    float x, float y, float z, float ibody, float a, unsigned int tint,
                    float babyHeadY = 8.0f, float babyHeadZ = 4.0f, float modelScale = 1.0f,
                    float overlayWhite = 0.0f, int bowPartIndex = -1, float modelScaleY = -1.0f,
                    short heldItemId = 0);

static inline int mobDepthBiasUnits(float d2, float nearZ, float biasBlocks) {
    if (d2 < 0.25f) d2 = 0.25f;
    float units = 65535.0f * nearZ * biasBlocks / d2;
    if (units > 4000.0f) units = 4000.0f;

    return (int)units;
}

#endif
