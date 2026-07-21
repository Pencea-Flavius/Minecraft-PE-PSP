#ifndef MCPSP_NBT_FLOAT_TAG_H
#define MCPSP_NBT_FLOAT_TAG_H
#include "nbt/tag.h"
class FloatTag : public Tag {
public:
    float data;
    FloatTag(const std::string& n) : Tag(n), data(0) {}
    FloatTag(const std::string& n, float d) : Tag(n), data(d) {}
    void write(IDataOutput* o) { o->writeFloat(data); }
    void load(IDataInput* i)   { data = i->readFloat(); }
    char getId() const { return TAG_Float; }
};
#endif
