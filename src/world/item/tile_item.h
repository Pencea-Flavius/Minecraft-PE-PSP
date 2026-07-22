#ifndef MCPSP_WORLD_ITEM_TILE_ITEM_H
#define MCPSP_WORLD_ITEM_TILE_ITEM_H

#include "world/item/item.h"

bool placeTileResolved(World* w, int nx, int ny, int nz, int tileId, int data, Player* placer);

class TileItem : public Item {
public:

    short tileId;
    TileItem(short id) : Item(id), tileId(id), icon(-1) {
        maxStackSize = 64;
        maxDamage = 0;
    }

    int icon;
    TileItem(short id, short tile, int icon_ = -1) : Item(id), tileId(tile), icon(icon_) {
        maxStackSize = 64;
        maxDamage = 0;
    }
    virtual int getIcon(short) const { return icon; }
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face,
                       float clickX, float clickY, float clickZ);
};

class SlabItem : public TileItem {
public:
    SlabItem(short id) : TileItem(id) {}
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face,
                       float clickX, float clickY, float clickZ);
};

class DoorItem : public Item {
public:
    short tileId;
    int   icon;
    DoorItem(short id, short tile, int icon)
        : Item(id), tileId(tile), icon(icon) { maxStackSize = 1; maxDamage = 0; }
    virtual int getIcon(short) const { return icon; }
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face,
                       float clickX, float clickY, float clickZ);
};

class BedItem : public Item {
public:
    short tileId;
    int   icon;
    BedItem(short id, short tile, int icon)
        : Item(id), tileId(tile), icon(icon) { maxStackSize = 1; maxDamage = 0; }
    virtual int getIcon(short) const { return icon; }
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face,
                       float clickX, float clickY, float clickZ);
};

class FlintAndSteelItem : public Item {
public:
    int icon;
    FlintAndSteelItem(short id, int icon) : Item(id), icon(icon) { maxStackSize = 1; maxDamage = 64; }
    virtual int  getIcon(short) const { return icon; }
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face,
                       float clickX, float clickY, float clickZ);
};

#endif
