#include "world/level/tile/entity/sign_tile_entity.h"
#include "nbt/compound_tag.h"

SignTileEntity::SignTileEntity()
    : super(TE_SIGN), selectedLine(-1), editable(true) {
    rendererId = TR_SIGN_RENDERER;
}

bool SignTileEntity::save(CompoundTag* tag) {
    if (!super::save(tag)) return false;
    tag->putString("Text1", messages[0]);
    tag->putString("Text2", messages[1]);
    tag->putString("Text3", messages[2]);
    tag->putString("Text4", messages[3]);
    return true;
}

void SignTileEntity::load(CompoundTag* tag) {
    editable = false;
    super::load(tag);
    messages[0] = tag->getString("Text1");
    messages[1] = tag->getString("Text2");
    messages[2] = tag->getString("Text3");
    messages[3] = tag->getString("Text4");
    for (int i = 0; i < NUM_LINES; i++)
        if ((int)messages[i].length() > MAX_LINE_LENGTH)
            messages[i].resize(MAX_LINE_LENGTH);
}
