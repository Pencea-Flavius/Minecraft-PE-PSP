#include "client/renderer/level/mesh_worker.h"
#include "world/level/chunk/chunk.h"
#include "world/level/world.h"

#include <pspkernel.h>
#include <pspthreadman.h>
#include <cstdio>

namespace MeshWorker {

static const int PENDING_CAP = 32;

static const int DONE_CAP    = 6;

static const int WORKER_CAP_OPAQUE = 16384;
static const int WORKER_CAP_WL     = 4096;

struct Job {
    ChunkMesh*     c;
    const World*   w;
    int            si;
    unsigned short seq;
};

struct DoneJob {
    ChunkMesh*        c;
    int               si;
    unsigned short    seq;
    SectionMeshResult r;
};

static Job     g_pending[PENDING_CAP];
static int     g_pendHead = 0, g_pendTail = 0, g_pendCount = 0;
static DoneJob g_done[DONE_CAP];
static int     g_doneHead = 0, g_doneTail = 0, g_doneCount = 0;

static SceUID g_mutex     = -1;
static SceUID g_pendSema  = -1;
static SceUID g_doneSpace = -1;
static SceUID g_thread    = -1;
static volatile bool g_stop = false;
static MeshScratch*  g_scratch = 0;

static inline void lock()   { sceKernelWaitSema(g_mutex, 1, 0); }
static inline void unlock() { sceKernelSignalSema(g_mutex, 1); }

bool isRunning() { return g_thread >= 0 && !g_stop; }

int pendingCount() {
    if (g_mutex < 0) return 0;
    lock(); int n = g_pendCount; unlock();
    return n;
}

bool enqueue(ChunkMesh* c, const World* w, int si) {
    if (!isRunning()) return false;
    lock();
    if (g_pendCount >= PENDING_CAP) { unlock(); return false; }

    unsigned short seq = ++c->sec[si].meshSeq;
    Job& j = g_pending[g_pendTail];
    j.c = c; j.w = w; j.si = si; j.seq = seq;
    g_pendTail = (g_pendTail + 1) % PENDING_CAP;
    g_pendCount++;

    c->sec[si].dirty = false;
    unlock();
    sceKernelSignalSema(g_pendSema, 1);
    return true;
}

static int workerEntry(SceSize, void*) {
    while (!g_stop) {

        if (sceKernelWaitSema(g_pendSema, 1, 0) < 0) break;
        if (g_stop) break;

        lock();
        if (g_pendCount == 0) { unlock(); continue; }
        Job j = g_pending[g_pendHead];
        g_pendHead = (g_pendHead + 1) % PENDING_CAP;
        g_pendCount--;
        unlock();

        if (j.c->sec[j.si].meshSeq != j.seq) continue;

        if (sceKernelWaitSema(g_doneSpace, 1, 0) < 0) break;
        if (g_stop) { sceKernelSignalSema(g_doneSpace, 1); break; }

        SectionMeshResult r;
        chunkComputeSection(j.c, j.w, j.si, g_scratch, &r);

        lock();
        DoneJob& d = g_done[g_doneTail];
        d.c = j.c; d.si = j.si; d.seq = j.seq; d.r = r;
        g_doneTail = (g_doneTail + 1) % DONE_CAP;
        g_doneCount++;
        unlock();
    }
    return 0;
}

int drain() {
    if (g_mutex < 0) return 0;
    int applied = 0;
    for (;;) {
        lock();
        if (g_doneCount == 0) { unlock(); break; }
        DoneJob d = g_done[g_doneHead];
        g_doneHead = (g_doneHead + 1) % DONE_CAP;
        g_doneCount--;
        unlock();
        sceKernelSignalSema(g_doneSpace, 1);

        if (d.c->sec[d.si].meshSeq == d.seq) {
            chunkApplySection(d.c, d.si, &d.r);
            applied++;
        } else {

            sectionMeshResultFree(&d.r);
        }
    }
    return applied;
}

bool start() {
    if (g_thread >= 0) return true;
    g_scratch = meshScratchCreate(WORKER_CAP_OPAQUE, WORKER_CAP_WL);
    if (!g_scratch) { printf("MeshWorker: no heap for scratch, staying sync\n"); return false; }

    g_mutex     = sceKernelCreateSema("mcMeshMtx",   0, 1, 1, NULL);
    g_pendSema  = sceKernelCreateSema("mcMeshPend",  0, 0, PENDING_CAP, NULL);
    g_doneSpace = sceKernelCreateSema("mcMeshDone",  0, DONE_CAP, DONE_CAP, NULL);
    if (g_mutex < 0 || g_pendSema < 0 || g_doneSpace < 0) {
        printf("MeshWorker: sema create failed\n");
        return false;
    }
    g_stop = false;
    g_pendHead = g_pendTail = g_pendCount = 0;
    g_doneHead = g_doneTail = g_doneCount = 0;

    g_thread = sceKernelCreateThread("mcChunkMesh", workerEntry, 0x21, 256 * 1024,
                                     PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU, NULL);
    if (g_thread < 0) { printf("MeshWorker: thread create failed 0x%08X\n", (unsigned)g_thread); return false; }
    sceKernelStartThread(g_thread, 0, NULL);
    printf("MeshWorker: started\n");
    return true;
}

void stop() {
    if (g_thread < 0) return;

    g_stop = true;
    sceKernelSignalSema(g_pendSema, 1);
    sceKernelSignalSema(g_doneSpace, 1);
    for (int i = 0; i < 200; i++) {
        SceKernelThreadInfo info;
        info.size = sizeof(info);
        if (sceKernelReferThreadStatus(g_thread, &info) < 0) break;
        if (info.status & PSP_THREAD_STOPPED) break;
        sceKernelDelayThread(2000);
    }
    sceKernelDeleteThread(g_thread);
    g_thread = -1;

    for (int i = 0; i < g_doneCount; i++)
        sectionMeshResultFree(&g_done[(g_doneHead + i) % DONE_CAP].r);
    g_doneHead = g_doneTail = g_doneCount = 0;
    g_pendHead = g_pendTail = g_pendCount = 0;
    sceKernelDeleteSema(g_mutex);     g_mutex = -1;
    sceKernelDeleteSema(g_pendSema);  g_pendSema = -1;
    sceKernelDeleteSema(g_doneSpace); g_doneSpace = -1;

}

}
