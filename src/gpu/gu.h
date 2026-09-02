
#ifndef MCPSP_GPU_GU_H
#define MCPSP_GPU_GU_H

#include <pspgu.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void* guFrameAlloc(int bytes);

void* guFrameAllocPriority(int bytes);

void guListSync(void);

enum {
    GU_PHASE_FRAME_START = 1,
    GU_PHASE_TERRAIN,
    GU_PHASE_WATER,
    GU_PHASE_ENTITY,
    GU_PHASE_2D,
    GU_PHASE_PHOTO
};
void guGlobalsCheck(int phase);

enum { GUF_FRAME = 1, GUF_DIALOG, GUF_RESUME, GUF_PHOTO };

void guDeferFree(void* p);

extern int g_dither;

void guSetDither(int wanted);

int guDitherWanted(void);

#ifndef MCPSP_DIAG
#define MCPSP_DIAG 0
#endif

extern unsigned int g_canaryBroken;

unsigned int guFrameId(void);

static inline void* guFrameCopy(const void* src, int bytes) {
    void* p = guFrameAlloc(bytes);
    if (p) memcpy(p, src, bytes);
    return p;
}

#define GU_BUF_WIDTH  512
#define GU_SCR_WIDTH  480
#define GU_SCR_HEIGHT 272

void guInit(void);

void* guVramAllocTexture(unsigned int bytes);

void guVramFreeTexture(void* ptr);

unsigned int guVramFree(void);

void guTerm(void);

bool guStartFrame(unsigned int clearColor);

void guEndFrame(void);

void guFinishFrame(void);
void guPresent(void);

void guSuspendForDialog(void);
void guResumeFromDialog(void);

void guResumeFromSleep(void);
void guDialogBegin(unsigned int clearColor);
void guDialogEnd(void);
void guDialogPresent(void);

void guWaitGeIdle(void);

bool guSavePhotoPng(const char* path, int shrink);

void guPerspective(float fovDeg, float nearZ, float farZ);

void guOrtho(void);

#ifdef __cplusplus
}
#endif

#endif
