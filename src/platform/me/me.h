
#ifndef MCPSP_PLATFORM_ME_H
#define MCPSP_PLATFORM_ME_H

struct World;

bool meInit();

bool meActive();

int meMeshCount(const World* w, int ox, int oz, int y0, int y1, int layer);

struct ChunkVertex;
int meMeshEmit(const World* w, int ox, int oz, int y0, int y1, int layer,
               struct ChunkVertex* out, int count);

int mePing(int a, int b);

int meReadBlock(const World* w, int x, int y, int z);

bool meBusy();

bool meEmitStart(const World* w, int ox, int oz, int y0, int y1, int layer,
                 struct ChunkVertex* out);

bool meEmitStartCapped(const World* w, int ox, int oz, int y0, int y1, int layer,
                       struct ChunkVertex* out, int cap);

bool meEmitReady();

int meEmitFinish(struct ChunkVertex* out, int count);

void meEmitAbort();

void meShutdown();

#endif
