
#ifndef MCPSP_NEAR_PATCH_MATH_H
#define MCPSP_NEAR_PATCH_MATH_H

#include <cmath>

namespace nearpatch {

static const float GUARD_PX   = 2047.0f;
static const float HALF_H_PX  = 136.0f;
static const float ASPECT     = 480.0f / 272.0f;
static const float PI_F       = 3.14159265f;

inline float tanV(float fovDeg) { return std::tan(fovDeg * 0.5f * PI_F / 180.0f); }
inline float tanH(float fovDeg) { return tanV(fovDeg) * ASPECT; }
inline float guardT(float fovDeg) { return GUARD_PX / (HALF_H_PX / tanV(fovDeg)); }

inline float minEdge(float nearZ, float fovDeg) {
    const float t = guardT(fovDeg), h = tanH(fovDeg);
    return 0.8f * nearZ * (t - h) / (1.0f + h);
}

inline float safeDist(float edge, float fovDeg) {
    const float t = guardT(fovDeg), h = tanH(fovDeg), v = tanV(fovDeg);
    return edge * (t + 1.0f) / (t - h) * std::sqrt(1.0f + h * h + v * v);
}

inline bool needsSplit(float dist, float edge, float margin, float nearZ, float fovDeg) {
    if (edge <= minEdge(nearZ, fovDeg)) return false;
    return dist - margin < safeDist(edge, fovDeg);
}

inline bool boxAllInBand(const float m[16], const float lo[3], const float hi[3],
                         float nearZ, float safety) {
    const float gx = GUARD_PX / 240.0f * safety, gy = GUARD_PX / HALF_H_PX * safety;
    for (int c = 0; c < 8; c++) {
        const float x = (c & 1) ? hi[0] : lo[0];
        const float y = (c & 2) ? hi[1] : lo[1];
        const float z = (c & 4) ? hi[2] : lo[2];
        const float w = m[3] * x + m[7] * y + m[11] * z + m[15];
        if (w <= nearZ) return false;
        const float cx = m[0] * x + m[4] * y + m[8]  * z + m[12];
        const float cy = m[1] * x + m[5] * y + m[9]  * z + m[13];
        if (cx > gx * w || cx < -gx * w || cy > gy * w || cy < -gy * w) return false;
    }
    return true;
}

}

#endif
