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
    int  getUseDuration() const { return EAT_TICKS; }
    int  getNutrition() const { return nutrition; }

    virtual short getFoodRemainder() const { return 0; }
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
    virtual short getFoodRemainder() const { return ITEM_BOWL; }
};

#endif
