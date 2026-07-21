
#ifndef MCPSP_WORLD_ITEM_CRAFTING_RECIPE_H
#define MCPSP_WORLD_ITEM_CRAFTING_RECIPE_H

#include "world/item/item_instance.h"
#include <map>
#include <vector>

class ItemPack {
public:
    typedef std::map<int, int> Map;

    void add(int id, int count = 1);
    int  getCount(int id) const;

    int getMaxMultipliesOf(const ItemPack& v) const;
    std::vector<ItemInstance> getItemInstances() const;

    static int getIdForItemInstance(const ItemInstance* ii);
    static ItemInstance getItemInstanceForId(int id);

private:
    Map items;
};

class Recipe {
public:
    static const int SIZE_2X2 = 0;
    static const int SIZE_3X3 = 1;
    static const int ANY_AUX_VALUE = -1;

    virtual ~Recipe() {}
    virtual const ItemPack& getItemPack() { return myItems; }
    virtual int getMaxCraftCount(const ItemPack& fromItems) = 0;
    virtual ItemInstance getResultItem() const = 0;
    virtual int getCraftingSize() = 0;

    static bool isAnyAuxValue(const ItemInstance* ii) { return isAnyAuxValue(ii->id); }
    static bool isAnyAuxValue(int id);

protected:
    ItemPack myItems;
};

class ShapedRecipe : public Recipe {
public:

    ShapedRecipe(int width, int height, ItemInstance* recipeItems, const ItemInstance& result)
        : width(width), height(height), recipeItems(recipeItems), result(result) {
        for (int i = 0; i < width * height; ++i)
            if (!recipeItems[i].isNull())
                myItems.add(ItemPack::getIdForItemInstance(&recipeItems[i]));
    }
    ~ShapedRecipe() { delete[] recipeItems; }

    int getMaxCraftCount(const ItemPack& fromItems) { return fromItems.getMaxMultipliesOf(myItems); }
    ItemInstance getResultItem() const { return result; }
    int getCraftingSize() { return (width <= 2 && height <= 2) ? SIZE_2X2 : SIZE_3X3; }

private:
    int width, height;
    ItemInstance* recipeItems;
    ItemInstance result;
};

class ShapelessRecipe : public Recipe {
public:
    ShapelessRecipe(const ItemInstance& result, const std::vector<ItemInstance>& ingredients)
        : result(result), ingredients(ingredients) {
        for (unsigned int i = 0; i < ingredients.size(); ++i)
            if (!ingredients[i].isNull())
                myItems.add(ItemPack::getIdForItemInstance(&ingredients[i]));
    }

    int getMaxCraftCount(const ItemPack& fromItems) { return fromItems.getMaxMultipliesOf(myItems); }
    ItemInstance getResultItem() const { return result; }
    int getCraftingSize() { return (ingredients.size() > 4) ? SIZE_3X3 : SIZE_2X2; }

private:
    ItemInstance result;
    std::vector<ItemInstance> ingredients;
};

#endif
