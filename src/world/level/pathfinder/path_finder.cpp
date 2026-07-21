#include "world/level/pathfinder/path_finder.h"
#include "world/level/pathfinder/path.h"
#include "world/entity/entity.h"
#include "world/level/level.h"
#include "world/level/chunk/chunk.h"
#include "util/mth.h"
#include <cstring>

PathFinder::PathFinder() : avoidWater(false), level(0), _nodeIndex(0) {}

bool PathFinder::findPath(Path* path, Entity* from, Entity* to, float maxDist) {
    return findPathTo(*path, from, to->x, to->bb.y0, to->z, maxDist);
}
bool PathFinder::findPath(Path* path, Entity* from, int x, int y, int z, float maxDist) {
    return findPathTo(*path, from, x + 0.5f, y + 0.5f, z + 0.5f, maxDist);
}

bool PathFinder::findPathTo(Path& path, Entity* e, float xt, float yt, float zt, float maxDist) {
    memset(_table, 0, sizeof(_table));
    _nodeIndex = 0;

    int startY;
    if (e->isInWater()) {
        startY = (int)(e->bb.y0);
        while (isWaterId((unsigned char)level->getTile(Mth::floor(e->x), startY, Mth::floor(e->z)))) ++startY;
    } else {
        startY = Mth::floor(e->bb.y0 + 0.5f);
    }

    Node* from = getNode(Mth::floor(e->bb.x0), startY, Mth::floor(e->bb.z0));

    const int xx0 = Mth::floor(xt - e->bbWidth / 2);
    const int yy0 = Mth::floor(yt);
    const int zz0 = Mth::floor(zt - e->bbWidth / 2);

    Node* to = 0;
    if (level->getTile(xx0, yy0 - 1, zz0)) {
        to = getNode(xx0, yy0, zz0);
    } else {
        const int xx1 = Mth::floor(xt + e->bbWidth / 2);
        const int zz1 = Mth::floor(zt + e->bbWidth / 2);
        for (int xx = xx0; xx <= xx1 && !to; ++xx)
            for (int zz = zz0; zz <= zz1; ++zz)
                if (level->getTile(xx, yy0 - 1, zz) != 0) { to = getNode(xx, yy0, zz); break; }
        if (!to) {
            int yy = yy0;
            while (!level->getTile(xx0, yy - 1, zz0) && yy > 0) --yy;
            to = getNode(xx0, yy, zz0);
        }
    }

    Node size(Mth::floor(e->bbWidth + 1), Mth::floor(e->bbHeight + 1), Mth::floor(e->bbWidth + 1));
    return findPathNodes(path, e, from, to, &size, maxDist);
}

bool PathFinder::findPathNodes(Path& path, Entity* e, Node* from, Node* to, const Node* size, float maxDist) {

    if (!from || !to) { path.destroy(); return false; }
    from->g = 0;
    from->h = from->distanceTo(to);
    from->f = from->h;

    openSet.clear();
    openSet.insert(from);

    Node* closest = from;

    const int MAX_POP_ITERS = 512;
    int iters = 0;

    while (!openSet.isEmpty()) {
        if (++iters > MAX_POP_ITERS) break;
        Node* x = openSet.pop();
        if (*x == *to) { reconstruct(path, from, to); return true; }
        if (x->distanceTo(to) < closest->distanceTo(to)) closest = x;
        x->closed = true;

        int n = getNeighbors(e, x, size, to, maxDist);
        for (int i = 0; i < n; i++) {
            Node* y = neighbors[i];
            if (y->closed) continue;
            float tentative_g = x->g + x->distanceTo(y);
            if (!y->inOpenSet() || tentative_g < y->g) {
                y->cameFrom = x;
                y->g = tentative_g;
                y->h = y->distanceTo(to);
                if (y->inOpenSet()) openSet.changeCost(y, y->g + y->h);
                else { y->f = y->g + y->h; openSet.insert(y); }
            }
        }
    }

    if (closest == from) return false;
    reconstruct(path, from, closest);
    return true;
}

