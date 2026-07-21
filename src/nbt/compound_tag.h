
#ifndef MCPSP_NBT_COMPOUND_TAG_H
#define MCPSP_NBT_COMPOUND_TAG_H

#include "nbt/tag.h"
#include "nbt/byte_tag.h"
#include "nbt/short_tag.h"
#include "nbt/int_tag.h"
#include "nbt/long_tag.h"
#include "nbt/float_tag.h"
#include "nbt/double_tag.h"
#include "nbt/string_tag.h"
#include "nbt/list_tag.h"
#include <map>
#include <vector>

class CompoundTag : public Tag {
    typedef std::map<std::string, Tag*> TagMap;
public:
    CompoundTag() : Tag("") {}
    CompoundTag(const std::string& n) : Tag(n) {}

    void write(IDataOutput* dos) {
        for (TagMap::iterator it = tags.begin(); it != tags.end(); ++it)
            Tag::writeNamedTag(it->second, dos);
        dos->writeByte(TAG_End);
    }

    void load(IDataInput* dis) {
        deleteChildren();
        Tag* tag = NULL;
        while ((tag = Tag::readNamedTag(dis)) && tag->getId() != TAG_End)
            tags[tag->getName()] = tag;
        delete tag;
    }

    char getId() const { return TAG_Compound; }

    void put(const std::string& n, Tag* tag)          { tags[n] = tag->setName(n); }
    void putByte(const std::string& n, char v)        { tags[n] = new ByteTag(n, v); }
    void putShort(const std::string& n, short v)      { tags[n] = new ShortTag(n, v); }
    void putInt(const std::string& n, int v)          { tags[n] = new IntTag(n, v); }
    void putLong(const std::string& n, long long v)   { tags[n] = new LongTag(n, v); }
    void putFloat(const std::string& n, float v)      { tags[n] = new FloatTag(n, v); }
    void putDouble(const std::string& n, double v)    { tags[n] = new DoubleTag(n, v); }
    void putString(const std::string& n, const std::string& v) { tags[n] = new StringTag(n, v); }
    void putBoolean(const std::string& n, bool v)     { putByte(n, v ? 1 : 0); }
    void putCompound(const std::string& n, CompoundTag* v) { tags[n] = v->setName(n); }

    bool contains(const std::string& n) const { return tags.find(n) != tags.end(); }
    bool contains(const std::string& n, int type) const {
        Tag* t = get(n); return t && t->getId() == type;
    }
    Tag* get(const std::string& n) const {
        TagMap::const_iterator it = tags.find(n);
        return it == tags.end() ? NULL : it->second;
    }

    char  getByte(const std::string& n) const  { return contains(n, TAG_Byte)  ? ((ByteTag*) get(n))->data  : 0; }
    short getShort(const std::string& n) const { return contains(n, TAG_Short) ? ((ShortTag*)get(n))->data  : 0; }
    int   getInt(const std::string& n) const   { return contains(n, TAG_Int)   ? ((IntTag*)  get(n))->data  : 0; }
    long long getLong(const std::string& n) const { return contains(n, TAG_Long) ? ((LongTag*)get(n))->data : 0; }
    float getFloat(const std::string& n) const { return contains(n, TAG_Float) ? ((FloatTag*)get(n))->data  : 0.0f; }
    bool  getBoolean(const std::string& n) const { return getByte(n) != 0; }
    std::string getString(const std::string& n) const {
        return contains(n, TAG_String) ? ((StringTag*)get(n))->data : std::string();
    }
    CompoundTag* getCompound(const std::string& n) const {
        return contains(n, TAG_Compound) ? (CompoundTag*)get(n) : NULL;
    }

    ListTag* getList(const std::string& n) {
        if (!contains(n, TAG_List)) { ListTag* l = new ListTag(n); tags[n] = l; return l; }
        return (ListTag*)get(n);
    }

    void getAllTags(std::vector<Tag*>& out) const {
        for (TagMap::const_iterator it = tags.begin(); it != tags.end(); ++it)
            out.push_back(it->second);
    }
    bool isEmpty() const { return tags.empty(); }

    void deleteChildren() {
        for (TagMap::iterator it = tags.begin(); it != tags.end(); ++it) {
            if (it->second) { it->second->deleteChildren(); delete it->second; }
        }
        tags.clear();
    }

private:
    TagMap tags;
};

static inline ListTag* floatList(float a, float b) {
    ListTag* l = new ListTag();
    l->add(new FloatTag("", a));
    l->add(new FloatTag("", b));
    return l;
}
static inline ListTag* floatList(float a, float b, float c) {
    ListTag* l = new ListTag();
    l->add(new FloatTag("", a));
    l->add(new FloatTag("", b));
    l->add(new FloatTag("", c));
    return l;
}

#endif
