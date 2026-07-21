#ifndef MCPSP_NBT_STRING_TAG_H
#define MCPSP_NBT_STRING_TAG_H
#include "nbt/tag.h"
class StringTag : public Tag {
public:
    std::string data;
    StringTag(const std::string& n) : Tag(n) {}
    StringTag(const std::string& n, const std::string& d) : Tag(n), data(d) {}
    void write(IDataOutput* o) { o->writeString(data); }
    void load(IDataInput* i)   { data = i->readString(); }
    char getId() const { return TAG_String; }
};
#endif
