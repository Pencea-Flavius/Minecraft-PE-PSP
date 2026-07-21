
#ifndef MCPSP_CLIENT_ENTITY_MOB_MODEL_H
#define MCPSP_CLIENT_ENTITY_MOB_MODEL_H

class Mob;
struct Texture;

struct SkinVertex { float u, v; unsigned int color; float x, y, z; };
struct MobPart { SkinVertex base[36]; SkinVertex mesh[36]; float px, py, pz; float xRot, yRot, zRot; bool head; };

void mobBuildBox(SkinVertex* out, float x0, float y0, float z0,
                 float x1, float y1, float z1, int tx, int ty, int w, int h, int d,
                 bool mirror, float grow);

void mobRenderParts(Mob* mob, MobPart* parts, int count, Texture* tex,
                    float x, float y, float z, float ibody, float a, unsigned int tint,
                    float babyHeadY = 8.0f, float babyHeadZ = 4.0f, float modelScale = 1.0f,
                    float overlayWhite = 0.0f, int bowPartIndex = -1, float modelScaleY = -1.0f,
                    short heldItemId = 0);

#endif
