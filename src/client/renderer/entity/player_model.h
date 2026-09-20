
#ifndef MCPSP_CLIENT_PLAYER_MODEL_H
#define MCPSP_CLIENT_PLAYER_MODEL_H

void playerModelRender(float a);

void playerModelRenderPreview(float sx, float sy, float scale);

void playerModelRenderPaperDoll(float a, bool displayGui);
extern int g_animatedCharacter;

struct Texture;
struct SkinBox;
struct MobVertex;

struct SkinDraw {
    Texture* tex;
    const MobVertex (*boxMesh)[36];
    const unsigned char* boxPart;
    int boxCount;
    unsigned int anim;
    Texture* cape;
    float helmetY;
};

int playerModelBuildSkinBoxes(const SkinBox* boxes, int n, MobVertex (*out)[36],
                              unsigned char* part, int max, float texH = 32.0f);

enum { SKIN_POSE_WALK = 0, SKIN_POSE_SNEAK, SKIN_POSE_ATTACK, SKIN_POSE_COUNT };

void playerModelRenderSkinPreview(const SkinDraw& d, float x, float y, float w, float h,
                                  float yRotDeg, float walkPos, float walkSpeed,
                                  int pose = SKIN_POSE_WALK, float swing = 0.0f);

void playerModelRenderWornPreview(float x, float y, float w, float h, float yRotDeg);

void playerModelDrawSkinBoxes(int part, unsigned int brCol, const float* toWorld);

#endif
