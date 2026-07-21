#ifndef MCPSP_WORLD_ITEM_DIGGER_ITEM_H
#define MCPSP_WORLD_ITEM_DIGGER_ITEM_H

#include "world/item/item.h"
#include "world/level/tile/material.h"

class DiggerItem : public Item {
public:
    virtual float getDestroySpeed(int blockId) const {
        if (blockId >= 0 && blockId < 256 && mineable[blockId]) return speed;
        return 1.0f;
    }
    virtual int  getAttackDamage() const { return attackDamage; }
    virtual bool isHandEquipped() const { return true; }
    virtual int  getIcon(short data) const { return icon; }

protected:
    DiggerItem(short id, int baseAttack, const Tier& tier, int icon)
        : Item(id), speed(tier.getSpeed()), icon(icon),
          attackDamage(baseAttack + tier.getAttackDamageBonus()), level(tier.getLevel()) {
        maxStackSize = 1;
        maxDamage    = (short)tier.getUses();
        for (int i = 0; i < 256; i++) mineable[i] = false;
    }
    void addTile(int b) { if (b >= 0 && b < 256) mineable[b] = true; }

    float speed;
    int   icon;
    int   attackDamage;
    int   level;
    bool  mineable[256];
};

class ShovelItem : public DiggerItem {
public:
    ShovelItem(short id, const Tier& tier, int icon) : DiggerItem(id, 1, tier, icon) {
        addTile(BLOCK_GRASS); addTile(BLOCK_DIRT); addTile(BLOCK_SAND); addTile(BLOCK_GRAVEL);
        addTile(BLOCK_TOPSNOW); addTile(BLOCK_SNOW_BLOCK); addTile(BLOCK_CLAY); addTile(BLOCK_FARMLAND);
    }
    virtual bool canDestroySpecial(int b) const {
        return b == BLOCK_TOPSNOW || b == BLOCK_SNOW_BLOCK;
    }
};

class PickaxeItem : public DiggerItem {
public:

    virtual float getDestroySpeed(int blockId) const {
        if (blockId >= 0 && blockId < 256) {
            TileMaterial m = tileMaterial((unsigned char)blockId);
            if (m == MAT_STONE || m == MAT_METAL) return speed;
        }
        return DiggerItem::getDestroySpeed(blockId);
    }
public:
    PickaxeItem(short id, const Tier& tier, int icon) : DiggerItem(id, 2, tier, icon) {
        addTile(BLOCK_STONE); addTile(BLOCK_COBBLESTONE);
        addTile(BLOCK_STONE_BRICKS); addTile(BLOCK_MOSSY_COBBLE);

        addTile(BLOCK_DOUBLE_SLAB); addTile(BLOCK_SLAB);
        addTile(BLOCK_SANDSTONE);
        addTile(BLOCK_ORE_IRON); addTile(BLOCK_IRON_BLOCK);
        addTile(BLOCK_ORE_COAL);
        addTile(BLOCK_GOLD_BLOCK); addTile(BLOCK_ORE_GOLD);
        addTile(BLOCK_ORE_EMERALD); addTile(BLOCK_DIAMOND_BLOCK);
        addTile(BLOCK_ICE);
        addTile(BLOCK_ORE_LAPIS); addTile(BLOCK_LAPIS_BLOCK);
        addTile(BLOCK_ORE_REDSTONE); addTile(BLOCK_ORE_REDSTONE_LIT);

        addTile(BLOCK_BRICKS); addTile(BLOCK_QUARTZ_BLOCK);
        addTile(BLOCK_NETHERRACK); addTile(BLOCK_NETHER_BRICK);
        addTile(BLOCK_FURNACE); addTile(BLOCK_FURNACE_LIT); addTile(BLOCK_STONECUTTER);
        addTile(BLOCK_STAIRS_COBBLESTONE); addTile(BLOCK_STAIRS_BRICK);
        addTile(BLOCK_STAIRS_STONE_BRICK); addTile(BLOCK_STAIRS_SANDSTONE);
        addTile(BLOCK_STAIRS_NETHER_BRICK);
    }
    virtual bool canDestroySpecial(int b) const {
        if (b == BLOCK_OBSIDIAN) return level == 3;
        if (b == BLOCK_DIAMOND_BLOCK || b == BLOCK_ORE_EMERALD) return level >= 2;
        if (b == BLOCK_GOLD_BLOCK || b == BLOCK_ORE_GOLD)       return level >= 2;
        if (b == BLOCK_IRON_BLOCK || b == BLOCK_ORE_IRON)       return level >= 1;
        if (b == BLOCK_LAPIS_BLOCK || b == BLOCK_ORE_LAPIS)     return level >= 1;
        if (b == BLOCK_ORE_REDSTONE || b == BLOCK_ORE_REDSTONE_LIT) return level >= 2;
        return mineable[b >= 0 && b < 256 ? b : 0];
    }
};

class HatchetItem : public DiggerItem {
public:

    virtual float getDestroySpeed(int blockId) const {
        if (blockId >= 0 && blockId < 256 &&
            tileMaterial((unsigned char)blockId) == MAT_WOOD) return speed;
        return DiggerItem::getDestroySpeed(blockId);
    }
public:
    HatchetItem(short id, const Tier& tier, int icon) : DiggerItem(id, 3, tier, icon) {
        addTile(BLOCK_PLANKS); addTile(BLOCK_BOOKSHELF);
        addTile(BLOCK_LOG);
        addTile(BLOCK_CHEST);
        addTile(BLOCK_DOUBLE_SLAB); addTile(BLOCK_SLAB);
    }
};

#endif
