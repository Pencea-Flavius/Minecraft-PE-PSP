
#ifndef MCPSP_WORLD_LEVEL_TILE_ENTITY_FURNACE_TILE_ENTITY_H
#define MCPSP_WORLD_LEVEL_TILE_ENTITY_FURNACE_TILE_ENTITY_H

#include "world/level/tile/entity/tile_entity.h"
#include "world/item/item_instance.h"
#include "world/Container.h"

class CompoundTag;

class FurnaceTileEntity : public TileEntity, public Container {
    typedef TileEntity super;
public:
    static const int BURN_INTERVAL = 200;
    enum { SLOT_INGREDIENT = 0, SLOT_FUEL = 1, SLOT_RESULT = 2, NUM_ITEMS = 3 };

    FurnaceTileEntity();

    virtual void tick();
    virtual bool save(CompoundTag* tag);
    virtual bool shouldSave();
    virtual void load(CompoundTag* tag);

    bool keepOnRemove;
    virtual void setRemoved() { if (!keepOnRemove) TileEntity::setRemoved(); }

    virtual ItemInstance* getItem(int slot);
    virtual void          setItem(int slot, ItemInstance* item);
    virtual ItemInstance  removeItem(int slot, int count);
    virtual int getContainerSize() const { return NUM_ITEMS; }
    virtual int getMaxStackSize() const  { return Container::LARGE_MAX_STACK_SIZE; }

    bool isLit() const { return litTime > 0; }

    int getBurnProgress(int max) const { return tickCount * max / BURN_INTERVAL; }
    int getLitProgress(int max) {

        if (!litDuration) litDuration = BURN_INTERVAL;
        int r = litTime * max / litDuration;
        return r >= max ? max : r;
    }

    static int getBurnDuration(const ItemInstance& fuel);

    static ItemInstance furnaceResult(short ingredientId);

    static bool isFuel(const ItemInstance& item);
    static bool isFurnaceItem(const ItemInstance& item);
    bool isSlotEmpty(int slot) const;

    bool canBurn() const;
    void burn();

    ItemInstance items[NUM_ITEMS];
    int litTime;
    int litDuration;
    int tickCount;
};

extern bool g_furnaceNoDrop;

void furnaceSetLitBlock(Level* level, int x, int y, int z, bool lit);

#endif
