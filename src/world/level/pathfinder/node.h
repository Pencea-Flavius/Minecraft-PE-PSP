
#ifndef MCPSP_WORLD_LEVEL_PATHFINDER_NODE_H
#define MCPSP_WORLD_LEVEL_PATHFINDER_NODE_H

#include "util/mth.h"

class Node {
public:
    Node(int x = 0, int y = 0, int z = 0)
    :   heapIdx(-1), g(0), h(0), f(0), cameFrom(0),
        x(x), y(y), z(z), closed(false), hash(createHash(x, y, z)) {}

    static int createHash(int x, int y, int z) {
        return (y & 0xff) | ((x & 0x7fff) << 8) | ((z & 0x7fff) << 24)
             | ((x < 0) ? 0x00800000 : 0) | ((z < 0) ? 0x00008000 : 0);
    }

    float distanceTo(Node* to) const {
        float xd = (float)(to->x - x), yd = (float)(to->y - y), zd = (float)(to->z - z);
        return Mth::sqrt(xd * xd + yd * yd + zd * zd);
    }

    bool operator==(const Node& rhs) const {
        return hash == rhs.hash && x == rhs.x && y == rhs.y && z == rhs.z;
    }
    int  hashCode() const { return hash; }
    bool inOpenSet() const { return heapIdx >= 0; }

public:
    int   heapIdx;
    float g, h, f;
    Node* cameFrom;
    short x, y, z;
    bool  closed;
private:
    int hash;
};

class TNode {
public:
    TNode(Node* node) : node(node) {}
    bool operator==(const TNode& rhs) const { return node->operator==(*rhs.node); }
    Node* node;
};

#endif
