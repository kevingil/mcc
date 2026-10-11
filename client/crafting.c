#include "crafting.h"

#include <stddef.h>

#define P INGREDIENT_PLANKS

static const CraftingRecipe recipes[] = {
    { BLOCK_OAK_PLANKS, 4, 0, 0, { BLOCK_OAK_LOG }, 1, RECIPE_CATEGORY_BUILDING, "planks" },
    { BLOCK_BIRCH_PLANKS, 4, 0, 0, { BLOCK_BIRCH_LOG }, 1, RECIPE_CATEGORY_BUILDING, "planks" },
    { BLOCK_ACACIA_PLANKS, 4, 0, 0, { BLOCK_ACACIA_LOG }, 1, RECIPE_CATEGORY_BUILDING, "planks" },
    { BLOCK_DARK_OAK_PLANKS, 4, 0, 0, { BLOCK_DARK_OAK_LOG }, 1, RECIPE_CATEGORY_BUILDING, "planks" },
    { BLOCK_STONE_BRICKS, 4, 2, 2, { BLOCK_STONE, BLOCK_STONE, BLOCK_STONE, BLOCK_STONE }, 4, RECIPE_CATEGORY_BUILDING, NULL },
    { BLOCK_SANDSTONE, 1, 2, 2, { BLOCK_SAND, BLOCK_SAND, BLOCK_SAND, BLOCK_SAND }, 4, RECIPE_CATEGORY_BUILDING, NULL },
    { BLOCK_RED_SANDSTONE, 1, 2, 2, { BLOCK_RED_SAND, BLOCK_RED_SAND, BLOCK_RED_SAND, BLOCK_RED_SAND }, 4, RECIPE_CATEGORY_BUILDING, NULL },
    { BLOCK_CUT_SANDSTONE, 4, 2, 2, { BLOCK_SANDSTONE, BLOCK_SANDSTONE, BLOCK_SANDSTONE, BLOCK_SANDSTONE }, 4, RECIPE_CATEGORY_BUILDING, NULL },
    { BLOCK_QUARTZ_PILLAR, 2, 1, 2, { BLOCK_QUARTZ_BLOCK, BLOCK_QUARTZ_BLOCK }, 2, RECIPE_CATEGORY_BUILDING, NULL },
    { BLOCK_ANDESITE, 2, 0, 0, { BLOCK_DIORITE, BLOCK_COBBLESTONE }, 2, RECIPE_CATEGORY_BUILDING, NULL },
    { BLOCK_PACKED_ICE, 1, 3, 3, { BLOCK_ICE, BLOCK_ICE, BLOCK_ICE, BLOCK_ICE, BLOCK_ICE, BLOCK_ICE, BLOCK_ICE, BLOCK_ICE, BLOCK_ICE }, 9, RECIPE_CATEGORY_BUILDING, NULL },
    { BLOCK_BLUE_ICE, 1, 3, 3, { BLOCK_PACKED_ICE, BLOCK_PACKED_ICE, BLOCK_PACKED_ICE, BLOCK_PACKED_ICE, BLOCK_PACKED_ICE,
        BLOCK_PACKED_ICE, BLOCK_PACKED_ICE, BLOCK_PACKED_ICE, BLOCK_PACKED_ICE }, 9, RECIPE_CATEGORY_BUILDING, NULL },
    { BLOCK_CRAFTING_TABLE, 1, 2, 2, { P, P, P, P }, 4, RECIPE_CATEGORY_MISC, NULL },
    { BLOCK_CHEST, 1, 3, 3, { P, P, P, P, INGREDIENT_EMPTY, P, P, P, P }, 8, RECIPE_CATEGORY_MISC, NULL },
    { BLOCK_FURNACE, 1, 3, 3, { BLOCK_COBBLESTONE, BLOCK_COBBLESTONE, BLOCK_COBBLESTONE, BLOCK_COBBLESTONE, INGREDIENT_EMPTY,
        BLOCK_COBBLESTONE, BLOCK_COBBLESTONE, BLOCK_COBBLESTONE, BLOCK_COBBLESTONE }, 8, RECIPE_CATEGORY_MISC, NULL }
};

