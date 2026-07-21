#ifndef MCPSP_NBT_LONG_TAG_H
#define MCPSP_NBT_LONG_TAG_H
#include "nbt/tag.h"

class LongTag : public Tag {
public:
    long long data;
    LongTag(const std::string& n) : Tag(n), data(0) {}
    LongTag(const std::string& n, long long d) : Tag(n), data(d) {}
    void write(IDataOutput* o) { o->writeLongLong(data); }
    void load(IDataInput* i)   { data = i->readLongLong(); }
    char getId() const { return TAG_Long; }
};
#endif
