
#ifndef MCPSP_NBT_LIST_TAG_H
#define MCPSP_NBT_LIST_TAG_H

#include "nbt/tag.h"
#include "nbt/float_tag.h"
#include <vector>

class ListTag : public Tag {
public:
    ListTag() : Tag(""), type(TAG_Byte) {}
    ListTag(const std::string& n) : Tag(n), type(TAG_Byte) {}

    void write(IDataOutput* dos) {
        type = list.empty() ? TAG_Byte : list.front()->getId();
        dos->writeByte(type);
        dos->writeInt((int)list.size());
        for (size_t i = 0; i < list.size(); i++)
            list[i]->write(dos);
    }

    void load(IDataInput* dis) {
        type = dis->readByte();
        int size = dis->readInt();
        list.clear();
        if (size < 0) size = 0;
        for (int i = 0; i < size; i++) {
            Tag* tag = Tag::newTag(type, NullString);
            if (!tag) break;
            tag->load(dis);
            list.push_back(tag);
        }
    }

    char getId() const { return TAG_List; }

    void add(Tag* tag) { type = tag->getId(); list.push_back(tag); }

    Tag* get(int index) const {
        if (index < 0 || index >= (int)list.size()) {
            errorState |= TAGERR_OUT_OF_BOUNDS;
            return NULL;
        }
        return list[index];
    }

    float getFloat(int index) {
        Tag* tag = get(index);
        if (!tag) return 0.0f;
        if (tag->getId() != TAG_Float) { errorState |= TAGERR_BAD_TYPE; return 0.0f; }
        return ((FloatTag*)tag)->data;
    }

    int size() const { return (int)list.size(); }

    void deleteChildren() {
        for (size_t i = 0; i < list.size(); i++) {
            if (list[i]) { list[i]->deleteChildren(); delete list[i]; }
        }
        list.clear();
    }

private:
    std::vector<Tag*> list;
    char type;
};

#endif
