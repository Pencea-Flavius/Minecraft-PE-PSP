
#ifndef MCPSP_GPU_TEXTURE_H
#define MCPSP_GPU_TEXTURE_H

#define TEXTURE_MAX_MIPS 7

#define TEXTURE_FORCE_8888 0

struct Texture {
    int realW, realH;
    int texW, texH;
    void* data;
    void* mip[TEXTURE_MAX_MIPS];
    int   mipCount;
    bool  swizzled;
    int   psm;
};

bool textureLoad(const char* path, Texture* out);

bool textureLoad4444(const char* path, Texture* out);

bool textureLoad16(const char* path, Texture* out, int psm);

void textureFree(Texture* tex);

void textureForgetFailures();

void textureGenMips(Texture* tex, int minSize);

bool textureLoadMipLevel(Texture* tex, int level, const char* path);

void textureSwizzle(Texture* tex);

void textureSwizzleBandInto(Texture* tex, int yTop, int bandH, const void* linearBandRGBA);

void textureBind(const Texture* tex);

void textureBindNoMip(const Texture* tex);

#endif
