#include "nbt/tag.h"

#include "nbt/end_tag.h"
#include "nbt/byte_tag.h"
#include "nbt/short_tag.h"
#include "nbt/int_tag.h"
#include "nbt/long_tag.h"
#include "nbt/float_tag.h"
#include "nbt/double_tag.h"
#include "nbt/byte_array_tag.h"
#include "nbt/string_tag.h"
#include "nbt/list_tag.h"
#include "nbt/compound_tag.h"

const std::string Tag::NullString = "";

Tag* Tag::readNamedTag(IDataInput* dis) {
    char type = dis->readByte();
    if (type == TAG_End) return new EndTag();

    Tag* tag = newTag(type, dis->readString());
    if (!tag) return NULL;

    tag->load(dis);
    return tag;
}

void Tag::writeNamedTag(Tag* tag, IDataOutput* dos) {
    dos->writeByte(tag->getId());
    if (tag->getId() == TAG_End) return;

    dos->writeString(tag->getName());
    tag->write(dos);
}

Tag* Tag::newTag(char type, const std::string& name) {
    switch (type) {
    case TAG_End:        return new EndTag();
    case TAG_Byte:       return new ByteTag(name);
    case TAG_Short:      return new ShortTag(name);
    case TAG_Int:        return new IntTag(name);
    case TAG_Long:       return new LongTag(name);
    case TAG_Float:      return new FloatTag(name);
    case TAG_Double:     return new DoubleTag(name);
    case TAG_Byte_Array: return new ByteArrayTag(name);
    case TAG_String:     return new StringTag(name);
    case TAG_List:       return new ListTag(name);
    case TAG_Compound:   return new CompoundTag(name);
    }
    return NULL;
}