#undef P

int CraftingRecipeCount(void)
{
    return (int)(sizeof(recipes)/sizeof(recipes[0]));
}

const CraftingRecipe *CraftingGetRecipe(int index)
{
    if ((index < 0) || (index >= CraftingRecipeCount())) return NULL;
    return &recipes[index];
}

bool CraftingIngredientAccepts(int ingredient, BlockType block)
{
    if (ingredient == INGREDIENT_EMPTY) return block == BLOCK_AIR;
    if (block == BLOCK_AIR) return false;
    if (ingredient == INGREDIENT_PLANKS)
    {
        return (block == BLOCK_OAK_PLANKS) || (block == BLOCK_BIRCH_PLANKS) ||
            (block == BLOCK_ACACIA_PLANKS) || (block == BLOCK_DARK_OAK_PLANKS);
    }
    return (int)block == ingredient;
}

static BlockType CellBlock(const BlockType *cells, const int *counts, int index)
{
    return (counts[index] > 0)? cells[index] : BLOCK_AIR;
}

// Every cell of the grid is checked, so cells outside the placed pattern
// have to be empty.
static bool MatchShapedAt(const CraftingRecipe *recipe, const BlockType *cells, const int *counts, int size,
    int offsetX, int offsetY, bool mirrored)
{
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int px = x - offsetX;
            int py = y - offsetY;
            int ingredient = INGREDIENT_EMPTY;

            if ((px >= 0) && (py >= 0) && (px < recipe->width) && (py < recipe->height))
            {
                int column = mirrored? recipe->width - px - 1 : px;

                ingredient = recipe->ingredients[py*recipe->width + column];
            }
            if (!CraftingIngredientAccepts(ingredient, CellBlock(cells, counts, y*size + x))) return false;
        }
    }
    return true;
}

static bool MatchShaped(const CraftingRecipe *recipe, const BlockType *cells, const int *counts, int size)
{
    if ((recipe->width > size) || (recipe->height > size)) return false;
    for (int y = 0; y <= size - recipe->height; y++)
    {
        for (int x = 0; x <= size - recipe->width; x++)
        {
            if (MatchShapedAt(recipe, cells, counts, size, x, y, true)) return true;
            if (MatchShapedAt(recipe, cells, counts, size, x, y, false)) return true;
        }
    }
    return false;
}

// Each ingredient in these recipes accepts a different set of blocks, so the
// first free ingredient that takes a cell is the only one that could.
static bool MatchShapeless(const CraftingRecipe *recipe, const BlockType *cells, const int *counts, int size)
{
    bool used[9] = { false };
    int filled = 0;

    for (int i = 0; i < size*size; i++)
    {
        BlockType block = CellBlock(cells, counts, i);
        int found = -1;

        if (block == BLOCK_AIR) continue;
        filled++;
        for (int k = 0; (k < recipe->ingredientCount) && (found < 0); k++)
        {
            if (!used[k] && CraftingIngredientAccepts(recipe->ingredients[k], block)) found = k;
        }
        if (found < 0) return false;
        used[found] = true;
    }
    return filled == recipe->ingredientCount;
}

int CraftingFindRecipe(const BlockType *cells, const int *counts, int size)
{
    for (int i = 0; i < CraftingRecipeCount(); i++)
    {
        const CraftingRecipe *recipe = &recipes[i];
        bool match = (recipe->width == 0)? MatchShapeless(recipe, cells, counts, size) : MatchShaped(recipe, cells, counts, size);

        if (match) return i;
    }
    return -1;
}

void CraftingConsume(BlockType *cells, int *counts, int size)
{
    for (int i = 0; i < size*size; i++)
    {
        if (counts[i] <= 0) continue;
        counts[i]--;
        if (counts[i] <= 0)
        {
            counts[i] = 0;
            cells[i] = BLOCK_AIR;
        }
    }
}
