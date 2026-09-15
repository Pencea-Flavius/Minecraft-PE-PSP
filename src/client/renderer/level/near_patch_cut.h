
#ifndef MCPSP_NEAR_PATCH_CUT_H
#define MCPSP_NEAR_PATCH_CUT_H

#include "world/level/chunk/chunk.h"
#include <math.h>

struct PieceVertex {
    short u, v;
    unsigned int color;
    float x, y, z;
};

struct PV { float x, y, z, lx, ly, lz, u, v; int r, g, b, a; };

static inline float dist2(const PV& p, const PV& q) {
    float dx = p.x - q.x, dy = p.y - q.y, dz = p.z - q.z;
    return dx * dx + dy * dy + dz * dz;
}

static inline void decode(const DrawVertex& d, const int origin[3], float overscale, PV& o) {
    const float k = overscale / (float)POS_ENC;
    o.x = d.x * k + (float)origin[0];
    o.y = d.y * k + (float)origin[1];
    o.z = d.z * k + (float)origin[2];
    o.lx = d.x; o.ly = d.y; o.lz = d.z;
    o.u = d.u / 32767.0f;
    o.v = d.v / 32767.0f;
    o.r = d.color & 0xFF;         o.g = (d.color >> 8) & 0xFF;
    o.b = (d.color >> 16) & 0xFF; o.a = (d.color >> 24) & 0xFF;
}

struct Out { PieceVertex* out; int n, cap; bool full; };

static inline void put(Out& o, const PV& p) {
    PieceVertex& d = o.out[o.n++];
    d.u = uvQ(p.u); d.v = uvQ(p.v);
    d.color = (unsigned int)p.r | ((unsigned int)p.g << 8) |
              ((unsigned int)p.b << 16) | ((unsigned int)p.a << 24);
    const float k = 1.0f / 32768.0f;
    d.x = p.lx * k; d.y = p.ly * k; d.z = p.lz * k;
}

static const int GRID_MAX_DEPTH = 3;
static inline void emitGrid(Out& o, const PV& a, const PV& b, const PV& c, int depth) {
    if (o.full) return;
    if (depth > GRID_MAX_DEPTH) depth = GRID_MAX_DEPTH;
    const int n = 1 << depth;
    const int tris = n * n;
    if (o.n + tris * 3 > o.cap) { o.full = true; return; }
    if (depth == 0) { put(o, a); put(o, b); put(o, c); return; }
    PV g[45];
    const float inv = 1.0f / (float)n;
    int k = 0;
    for (int j = 0; j <= n; j++)
        for (int i = 0; i + j <= n; i++, k++) {
            const float fb = i * inv, fc = j * inv, fa = 1.0f - fb - fc;
            PV& q = g[k];
            q.x  = a.x  * fa + b.x  * fb + c.x  * fc;
            q.y  = a.y  * fa + b.y  * fb + c.y  * fc;
            q.z  = a.z  * fa + b.z  * fb + c.z  * fc;
            q.lx = a.lx * fa + b.lx * fb + c.lx * fc;
            q.ly = a.ly * fa + b.ly * fb + c.ly * fc;
            q.lz = a.lz * fa + b.lz * fb + c.lz * fc;
            q.u  = a.u  * fa + b.u  * fb + c.u  * fc;
            q.v  = a.v  * fa + b.v  * fb + c.v  * fc;
            q.r = (int)(a.r * fa + b.r * fb + c.r * fc + 0.5f);
            q.g = (int)(a.g * fa + b.g * fb + c.g * fc + 0.5f);
            q.b = (int)(a.b * fa + b.b * fb + c.b * fc + 0.5f);
            q.a = (int)(a.a * fa + b.a * fb + c.a * fc + 0.5f);
        }

    for (int j = 0; j < n; j++) {
        const int r0 = j * (n + 1) - j * (j - 1) / 2;
        const int r1 = (j + 1) * (n + 1) - (j + 1) * j / 2;
        for (int i = 0; i + j < n; i++) {
            put(o, g[r0 + i]); put(o, g[r0 + i + 1]); put(o, g[r1 + i]);
            if (i + j < n - 1) {
                put(o, g[r0 + i + 1]); put(o, g[r1 + i + 1]); put(o, g[r1 + i]);
            }
        }
    }
}

static inline void inflateCorners(PV* p, const float grow[3], float worldPerLocal) {
    const float cx = (p[0].x + p[1].x + p[2].x) / 3.0f;
    const float cy = (p[0].y + p[1].y + p[2].y) / 3.0f;
    const float cz = (p[0].z + p[1].z + p[2].z) / 3.0f;
    for (int k = 0; k < 3; k++) {
        const float dx = p[k].x - cx, dy = p[k].y - cy, dz = p[k].z - cz;
        const float len = sqrtf(dx * dx + dy * dy + dz * dz);
        if (len <= 0.0f) continue;
        const float f = grow[k] / len;
        p[k].x += dx * f; p[k].y += dy * f; p[k].z += dz * f;
        const float fl = f / worldPerLocal;
        p[k].lx += dx * fl; p[k].ly += dy * fl; p[k].lz += dz * fl;
    }
}

