
#pragma once

#include "gpu/texture.h"

enum SkinAnimBit {
    SKIN_ANIM_ARMS_DOWN = 0,
    SKIN_ANIM_ARMS_OUT_FRONT,
    SKIN_ANIM_NO_LEG_ANIM,
    SKIN_ANIM_HAS_IDLE,
    SKIN_ANIM_FORCE_ANIM,
    SKIN_ANIM_SINGLE_LEGS,
    SKIN_ANIM_SINGLE_ARMS,
    SKIN_ANIM_STATUE_OF_LIBERTY,
    SKIN_ANIM_DONT_RENDER_ARMOUR,
    SKIN_ANIM_NO_BOBBING,
    SKIN_ANIM_DISABLE_HEAD,
    SKIN_ANIM_DISABLE_ARM0,
    SKIN_ANIM_DISABLE_ARM1,
    SKIN_ANIM_DISABLE_TORSO,
    SKIN_ANIM_DISABLE_LEG0,
    SKIN_ANIM_DISABLE_LEG1,
    SKIN_ANIM_DISABLE_HAIR,
};

struct SkinBox {
    unsigned char part;
    float x, y, z, w, h, d, u, v;
};

#define SKIN_MAX_SKINS 96
#define SKIN_MAX_BOXES 64

struct SkinEntry {
    char name[32];
    char theme[32];
    char file[32];
    char capeFile[32];
    unsigned int offset, size;
    unsigned int anim;
    int boxFirst, boxCount;
    short texH;
    float helmetY;
};

struct SkinPack {
    char path[128];
    char name[48];
    int skinCount;
    SkinEntry* skins;
    int boxCount;
    SkinBox* boxes;

    int capeCount;
    struct Cape { char file[32]; unsigned int offset, size; }* capes;
};

bool skinPackOpen(const char* path, SkinPack* out);
void skinPackClose(SkinPack* p);

bool skinPackLoadTexture(const SkinPack& p, int idx, Texture* out);
bool skinPackLoadCape(const SkinPack& p, int idx, Texture* out);

#define SKIN_DEFAULT_COUNT 8
const char* skinDefaultName(int i);
bool        skinDefaultLoadTexture(int i, Texture* out);

#define SKIN_MAX_FAVORITES 10
int         skinFavoriteCount(void);
const char* skinFavorite(int i);
int         skinFavoriteFind(const char* entry);

void        skinFavoriteAdd(const char* entry);
void        skinFavoriteRemove(int i);
void        skinFavoritesOptionSet(const char* value);
void        skinFavoritesOptionGet(char* out, int cap);

void        skinOptionSet(const char* value);
const char* skinOptionGet(void);

Texture*         skinTexture(void);
Texture*         skinCapeTexture(void);
const SkinEntry* skinCurrent(void);
int              skinCurrentPackSize(void);
const SkinBox*   skinBoxes(void);
unsigned int     skinAnim(void);

inline bool      skinNoViewBob(void) {
    return (skinAnim() & ((1u << SKIN_ANIM_NO_LEG_ANIM) | (1u << SKIN_ANIM_NO_BOBBING))) != 0;
}

unsigned int     skinGeneration(void);
