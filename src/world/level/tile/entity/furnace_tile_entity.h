
#ifndef MCPSP_WORLD_LEVEL_TILE_ENTITY_FURNACE_TILE_ENTITY_H
#define MCPSP_WORLD_LEVEL_TILE_ENTITY_FURNACE_TILE_ENTITY_H

#include "world/level/tile/entity/tile_entity.h"
#include "world/item/item_instance.h"

class CompoundTag;

class FurnaceTileEntity : public TileEntity {
    typedef TileEntity super;
public:
    static const int BURN_INTERVAL = 200;
    enum { SLOT_INGREDIENT = 0, SLOT_FUEL = 1, SLOT_RESULT = 2, NUM_ITEMS = 3 };

    FurnaceTileEntity();

    virtual void tick();
    virtual bool save(CompoundTag* tag);
    virtual void load(CompoundTag* tag);

    bool isLit() const { return litTime > 0; }

    int getBurnProgress(int max) const { return tickCount * max / BURN_INTERVAL; }
    int getLitProgress(int max) const {
        int d = litDuration > 0 ? litDuration : BURN_INTERVAL;
        return litTime * max / d;
    }

    static int getBurnDuration(const ItemInstance& fuel);

    static ItemInstance furnaceResult(short ingredientId);

    bool canBurn() const;
    void burn();

    ItemInstance items[NUM_ITEMS];
    int litTime;
    int litDuration;
    int tickCount;
};

void furnaceSetLitBlock(Level* level, int x, int y, int z, bool lit);

#endif
