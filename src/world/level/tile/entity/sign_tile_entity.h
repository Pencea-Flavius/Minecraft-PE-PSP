
#ifndef MCPSP_WORLD_LEVEL_TILE_ENTITY_SIGN_TILE_ENTITY_H
#define MCPSP_WORLD_LEVEL_TILE_ENTITY_SIGN_TILE_ENTITY_H

#include <string>
#include "world/level/tile/entity/tile_entity.h"

class CompoundTag;

class SignTileEntity : public TileEntity {
    typedef TileEntity super;
public:
    static const int MAX_LINE_LENGTH = 15;
    static const int NUM_LINES = 4;

    SignTileEntity();

    virtual bool save(CompoundTag* tag);
    virtual void load(CompoundTag* tag);

    bool isEditable() const { return editable; }
    void setEditable(bool e) { editable = e; }

    std::string messages[NUM_LINES];
    int selectedLine;
private:
    bool editable;
};

#endif
