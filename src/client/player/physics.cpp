#include "client/player/physics.h"
#include "world/level/world.h"
#include "world/level/chunk/chunk.h"
#include "world/level/tile/tile.h"
#include <cmath>

static const float FACE_EPS = 1.0e-3f;

float clipYCollide(float x0, float y0, float z0, float x1, float y1, float z1,
                          const float c[6], float ya) {
    if (c[3] <= x0 + FACE_EPS || c[0] >= x1 - FACE_EPS) return ya;
    if (c[5] <= z0 + FACE_EPS || c[2] >= z1 - FACE_EPS) return ya;
    if (ya > 0 && c[4] <= y0 + FACE_EPS) { float m = y0 - c[4]; if (m < ya) ya = m; }
    if (ya < 0 && c[1] >= y1 - FACE_EPS) { float m = y1 - c[1]; if (m > ya) ya = m; }
    return ya;
}

float clipXCollide(float x0, float y0, float z0, float x1, float y1, float z1,
                          const float c[6], float xa) {
    if (c[4] <= y0 + FACE_EPS || c[1] >= y1 - FACE_EPS) return xa;
    if (c[5] <= z0 + FACE_EPS || c[2] >= z1 - FACE_EPS) return xa;
    if (xa > 0 && c[3] <= x0 + FACE_EPS) { float m = x0 - c[3]; if (m < xa) xa = m; }
    if (xa < 0 && c[0] >= x1 - FACE_EPS) { float m = x1 - c[0]; if (m > xa) xa = m; }
    return xa;
}

float clipZCollide(float x0, float y0, float z0, float x1, float y1, float z1,
                          const float c[6], float za) {
    if (c[3] <= x0 + FACE_EPS || c[0] >= x1 - FACE_EPS) return za;
    if (c[4] <= y0 + FACE_EPS || c[1] >= y1 - FACE_EPS) return za;
    if (za > 0 && c[5] <= z0 + FACE_EPS) { float m = z0 - c[5]; if (m < za) za = m; }
    if (za < 0 && c[2] >= z1 - FACE_EPS) { float m = z1 - c[2]; if (m > za) za = m; }
    return za;
}

int getBlockAABBs(const World* w, int x, int y, int z, BlockAABB out[3]) {
    unsigned char id = worldBlock(w, x, y, z);
    Tile* t = Tile::tiles[id];
    return t ? t->getAABB(w, x, y, z, out) : 0;
}
