
#ifndef MCPSP_UTIL_MTH_H
#define MCPSP_UTIL_MTH_H

#include <cmath>

namespace Mth {
    static const float PI = 3.14159265358979323846f;

    static inline int floor(float v) {
        int i = (int)v;
        return (v < 0 && (float)i != v) ? i - 1 : i;
    }
    static inline float abs(float v) { return v < 0 ? -v : v; }
    static inline int   Max(int a, int b) { return a > b ? a : b; }
    static inline float sqrt(float v) { return sqrtf(v); }
    static inline float clamp(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
    static inline float absMax(float a, float b) {
        float aa = a < 0 ? -a : a;
        float bb = b < 0 ? -b : b;
        return aa > bb ? aa : bb;
    }
}

#endif
