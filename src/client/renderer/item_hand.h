#pragma once

#define SWING_DURATION 8
extern bool  g_swinging;
extern int   g_swingTime;
extern float g_attackAnim, g_oAttackAnim;

void playerSwing(void);
void loadCharIfNeeded(void);
void itemHandTick(void);
void itemHandDraw(float a, float bs, float bc);

struct ChunkVertex;
struct Texture;
int  itemBuildBlockMesh(short id, unsigned char data, ChunkVertex* out);
int  itemBuildFlatMesh (short id, unsigned char data, ChunkVertex* out, int bowStage, int cap = 100000);
int  bowStageIcon(float ticks);
bool itemIsFlat2D(short id);
const Texture* itemFlatTexture(short id, unsigned char data);
const Texture* itemFlatIconUV(short id, unsigned char data,
                            float* u0, float* v0, float* u1, float* v1);
