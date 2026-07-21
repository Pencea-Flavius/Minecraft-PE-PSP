
#ifndef MCPSP_WORLD_LEVEL_PATHFINDER_PATH_FINDER_H
#define MCPSP_WORLD_LEVEL_PATHFINDER_PATH_FINDER_H

#include "world/level/pathfinder/node.h"
#include "world/level/pathfinder/binary_heap.h"

class Path;
class Entity;
class Level;

class PathFinder {
public:
    PathFinder();

    void setLevel(Level* l) { level = l; }

    bool findPath(Path* path, Entity* from, Entity* to, float maxDist);
    bool findPath(Path* path, Entity* from, int x, int y, int z, float maxDist);

    bool avoidWater;

private:
    bool  findPathTo(Path& path, Entity* e, float xt, float yt, float zt, float maxDist);
    bool  findPathNodes(Path& path, Entity* e, Node* from, Node* to, const Node* size, float maxDist);
    int   getNeighbors(Entity* e, Node* pos, const Node* size, Node* target, float maxDist);
    Node* getNodeFor(Entity* e, int x, int y, int z, const Node* size, int jumpSize);
    Node* getNode(int x, int y, int z);
    int   isFree(Entity* e, int x, int y, int z, const Node* size);
    void  reconstruct(Path& path, Node* from, Node* to);
    Node* newNode(int x, int y, int z);

    static const int MAX_NODES = 2048;

    static const int TABLE_SIZE = 4096;
    static const int TYPE_FENCE = 1, TYPE_LAVA = 2, TYPE_WATER = 3,
                     TYPE_BLOCKED = 4, TYPE_OPEN = 5, TYPE_WALKABLE = 6;

    Level*     level;
    BinaryHeap openSet;
    Node       _nodes[MAX_NODES];
    short      _table[TABLE_SIZE];
    int        _nodeIndex;
    Node*      neighbors[32];
};

#endif
