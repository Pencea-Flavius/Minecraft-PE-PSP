#ifndef MCPSP_NBT_SHORT_TAG_H
#define MCPSP_NBT_SHORT_TAG_H
#include "nbt/tag.h"
class ShortTag : public Tag {
public:
    short data;
    ShortTag(const std::string& n) : Tag(n), data(0) {}
    ShortTag(const std::string& n, short d) : Tag(n), data(d) {}
    void write(IDataOutput* o) { o->writeShort(data); }
    void load(IDataInput* i)   { data = i->readShort(); }
    char getId() const { return TAG_Short; }
};
#endif
