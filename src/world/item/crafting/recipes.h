
#ifndef MCPSP_WORLD_ITEM_CRAFTING_RECIPES_H
#define MCPSP_WORLD_ITEM_CRAFTING_RECIPES_H

#include "world/item/crafting/recipe.h"
#include <initializer_list>

class Recipes {
public:
    static Recipes* getInstance();
    const std::vector<Recipe*>& getRecipes() const { return recipes; }

    struct Type { char c; ItemInstance ing; };

    void addShapedRecipe(const ItemInstance& result, const char* r0,
                         std::initializer_list<Type> types);
    void addShapedRecipe(const ItemInstance& result, const char* r0, const char* r1,
                         std::initializer_list<Type> types);
    void addShapedRecipe(const ItemInstance& result, const char* r0, const char* r1, const char* r2,
                         std::initializer_list<Type> types);
    void addShapelessRecipe(const ItemInstance& result,
                            std::initializer_list<ItemInstance> ingredients);

private:
    Recipes();
    ~Recipes();
    void addShapedRows(const ItemInstance& result, const char* const* rows, int height,
                       std::initializer_list<Type> types);

    static Recipes* instance;
    std::vector<Recipe*> recipes;
};

inline Recipes::Type TILE(char c, short tileId) { return { c, ItemInstance(tileId, 1, Recipe::ANY_AUX_VALUE) }; }
inline Recipes::Type ITEM(char c, short itemId) { return { c, ItemInstance(itemId, 1, 0) }; }
inline Recipes::Type INST(char c, short id, short aux) { return { c, ItemInstance(id, 1, aux) }; }

#endif
