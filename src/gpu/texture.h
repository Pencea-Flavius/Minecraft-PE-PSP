
#ifndef MCPSP_GPU_TEXTURE_H
#define MCPSP_GPU_TEXTURE_H

#define TEXTURE_MAX_MIPS 7

#define TEXTURE_FORCE_8888 0

#define TEXTURE_USE_VRAM 1

struct Texture {
    int realW, realH;
    int texW, texH;
    void* data;
    void* mip[TEXTURE_MAX_MIPS];
    int   mipCount;
    bool  swizzled;
    bool  vram;
    int   psm;
};

bool textureLoad(const char* path, Texture* out);

bool textureLoad4444(const char* path, Texture* out);

bool textureLoad16(const char* path, Texture* out, int psm);

bool textureLoad16Optional(const char* path, Texture* out, int psm);

bool textureLoad16At(const char* path, unsigned int offset, Texture* out, int psm);

bool textureLoad16Mem(const char* name, const unsigned char* mem, unsigned int offset, unsigned int size,
                      Texture* out, int psm);

bool textureLoadVram(const char* path, Texture* out, int psm);

void textureFree(Texture* tex);

void textureForgetFailures();

extern unsigned int g_textureBindFailures;

extern unsigned int g_textureLoadFailures;
extern const char*  g_textureFailReason;
extern unsigned int g_textureFailHeapUsed;
extern unsigned int g_textureFailHeapBig;
extern char g_textureLastFailed[80];

void textureGenMips(Texture* tex, int minSize);

bool textureLoadMipLevel(Texture* tex, int level, const char* path);

void textureSwizzle(Texture* tex);

void textureSwizzleBandInto(Texture* tex, int yTop, int bandH, const void* linearBandRGBA);

void textureBind(const Texture* tex);

void textureBindNoMip(const Texture* tex);

void textureBindLastBoundReset();

void textureMipAuto();

#endif
