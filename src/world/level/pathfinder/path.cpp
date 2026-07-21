#include "world/level/pathfinder/path.h"
#include "world/entity/entity.h"

int Path::p = 0;

Path::Path() : length(0), index(0) { ++p; }
Path::~Path() { destroy(); }

bool Path::isEmpty() const { return length == 0; }

void Path::copyNodes(Node** src, int len) {
    if (len > MAX_PATH) len = MAX_PATH;
    length = len;
    index = 0;
    for (int i = 0; i < len; ++i) nodes[i] = *src[i];
}

void Path::destroy() { index = length = 0; }

Node* Path::currentPos() { return length ? (Node*)&nodes[index] : 0; }
Vec3  Path::currentPos(Entity* e) const { return getPos(e, index); }
void  Path::next() { index++; }
int   Path::getSize() const { return length; }
bool  Path::isDone() const { return index >= length; }
Node* Path::last() const { return length > 0 ? (Node*)&nodes[length - 1] : 0; }
Node* Path::get(int i) const { return (Node*)&nodes[i]; }
int   Path::getIndex() const { return index; }
void  Path::setIndex(int i) { index = i; }

Vec3 Path::getPos(Entity* e, int i) const {
    float x = nodes[i].x + (int)(e->bbWidth + 1) * 0.5f;
    float z = nodes[i].z + (int)(e->bbWidth + 1) * 0.5f;
    float y = nodes[i].y;
    return Vec3(x, y, z);
}
