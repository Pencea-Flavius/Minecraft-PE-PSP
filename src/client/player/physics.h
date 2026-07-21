#pragma once

struct World;
struct BlockAABB { float x0, y0, z0, x1, y1, z1; };
int getBlockAABBs(const World* w, int x, int y, int z, BlockAABB out[3]);

float clipXCollide(float x0, float y0, float z0, float x1, float y1, float z1, const float c[6], float xa);
float clipYCollide(float x0, float y0, float z0, float x1, float y1, float z1, const float c[6], float ya);
float clipZCollide(float x0, float y0, float z0, float x1, float y1, float z1, const float c[6], float za);
