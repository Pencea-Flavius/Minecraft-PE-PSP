#ifndef MCPSP_MESH_WORKER_H
#define MCPSP_MESH_WORKER_H

struct World;
struct ChunkMesh;

namespace MeshWorker {

bool start();

void stop();
bool isRunning();

bool enqueue(ChunkMesh* c, const World* w, int si);
int  pendingCount();

int  drain();

}

#endif
