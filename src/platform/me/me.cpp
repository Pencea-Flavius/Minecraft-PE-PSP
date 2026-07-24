#include "me.h"
#include "../path.h"
#include "world/level/world.h"

#include <pspsdk.h>
#include <pspkernel.h>
#include <psppower.h>
extern "C" {
#include <libpspexploit.h>
}
#include <cstring>
#include <malloc.h>

#define hwp          volatile u32*
#define hw(addr)     (*((hwp)(addr)))

#define UNCACHED_USER_MASK    0x40000000
#define ME_HANDLER_BASE       0xbfc00000

#define delayPipeline() asm volatile("nop; nop; nop; nop; nop; nop; nop;")

#define ME_STACK_SIZE (48 * 1024)
static unsigned char g_meStack[ME_STACK_SIZE] __attribute__((aligned(16)));

struct MeJob {
    volatile int cmd;
    volatile int done;
    volatile int result;
    const World*  w;
    int ox, oz, y0, y1, layer;
    ChunkVertex*  out;
    int cap;
};

static MeJob* g_job = nullptr;
static void*  g_jobBase = nullptr;
static bool   g_active = false;

static volatile MeJob* volatile g_meJobPtr = nullptr;

static inline void meInvalidateDCacheAll() {
    for (int i = 0; i < 8192; i += 64) {
        asm("cache 0x14, 0(%0)" :: "r"(i));
        asm("cache 0x14, 0(%0)" :: "r"(i));
    }
    asm volatile("sync");
}

static inline void meHalt() {
    asm volatile(".word 0x70000000");
}

__attribute__((noinline, aligned(4), used))
static void meLoop() {
    do {
        meInvalidateDCacheAll();
    } while (!g_meJobPtr);

    volatile MeJob* job = g_meJobPtr;

    for (;;) {
        int cmd = job->cmd;
        if (cmd == 2) break;
        if (cmd == 1) {
            int c;
            if (job->layer == 999) {
                c = job->ox + job->oz;
            } else if (job->layer == 998) {
                c = (int)worldBlock(job->w, job->ox, job->y0, job->oz);
            } else {
                meInvalidateDCacheAll();
                c = meshPass(job->w, job->ox, job->oz, job->y0, job->y1,
                             job->out, job->layer, job->cap);
            }
            job->result = c;
            asm volatile("sync");
            job->done = 1;
            asm volatile("sync");
            while (job->cmd == 1) {
                delayPipeline();
            }
        } else {
            delayPipeline();
        }
    }

    job->done = 2;
    meHalt();
}

extern char __start__me_section;
extern char __stop__me_section;

__attribute__((section("_me_section"), noinline, aligned(4), used))
static void meHandler() {
    hw(0xbc100040) = 0x02;
    hw(0xbc100050) = 0x0f;
    hw(0xbc100004) = 0xffffffff;
    asm("sync");

    asm volatile(
        "la          $sp, %1             \n"
        "li          $k0, 0x30000000     \n"
        "mtc0        $k0, $12            \n"
        "sync                            \n"
        "la          $k0, %0             \n"
        "li          $k1, 0x80000000     \n"
        "or          $k0, $k0, $k1       \n"
        "cache       0x8, 0($k0)         \n"
        "sync                            \n"
        "jr          $k0                 \n"
        "nop\n"
        :
        : "i" (meLoop), "i" (&g_meStack[ME_STACK_SIZE - 16])
        : "k0", "k1"
    );
}

static void kernelMeInit() {
    int k1 = pspSdkSetK1(0);
    const u32 sectionSize = (u32)(&__stop__me_section - &__start__me_section);
    memcpy((void*)ME_HANDLER_BASE, (void*)&__start__me_section, sectionSize);
    sceKernelDcacheWritebackInvalidateAll();

    hw(0xBC10004C) = 0x14;
    hw(0xBC10004C) = 0x00;
    asm volatile("sync");

    pspXploitRepairKernel();
    pspSdkSetK1(k1);
}

bool meInit() {
    if (g_active)
        return true;

    int res = pspXploitInitKernelExploit();
    if (res == 0) {
        res = pspXploitDoKernelExploit();
        if (res == 0) {

            g_jobBase = memalign(16, (sizeof(MeJob) + 15) & ~15);
            memset(g_jobBase, 0, sizeof(MeJob));
            g_job = (MeJob*)(UNCACHED_USER_MASK | (u32)g_jobBase);
            g_job->cmd = 0;
            g_job->done = 0;

            g_meJobPtr = g_job;
            sceKernelDcacheWritebackInvalidateAll();

            pspXploitExecuteKernel((void*)kernelMeInit);
            g_active = true;
            return true;
        }
    }
    return false;
}

bool meActive() { return g_active; }

