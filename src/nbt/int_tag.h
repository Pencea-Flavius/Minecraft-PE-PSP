#ifndef MCPSP_NBT_INT_TAG_H
#define MCPSP_NBT_INT_TAG_H
#include "nbt/tag.h"
class IntTag : public Tag {
public:
    int data;
    IntTag(const std::string& n) : Tag(n), data(0) {}
    IntTag(const std::string& n, int d) : Tag(n), data(d) {}
    void write(IDataOutput* o) { o->writeInt(data); }
    void load(IDataInput* i)   { data = i->readInt(); }
    char getId() const { return TAG_Int; }
};
#endif
