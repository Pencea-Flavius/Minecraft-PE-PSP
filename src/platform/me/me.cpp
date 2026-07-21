
#include "me.h"

bool meInit()     { return false; }
bool meActive()   { return false; }
void meShutdown() {}

int meMeshCount(const World*, int, int, int, int, int)                    { return -1; }
int meMeshEmit(const World*, int, int, int, int, int, ChunkVertex*, int)  { return -1; }
int mePing(int, int)                                                      { return -1; }
int meReadBlock(const World*, int, int, int)                              { return -1; }

bool meBusy()                                                        { return false; }
bool meEmitStart(const World*, int, int, int, int, int, ChunkVertex*){ return false; }
bool meEmitReady()                                                   { return false; }
int  meEmitFinish(ChunkVertex*, int)                                 { return 0; }
void meEmitAbort()                                                   {}
