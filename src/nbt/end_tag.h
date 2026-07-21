#ifndef MCPSP_NBT_END_TAG_H
#define MCPSP_NBT_END_TAG_H
#include "nbt/tag.h"
class EndTag : public Tag {
public:
    EndTag() : Tag("") {}
    void write(IDataOutput*) {}
    void load(IDataInput*)   {}
    char getId() const { return TAG_End; }
};
#endif
