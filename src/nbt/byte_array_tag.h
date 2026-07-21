#ifndef MCPSP_NBT_BYTE_ARRAY_TAG_H
#define MCPSP_NBT_BYTE_ARRAY_TAG_H
#include "nbt/tag.h"
#include <cstring>

struct TagMemoryChunk {
    void* data;
    int len;
    TagMemoryChunk() : data(0), len(0) {}
};
class ByteArrayTag : public Tag {
public:
    TagMemoryChunk data;
    ByteArrayTag(const std::string& n) : Tag(n) {}
    ByteArrayTag(const std::string& n, TagMemoryChunk d) : Tag(n), data(d) {}
    char getId() const { return TAG_Byte_Array; }
    void write(IDataOutput* o) { o->writeInt(data.len); o->writeBytes(data.data, data.len); }
    void load(IDataInput* i) {
        int length = i->readInt();
        if (length < 0) length = 0;
        data.data = new char[length];
        data.len = length;
        i->readBytes(data.data, length);
    }
};
#endif
