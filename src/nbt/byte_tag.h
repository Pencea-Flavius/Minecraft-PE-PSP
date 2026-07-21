#ifndef MCPSP_NBT_BYTE_TAG_H
#define MCPSP_NBT_BYTE_TAG_H
#include "nbt/tag.h"
class ByteTag : public Tag {
public:
    char data;
    ByteTag(const std::string& n) : Tag(n), data(0) {}
    ByteTag(const std::string& n, char d) : Tag(n), data(d) {}
    void write(IDataOutput* o) { o->writeByte(data); }
    void load(IDataInput* i)   { data = i->readByte(); }
    char getId() const { return TAG_Byte; }
};
#endif
