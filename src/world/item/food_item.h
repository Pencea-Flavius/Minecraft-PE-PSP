#ifndef MCPSP_WORLD_ITEM_FOOD_ITEM_H
#define MCPSP_WORLD_ITEM_FOOD_ITEM_H

#include "world/item/item.h"

class FoodItem : public Item {
public:

    static const int EAT_TICKS = 32;

    FoodItem(short id, int nutrition, bool isMeat, int icon)
        : Item(id), nutrition(nutrition), icon(icon), meat(isMeat) {}
    virtual bool isFood() const { return true; }
    virtual int  getIcon(short data) const { return icon; }

    virtual void use(ItemInstance* item, Player* player, World* world);
    virtual int  getMaxUseDuration() const { return EAT_TICKS; }
    virtual void useTimeDepleted(ItemInstance* item, Player* player);
    virtual int  getUseAnimation() const { return 1; }
    int  getNutrition() const { return nutrition; }
    bool isMeat() const { return meat; }
protected:
    int  nutrition;
    int  icon;
    bool meat;
};

class BowlFoodItem : public FoodItem {
public:
    BowlFoodItem(short id, int nutrition, int icon) : FoodItem(id, nutrition, false, icon) {
        maxStackSize = 1;
    }
    virtual void useTimeDepleted(ItemInstance* item, Player* player);
};

class SeedFoodItem : public FoodItem {
public:
    short plantTile;

    SeedFoodItem(short id, int nutrition, short plantTile, int icon)
        : FoodItem(id, nutrition, false, icon), plantTile(plantTile) {}
    virtual bool useOn(ItemInstance* item, Player* player, World* world, int x, int y, int z, int face, float, float, float);

    virtual bool placesTile() const { return true; }
    virtual short plantedTileId() const { return plantTile; }
};

#endif
