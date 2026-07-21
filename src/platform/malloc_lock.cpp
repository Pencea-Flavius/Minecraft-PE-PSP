
#include <malloc.h>
#include <pspkernel.h>
#include <pspthreadman.h>

static SceUID g_mallocSema  = -1;
static SceUID g_mallocOwner = -1;
static int    g_mallocDepth = 0;

extern "C" void __malloc_lock(struct _reent*)
{
    if (g_mallocSema < 0) {
        g_mallocSema = sceKernelCreateSema("mcMallocLock", 0, 1, 1, NULL);
        if (g_mallocSema < 0) return;
    }
    SceUID self = sceKernelGetThreadId();
    if (g_mallocOwner == self) { ++g_mallocDepth; return; }
    sceKernelWaitSema(g_mallocSema, 1, NULL);
    g_mallocOwner = self;
    g_mallocDepth = 1;
}

extern "C" void __malloc_unlock(struct _reent*)
{
    if (g_mallocSema < 0) return;
    if (g_mallocOwner != sceKernelGetThreadId()) return;
    if (--g_mallocDepth > 0) return;
    g_mallocOwner = -1;
    sceKernelSignalSema(g_mallocSema, 1);
}
