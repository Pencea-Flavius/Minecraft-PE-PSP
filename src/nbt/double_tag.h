#ifndef MCPSP_NBT_DOUBLE_TAG_H
#define MCPSP_NBT_DOUBLE_TAG_H
#include "nbt/tag.h"
class DoubleTag : public Tag {
public:
    double data;
    DoubleTag(const std::string& n) : Tag(n), data(0) {}
    DoubleTag(const std::string& n, double d) : Tag(n), data(d) {}
    void write(IDataOutput* o) { o->writeDouble(data); }
    void load(IDataInput* i)   { data = i->readDouble(); }
    char getId() const { return TAG_Double; }
};
#endif