int PathFinder::getNeighbors(Entity* e, Node* pos, const Node* size, Node* target, float maxDist) {
    int p = 0;
    int jumpSize = 0;
    if (isFree(e, pos->x, pos->y + 1, pos->z, size) == TYPE_OPEN) jumpSize = 1;

    Node* n = getNodeFor(e, pos->x, pos->y, pos->z + 1, size, jumpSize);
    Node* w = getNodeFor(e, pos->x - 1, pos->y, pos->z, size, jumpSize);
    Node* ea = getNodeFor(e, pos->x + 1, pos->y, pos->z, size, jumpSize);
    Node* s = getNodeFor(e, pos->x, pos->y, pos->z - 1, size, jumpSize);

    if (n  && !n->closed  && n->distanceTo(target)  < maxDist) neighbors[p++] = n;
    if (w  && !w->closed  && w->distanceTo(target)  < maxDist) neighbors[p++] = w;
    if (ea && !ea->closed && ea->distanceTo(target) < maxDist) neighbors[p++] = ea;
    if (s  && !s->closed  && s->distanceTo(target)  < maxDist) neighbors[p++] = s;
    return p;
}

Node* PathFinder::getNodeFor(Entity* e, int x, int y, int z, const Node* size, int jumpSize) {
    Node* best = 0;
    int pathType = isFree(e, x, y, z, size);
    if (pathType == TYPE_WALKABLE) return getNode(x, y, z);
    if (pathType == TYPE_OPEN) best = getNode(x, y, z);
    if (!best && jumpSize > 0 && pathType != TYPE_FENCE && isFree(e, x, y + jumpSize, z, size) == TYPE_OPEN) {
        best = getNode(x, y + jumpSize, z);
        y += jumpSize;
    }
    if (best) {
        int drop = 0, cost = 0;
        while (y > 0) {
            cost = isFree(e, x, y - 1, z, size);
            if (avoidWater && cost == TYPE_WATER) return 0;
            if (cost != TYPE_OPEN) break;
            if (++drop >= 4) return 0;
            if (--y > 0) best = getNode(x, y, z);
        }
        if (cost == TYPE_LAVA) return 0;
    }
    return best;
}

Node* PathFinder::getNode(int x, int y, int z) {
    int hash = Node::createHash(x, y, z);
    int slot = (hash * 0x9e3779b1u) & (TABLE_SIZE - 1);
    while (_table[slot]) {
        Node* n = &_nodes[_table[slot] - 1];
        if (n->x == x && n->y == y && n->z == z) return n;
        slot = (slot + 1) & (TABLE_SIZE - 1);
    }
    Node* node = newNode(x, y, z);
    if (!node) return 0;
    _table[slot] = (short)(_nodeIndex);
    return node;
}

int PathFinder::isFree(Entity* , int x, int y, int z, const Node* size) {
    for (int xx = x; xx < x + size->x; xx++)
        for (int yy = y; yy < y + size->y; yy++)
            for (int zz = z; zz < z + size->z; zz++) {
                int id = level->getTile(xx, yy, zz);
                if (id <= 0) continue;
                unsigned char b = (unsigned char)id;
                if (isDoor(b)) {

                    if (!(level->getData(xx, yy, zz) & 4)) return TYPE_BLOCKED;
                    continue;
                } else if (isWaterId(b)) {
                    if (avoidWater) return TYPE_WATER;

                } else if (isFence(b) || isFenceGate(b)) {
                    return TYPE_FENCE;
                }
                if (isSolidPhys(b)) return TYPE_BLOCKED;
                if (isLavaId(b)) return TYPE_LAVA;
            }
    return TYPE_OPEN;
}

void PathFinder::reconstruct(Path& path, Node* , Node* to) {
    int count = 1;
    Node* n = to;
    while (n->cameFrom) { count++; n = n->cameFrom; }

    static Node* list[MAX_NODES];
    int size = count;
    n = to;
    list[--count] = n;
    while (n->cameFrom) { n = n->cameFrom; list[--count] = n; }
    path.copyNodes(list, size);
}

Node* PathFinder::newNode(int x, int y, int z) {

    if (_nodeIndex >= MAX_NODES) return 0;
    Node& nref = _nodes[_nodeIndex++];
    nref = Node(x, y, z);
    return &nref;
}
