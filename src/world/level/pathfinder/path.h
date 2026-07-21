
#ifndef MCPSP_WORLD_LEVEL_PATHFINDER_PATH_H
#define MCPSP_WORLD_LEVEL_PATHFINDER_PATH_H

#include "world/level/pathfinder/node.h"
#include "world/level/pathfinder/vec3.h"

class Entity;

class Path {
public:
    Path();
    ~Path();

    void  copyNodes(Node** nodes, int length);
    void  destroy();
    void  next();

    int   getSize() const;
    bool  isEmpty() const;
    bool  isDone() const;

    Node* last() const;
    Node* get(int i) const;
    int   getIndex() const;
    void  setIndex(int index);

    Vec3  currentPos(Entity* e) const;
    Node* currentPos();
    Vec3  getPos(Entity* e, int index) const;

private:

    static const int MAX_PATH = 64;
    Node nodes[MAX_PATH];
    int length;
    int index;
    static int p;
};

#endif
