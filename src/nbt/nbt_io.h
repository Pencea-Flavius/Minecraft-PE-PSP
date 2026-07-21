
#ifndef MCPSP_NBT_NBT_IO_H
#define MCPSP_NBT_NBT_IO_H

#include "nbt/compound_tag.h"
#include "util/data_io.h"

class NbtIo {
public:
    static CompoundTag* read(IDataInput* dis) {
        Tag* tag = Tag::readNamedTag(dis);
        if (tag && tag->getId() == Tag::TAG_Compound) return (CompoundTag*)tag;
        delete tag;
        return NULL;
    }
    static void write(CompoundTag* tag, IDataOutput* dos) {
        if (tag) Tag::writeNamedTag(tag, dos);
    }
};

#endif