static int meRun(const World* w, int ox, int oz, int y0, int y1, int layer,
                 ChunkVertex* out) {
    if (!g_active)
        return -1;

    static struct {
        unsigned char* blocks;
        unsigned char* data;
        unsigned char* light;
        unsigned char* heightmap;
    } meWorld;
    if (w) {
        meWorld.blocks    = (unsigned char*)(UNCACHED_USER_MASK | (u32)w->blocks);
        meWorld.data      = (unsigned char*)(UNCACHED_USER_MASK | (u32)w->data);
        meWorld.light     = (unsigned char*)(UNCACHED_USER_MASK | (u32)w->light);
        meWorld.heightmap = (unsigned char*)(UNCACHED_USER_MASK | (u32)w->heightmap);
        g_job->w = (const World*)(UNCACHED_USER_MASK | (u32)&meWorld);
    } else {
        g_job->w = w;
    }
    g_job->ox = ox; g_job->oz = oz;
    g_job->y0 = y0; g_job->y1 = y1;
    g_job->layer = layer;
    g_job->out = out;
    g_job->result = -1;
    g_job->done = 0;

    sceKernelDcacheWritebackInvalidateAll();

    g_job->cmd = 1;
    asm volatile("sync");

    const unsigned WATCHDOG = 200000000u;
    unsigned spins = 0;
    while (!g_job->done) {
        asm volatile("sync");
        if (++spins > WATCHDOG) {
            g_active = false;
            return -2;
        }
    }

    int r = g_job->result;
    g_job->cmd = 0;
    asm volatile("sync");
    return r;
}

int meMeshCount(const World* w, int ox, int oz, int y0, int y1, int layer) {
    return meRun(w, ox, oz, y0, y1, layer, (ChunkVertex*)0);
}

int mePing(int a, int b) {
    return meRun((const World*)0, a, b, 0, 0, 999, (ChunkVertex*)0);
}

int meReadBlock(const World* w, int x, int y, int z) {
    return meRun(w, x, z, y, y, 998, (ChunkVertex*)0);
}

int meMeshEmit(const World* w, int ox, int oz, int y0, int y1, int layer,
               ChunkVertex* out, int count) {
    ChunkVertex* uview = (ChunkVertex*)(UNCACHED_USER_MASK | (u32)out);
    int r = meRun(w, ox, oz, y0, y1, layer, uview);
    if (r >= 0)
        sceKernelDcacheWritebackInvalidateRange(out, count * sizeof(ChunkVertex));
    return r;
}

static bool g_meBusy = false;

bool meBusy() { return g_meBusy; }

bool meEmitStart(const World* w, int ox, int oz, int y0, int y1, int layer,
                 ChunkVertex* out) {
    return meEmitStartCapped(w, ox, oz, y0, y1, layer, out, 0x7fffffff);
}

bool meEmitStartCapped(const World* w, int ox, int oz, int y0, int y1, int layer,
                       ChunkVertex* out, int cap) {
    if (!g_active || g_meBusy || !w)
        return false;

    static struct {
        unsigned char* blocks;
        unsigned char* data;
        unsigned char* light;
        unsigned char* heightmap;
    } meWorld;
    meWorld.blocks    = (unsigned char*)(UNCACHED_USER_MASK | (u32)w->blocks);
    meWorld.data      = (unsigned char*)(UNCACHED_USER_MASK | (u32)w->data);
    meWorld.light     = (unsigned char*)(UNCACHED_USER_MASK | (u32)w->light);
    meWorld.heightmap = (unsigned char*)(UNCACHED_USER_MASK | (u32)w->heightmap);
    g_job->w = (const World*)(UNCACHED_USER_MASK | (u32)&meWorld);
    g_job->out = out ? (ChunkVertex*)(UNCACHED_USER_MASK | (u32)out) : (ChunkVertex*)0;
    g_job->ox = ox; g_job->oz = oz;
    g_job->y0 = y0; g_job->y1 = y1;
    g_job->layer = layer;
    g_job->cap = cap;
    g_job->result = -1;
    g_job->done = 0;

    sceKernelDcacheWritebackInvalidateAll();
    g_job->cmd = 1;
    asm volatile("sync");
    g_meBusy = true;
    return true;
}

bool meEmitReady() {
    return g_meBusy && g_job->done;
}

int meEmitFinish(ChunkVertex* out, int count) {
    int r = g_job->result;
    g_job->cmd = 0;
    asm volatile("sync");
    if (out && count > 0)
        sceKernelDcacheWritebackInvalidateRange(out, (size_t)count * sizeof(ChunkVertex));
    g_meBusy = false;
    return r;
}

void meEmitAbort() {
    g_active = false;
    g_meBusy = false;
}

void meShutdown() {
    if (!g_active)
        return;

    g_job->cmd = 2;
    asm volatile("sync");
    unsigned spins = 0;
    while (g_job->done < 2) {
        asm volatile("sync");
        if (++spins > 200000000u) break;
    }

    if (g_jobBase) {
        free(g_jobBase);
        g_jobBase = nullptr;
    }
    g_job = nullptr;
    g_meJobPtr = nullptr;
    g_active = false;
}
