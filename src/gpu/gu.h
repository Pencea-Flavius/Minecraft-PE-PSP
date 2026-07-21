
#ifndef MCPSP_GPU_GU_H
#define MCPSP_GPU_GU_H

#include <pspgu.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void* guFrameCopy(const void* src, int bytes) {
    void* p = sceGuGetMemory(bytes);
    memcpy(p, src, bytes);
    return p;
}

#define GU_BUF_WIDTH  512
#define GU_SCR_WIDTH  480
#define GU_SCR_HEIGHT 272

void guInit(void);

void guTerm(void);

void guStartFrame(unsigned int clearColor);

void guEndFrame(void);

void guFinishFrame(void);
void guPresent(void);

bool guSavePhotoPng(const char* path);

void guPerspective(float fovDeg, float nearZ, float farZ);

void guOrtho(void);

#ifdef __cplusplus
}
#endif

#endif