static const int BOX_MAX_DEPTH = 2;

template <class V>
static inline void boxToView(const float* m, const V& p, float o[3]) {
    o[0] = m[0] * p.x + m[4] * p.y + m[8]  * p.z + m[12];
    o[1] = m[1] * p.x + m[5] * p.y + m[9]  * p.z + m[13];
    o[2] = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
}

template <class V>
static bool boxPartNear(const float* m, const V* v, int n, float safePerEdge) {
    if (n <= 0) return false;
    float lo[3] = { v[0].x, v[0].y, v[0].z }, hi[3] = { v[0].x, v[0].y, v[0].z };
    for (int i = 1; i < n; i++) {
        const float q[3] = { v[i].x, v[i].y, v[i].z };
        for (int j = 0; j < 3; j++) {
            if (q[j] < lo[j]) lo[j] = q[j];
            if (q[j] > hi[j]) hi[j] = q[j];
        }
    }
    float scale = 0.0f;
    for (int c = 0; c < 3; c++) {
        const float l = m[c * 4] * m[c * 4] + m[c * 4 + 1] * m[c * 4 + 1] + m[c * 4 + 2] * m[c * 4 + 2];
        if (l > scale) scale = l;
    }
    scale = sqrtf(scale);
    const float dx = hi[0] - lo[0], dy = hi[1] - lo[1], dz = hi[2] - lo[2];
    const float diag = sqrtf(dx * dx + dy * dy + dz * dz) * scale;
    V mid = v[0];
    mid.x = (lo[0] + hi[0]) * 0.5f; mid.y = (lo[1] + hi[1]) * 0.5f; mid.z = (lo[2] + hi[2]) * 0.5f;
    float c[3];
    boxToView(m, mid, c);
    const float dc = sqrtf(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
    return dc - 0.5f * diag < safePerEdge * diag;
}

template <class V>
static int boxFaceDepth(const float* m, const V* f, float minEdge, float safePerEdge) {
    float p[6][3];
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    for (int i = 0; i < 6; i++) {
        boxToView(m, f[i], p[i]);
        for (int j = 0; j < 3; j++) {
            if (p[i][j] < lo[j]) lo[j] = p[i][j];
            if (p[i][j] > hi[j]) hi[j] = p[i][j];
        }
    }
    float d2 = 0.0f;
    for (int j = 0; j < 3; j++) {
        const float q = lo[j] > 0.0f ? lo[j] : (hi[j] < 0.0f ? -hi[j] : 0.0f);
        d2 += q * q;
    }
    float e2 = 0.0f;
    for (int t = 0; t < 6; t += 3)
        for (int k = 0; k < 3; k++) {
            const float* a = p[t + k], *b = p[t + (k + 1) % 3];
            const float ex = a[0] - b[0], ey = a[1] - b[1], ez = a[2] - b[2];
            const float l = ex * ex + ey * ey + ez * ez;
            if (l > e2) e2 = l;
        }
    float edge = sqrtf(e2);
    const float d = sqrtf(d2);
    int depth = 0;
    while (depth < BOX_MAX_DEPTH && edge > minEdge && d < safePerEdge * edge) {
        edge *= 0.5f;
        depth++;
    }
    return depth;
}

static inline int boxFaceVerts(int depth) { return 6 << (2 * depth); }

template <class V>
static void boxFaceEmit(V* out, const V* f, int depth) {
    if (depth > BOX_MAX_DEPTH) depth = BOX_MAX_DEPTH;
    const int n = 1 << depth;
    const float inv = 1.0f / (float)n;
    int o = 0;
    for (int t = 0; t < 6; t += 3) {
        const V& a = f[t], &b = f[t + 1], &c = f[t + 2];
        V g[15];
        int k = 0;
        for (int j = 0; j <= n; j++)
            for (int i = 0; i + j <= n; i++, k++) {
                const float fb = i * inv, fc = j * inv, fa = 1.0f - fb - fc;
                g[k] = a;
                g[k].u = a.u * fa + b.u * fb + c.u * fc;
                g[k].v = a.v * fa + b.v * fb + c.v * fc;
                g[k].x = a.x * fa + b.x * fb + c.x * fc;
                g[k].y = a.y * fa + b.y * fb + c.y * fc;
                g[k].z = a.z * fa + b.z * fb + c.z * fc;
            }
        for (int j = 0; j < n; j++) {
            const int r0 = j * (n + 1) - j * (j - 1) / 2;
            const int r1 = (j + 1) * (n + 1) - (j + 1) * j / 2;
            for (int i = 0; i + j < n; i++) {
                out[o++] = g[r0 + i]; out[o++] = g[r0 + i + 1]; out[o++] = g[r1 + i];
                if (i + j < n - 1) {
                    out[o++] = g[r0 + i + 1]; out[o++] = g[r1 + i + 1]; out[o++] = g[r1 + i];
                }
            }
        }
    }
}

#endif
