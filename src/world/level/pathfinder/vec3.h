
#ifndef MCPSP_WORLD_LEVEL_PATHFINDER_VEC3_H
#define MCPSP_WORLD_LEVEL_PATHFINDER_VEC3_H

struct Vec3 {
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    float distanceToSqr(float ox, float oy, float oz) const {
        float dx = x - ox, dy = y - oy, dz = z - oz;
        return dx * dx + dy * dy + dz * dz;
    }
};

#endif
