#ifndef CRAFTING_H
#define CRAFTING_H

#include "voxel_types.h"

// An ingredient is a block id, INGREDIENT_PLANKS for any of the planks, or
// INGREDIENT_EMPTY for a cell that has to stay empty.
#define INGREDIENT_EMPTY 0
#define INGREDIENT_PLANKS (-1)

typedef enum {
    RECIPE_CATEGORY_BUILDING = 0,
    RECIPE_CATEGORY_MISC
} RecipeCategory;

typedef struct {
    BlockType result;
    int count;
    int width;              // 0 marks a shapeless recipe
    int height;
    int ingredients[9];     // Row-major pattern, or the shapeless list
    int ingredientCount;
    RecipeCategory category;
    const char *group;      // Recipes sharing a group share one recipe book entry
} CraftingRecipe;

int CraftingRecipeCount(void);
const CraftingRecipe *CraftingGetRecipe(int index);
bool CraftingIngredientAccepts(int ingredient, BlockType block);

// cells and counts are row-major size x size grids, size 2 or 3. Returns the
// index of the matching recipe or -1.
int CraftingFindRecipe(const BlockType *cells, const int *counts, int size);
// Takes one item out of every occupied cell.
void CraftingConsume(BlockType *cells, int *counts, int size);

#endif // CRAFTING_H
