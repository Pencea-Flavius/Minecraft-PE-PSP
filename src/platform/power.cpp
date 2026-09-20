
#include "platform/power.h"

#include <cstddef>

#include <pspkernel.h>
#include <psppower.h>

static SceUID s_sema = -1;
static int    s_held = 0;

void powerHoldInit() {
    if (s_sema < 0) s_sema = sceKernelCreateSema("mcPowerHold", 0, 1, 1, NULL);
}

static void lockCount()   { powerHoldInit(); if (s_sema >= 0) sceKernelWaitSema(s_sema, 1, NULL); }
static void unlockCount() { if (s_sema >= 0) sceKernelSignalSema(s_sema, 1); }

void powerHoldAcquire() {
    lockCount();
    if (++s_held == 1) scePowerLock(0);
    unlockCount();
}

void powerHoldRelease() {
    lockCount();

    if (s_held > 0 && --s_held == 0) scePowerUnlock(0);
    unlockCount();
}
