#include "player.h"
#include "menu_ui.h"
#include "screens.h"
#include "voxel_renderer.h"

#include <string.h>

typedef struct CraftRecipe {
    const char *name;
    BlockType item;
    int kind;
    BlockType out;
    int outCount;
    int width;
} CraftRecipe;

static const CraftRecipe recipes[] = {
    { "Oak Planks", BLOCK_OAK_LOG, 1, BLOCK_OAK_PLANKS, 4, 2 },
    { "Birch Planks", BLOCK_BIRCH_LOG, 1, BLOCK_BIRCH_PLANKS, 4, 2 },
    { "Acacia Planks", BLOCK_ACACIA_LOG, 1, BLOCK_ACACIA_PLANKS, 4, 2 },
    { "Dark Oak Planks", BLOCK_DARK_OAK_LOG, 1, BLOCK_DARK_OAK_PLANKS, 4, 2 },
    { "Crafting Table", BLOCK_OAK_PLANKS, 4, BLOCK_CRAFTING_TABLE, 1, 2 },
    { "Sandstone", BLOCK_SAND, 4, BLOCK_SANDSTONE, 1, 2 },
    { "Stone Bricks", BLOCK_COBBLESTONE, 4, BLOCK_STONE_BRICKS, 1, 2 },
    { "Furnace", BLOCK_COBBLESTONE, 8, BLOCK_FURNACE, 1, 3 },
    { "Chest", BLOCK_OAK_PLANKS, 8, BLOCK_CHEST, 1, 3 }
};

static Texture2D guiInventory = { 0 };
static Texture2D guiCrafting = { 0 };
static Texture2D guiWidgets = { 0 };
static int guiReady = 0;

static int RecipeCount(void)
{
    return (int)(sizeof(recipes)/sizeof(recipes[0]));
}

static void LoadGui(void)
{
    if (guiReady) return;
    guiReady = 1;
    guiInventory = LoadTexture("resources/textures/gui/container/inventory.png");
    guiCrafting = LoadTexture("resources/textures/gui/container/crafting_table.png");
    guiWidgets = LoadTexture("resources/textures/gui/widgets.png");
    if (guiInventory.id != 0) SetTextureFilter(guiInventory, TEXTURE_FILTER_POINT);
    if (guiCrafting.id != 0) SetTextureFilter(guiCrafting, TEXTURE_FILTER_POINT);
    if (guiWidgets.id != 0) SetTextureFilter(guiWidgets, TEXTURE_FILTER_POINT);
}

static int IsLog(BlockType block)
{
    return (block == BLOCK_OAK_LOG) || (block == BLOCK_BIRCH_LOG) ||
        (block == BLOCK_ACACIA_LOG) || (block == BLOCK_DARK_OAK_LOG);
}

static int IsPlanks(BlockType block)
{
    return (block == BLOCK_OAK_PLANKS) || (block == BLOCK_BIRCH_PLANKS) ||
        (block == BLOCK_ACACIA_PLANKS) || (block == BLOCK_DARK_OAK_PLANKS);
}

static BlockType PlanksFor(BlockType log)
{
    if (log == BLOCK_BIRCH_LOG) return BLOCK_BIRCH_PLANKS;
    if (log == BLOCK_ACACIA_LOG) return BLOCK_ACACIA_PLANKS;
    if (log == BLOCK_DARK_OAK_LOG) return BLOCK_DARK_OAK_PLANKS;
    return BLOCK_OAK_PLANKS;
}

static BlockType Cell(const Player *player, int x, int y)
{
    int index = 0;

    if ((x < 0) || (y < 0) || (x >= player->craftSize) || (y >= player->craftSize)) return BLOCK_AIR;
    index = y*player->craftSize + x;
    if (player->craftCount[index] <= 0) return BLOCK_AIR;
    return player->craft[index];
}

static int MatchCraft(const Player *player, BlockType *out, int *outCount)
{
    int n = player->craftSize;
    int minX = n;
    int minY = n;
    int maxX = -1;
    int maxY = -1;
    int filled = 0;
    int uniform = 1;
    BlockType only = BLOCK_AIR;
    int x = 0;
    int y = 0;

    if (n < 2) n = 2;
    for (y = 0; y < n; y++)
    {
        for (x = 0; x < n; x++)
        {
            BlockType block = Cell(player, x, y);
            if (block == BLOCK_AIR) continue;
            filled++;
            if (only == BLOCK_AIR) only = block;
            else if (only != block) uniform = 0;
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
        }
    }
    if (filled == 1 && IsLog(only))
    {
        *out = PlanksFor(only);
        *outCount = 4;
        return 1;
    }
    if (filled == 4 && uniform && (maxX - minX == 1) && (maxY - minY == 1))
    {
        if (IsPlanks(only))
        {
            *out = BLOCK_CRAFTING_TABLE;
            *outCount = 1;
            return 1;
        }
        if (only == BLOCK_SAND)
        {
            *out = BLOCK_SANDSTONE;
            *outCount = 1;
            return 1;
        }
        if (only == BLOCK_RED_SAND)
        {
            *out = BLOCK_RED_SANDSTONE;
            *outCount = 1;
            return 1;
        }
        if (only == BLOCK_COBBLESTONE)
        {
            *out = BLOCK_STONE_BRICKS;
            *outCount = 1;
            return 1;
        }
    }
    if ((n == 3) && (filled == 8) && uniform && (Cell(player, 1, 1) == BLOCK_AIR))
    {
        if (only == BLOCK_COBBLESTONE)
        {
            *out = BLOCK_FURNACE;
            *outCount = 1;
            return 1;
        }
        if (IsPlanks(only))
        {
            *out = BLOCK_CHEST;
            *outCount = 1;
            return 1;
        }
    }
    *out = BLOCK_AIR;
    *outCount = 0;
    return 0;
}

static void ClickStack(BlockType *block, int *count, Player *player)
{
    BlockType held = player->cursorBlock;
    int heldCount = player->cursorCount;

    if ((held == BLOCK_AIR) || (heldCount <= 0))
    {
        player->cursorBlock = *block;
        player->cursorCount = *count;
        *block = BLOCK_AIR;
        *count = 0;
        return;
    }
    if ((*block == BLOCK_AIR) || (*count <= 0))
    {
        *block = held;
        *count = heldCount;
        player->cursorBlock = BLOCK_AIR;
        player->cursorCount = 0;
        return;
    }
    if (*block == held)
    {
        *count += heldCount;
        player->cursorBlock = BLOCK_AIR;
        player->cursorCount = 0;
        return;
    }
    player->cursorBlock = *block;
    player->cursorCount = *count;
    *block = held;
    *count = heldCount;
}

static void StowCursor(Player *player)
{
    int i = 0;

    if ((player->cursorBlock == BLOCK_AIR) || (player->cursorCount <= 0))
    {
        player->cursorBlock = BLOCK_AIR;
        player->cursorCount = 0;
        return;
    }
    for (i = 0; i < INVENTORY_SIZE; i++)
    {
        if (player->inventory.blocks[i] == player->cursorBlock)
        {
            player->inventory.quantities[i] += player->cursorCount;
            player->cursorBlock = BLOCK_AIR;
            player->cursorCount = 0;
            return;
        }
    }
    for (i = 0; i < INVENTORY_SIZE; i++)
    {
        if ((player->inventory.blocks[i] == BLOCK_AIR) || (player->inventory.quantities[i] <= 0))
        {
            player->inventory.blocks[i] = player->cursorBlock;
            player->inventory.quantities[i] = player->cursorCount;
            player->cursorBlock = BLOCK_AIR;
            player->cursorCount = 0;
            return;
        }
    }
}

void InventoryClose(Player *player)
{
    StowCursor(player);
    player->inventoryOpen = false;
    DisableCursor();
}

static void FillRecipe(Player *player, int index)
{
    const CraftRecipe *recipe = NULL;
    int i = 0;
    int n = 0;

    if ((index < 0) || (index >= RecipeCount())) return;
    recipe = &recipes[index];
    if (recipe->width > player->craftSize) return;
    n = player->craftSize*player->craftSize;
    for (i = 0; i < 9; i++)
    {
        player->craft[i] = BLOCK_AIR;
        player->craftCount[i] = 0;
    }
    if (recipe->kind == 1)
    {
        player->craft[0] = recipe->item;
        player->craftCount[0] = 1;
    }
    else if (recipe->kind == 4)
    {
        for (i = 0; i < n; i++)
        {
            int x = i%player->craftSize;
            int y = i/player->craftSize;
            if ((x < 2) && (y < 2))
            {
                player->craft[i] = recipe->item;
                player->craftCount[i] = 1;
            }
        }
    }
    else if ((recipe->kind == 8) && (player->craftSize == 3))
    {
        for (i = 0; i < 9; i++)
        {
            if (i == 4) continue;
            player->craft[i] = recipe->item;
            player->craftCount[i] = 1;
        }
    }
    player->recipeIndex = index;
}

static void TakeResult(Player *player)
{
    BlockType out = BLOCK_AIR;
    int outCount = 0;
    int i = 0;
    int n = 0;

    if (!MatchCraft(player, &out, &outCount)) return;
    if ((player->cursorBlock != BLOCK_AIR) && (player->cursorBlock != out)) return;
    if (player->cursorBlock == out) player->cursorCount += outCount;
    else
    {
        player->cursorBlock = out;
        player->cursorCount = outCount;
    }
    n = player->craftSize*player->craftSize;
    for (i = 0; i < n; i++)
    {
        if (player->craftCount[i] > 0) player->craftCount[i]--;
        if (player->craftCount[i] <= 0)
        {
            player->craft[i] = BLOCK_AIR;
            player->craftCount[i] = 0;
        }
    }
    player->selectedBlock = out;
}

static void RememberHeld(Player *player)
{
    if ((player->hotbarSlot >= 0) && (player->hotbarSlot < 9))
    {
        player->selectedBlock = player->hotbar[player->hotbarSlot];
    }
}

static Rectangle GuiOrigin(const Player *player, int *scaleOut, int *panelW)
{
    int scale = MenuScale();
    int guiW = 176*scale;
    int extra = 0;
    int ox = 0;

    if (player->recipeBookOpen) extra = 120*scale;
    ox = (GetScreenWidth() - guiW - extra)/2 + extra;
    if (scaleOut != NULL) *scaleOut = scale;
    if (panelW != NULL) *panelW = extra;
    return (Rectangle){ (float)ox, (float)((GetScreenHeight() - 166*scale)/2), (float)guiW, (float)(166*scale) };
}

static Rectangle SlotBox(Rectangle gui, int scale, int x, int y)
{
    return (Rectangle){ gui.x + (float)(x*scale), gui.y + (float)(y*scale), (float)(18*scale), (float)(18*scale) };
}

static void DrawIcon(Rectangle box, int scale, BlockType block, int count)
{
    Texture2D atlas = { 0 };
    float pad = 0.0f;

    if ((block == BLOCK_AIR) || (count <= 0)) return;
    pad = (float)scale;
    atlas = GetTextureAtlas();
    if (atlas.id > 0)
    {
        float u = 0.0f;
        float v = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        Rectangle src = { 0 };
        Rectangle dest = { box.x + pad, box.y + pad, box.width - pad*2.0f, box.height - pad*2.0f };

        GetBlockTextureUV(block, FACE_TOP, &u, &v, &w, &h);
        src = (Rectangle){ u*(float)atlas.width, v*(float)atlas.height, w*(float)atlas.width, h*(float)atlas.height };
        DrawTexturePro(atlas, src, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
    else
    {
        DrawRectangle((int)(box.x + pad), (int)(box.y + pad), (int)(box.width - pad*2.0f), (int)(box.height - pad*2.0f), GetBlockColor(block));
    }
    if (count > 1)
    {
        const char *text = TextFormat("%d", count);
        int fontSize = 8*((scale > 1) ? scale - 1 : 1);
        int width = 0;

        if (fontSize < 10) fontSize = 10;
        width = MeasureText(text, fontSize);
        DrawText(text, (int)(box.x + box.width - (float)width - pad), (int)(box.y + box.height - (float)fontSize - 1), fontSize, WHITE);
    }
}

static void DrawFrame(Rectangle box, Color color)
{
    DrawRectangleLinesEx(box, 2.0f, color);
}

void DrawHotbar(Player *player)
{
    int scale = MenuScale();
    int width = 182*scale;
    int height = 22*scale;
    int x = (GetScreenWidth() - width)/2;
    int y = GetScreenHeight() - height;
    int i = 0;
    Rectangle bar = { (float)x, (float)y, (float)width, (float)height };

    LoadGui();
    if (guiWidgets.id != 0)
    {
        DrawTexturePro(guiWidgets, (Rectangle){ 0, 0, 182, 22 }, bar, (Vector2){ 0, 0 }, 0.0f, WHITE);
        if ((player->hotbarSlot >= 0) && (player->hotbarSlot < 9))
        {
            Rectangle sel = {
                (float)(x + player->hotbarSlot*20*scale - scale),
                (float)(y - scale),
                (float)(24*scale),
                (float)(24*scale)
            };
            DrawTexturePro(guiWidgets, (Rectangle){ 0, 22, 24, 23 }, sel, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }
    }
    else
    {
        DrawRectangleRec(bar, (Color){ 20, 20, 20, 180 });
    }
    for (i = 0; i < 9; i++)
    {
        Rectangle slot = { (float)(x + 3*scale + i*20*scale), (float)(y + 3*scale), (float)(16*scale), (float)(16*scale) };
        if (player->hotbar[i] != BLOCK_AIR) DrawIcon(slot, scale, player->hotbar[i], 1);
        if ((guiWidgets.id == 0) && (i == player->hotbarSlot)) DrawFrame(slot, YELLOW);
    }
}

static void DrawRecipePanel(const Player *player, int panelW, int scale, Rectangle gui)
{
    int i = 0;
    int shown = 0;
    int fontSize = 8*scale;
    Rectangle panel = { gui.x - (float)panelW, gui.y, (float)(panelW - 4*scale), gui.height };
    MenuButton book = { 0 };

    book.bounds = (Rectangle){ panel.x, panel.y - (float)(24*scale), (float)(panel.width), (float)(20*scale) };
    book.label = "Recipe Book";
    book.enabled = true;
    book.hovered = CheckCollisionPointRec(GetMousePosition(), book.bounds);
    DrawStoneButton(&book, player->recipeBookOpen);
    DrawRectangleRec(panel, (Color){ 198, 198, 198, 255 });
    DrawRectangleLinesEx(panel, (float)scale, (Color){ 55, 55, 55, 255 });
    DrawMenuText((int)panel.x + 4*scale, (int)panel.y + 4*scale, fontSize, "Select a recipe", (Color){ 64, 64, 64, 255 });
    for (i = 0; i < RecipeCount(); i++)
    {
        Rectangle row = { 0 };
        Color color = { 48, 48, 48, 255 };

        if (recipes[i].width > player->craftSize) continue;
        row = (Rectangle){ panel.x + 4*scale, panel.y + (float)((16 + shown*14)*scale), panel.width - 8*scale, (float)(12*scale) };
        if (i == player->recipeIndex) color = (Color){ 180, 120, 0, 255 };
        else if (CheckCollisionPointRec(GetMousePosition(), row)) color = (Color){ 80, 80, 160, 255 };
        DrawMenuText((int)row.x, (int)row.y, fontSize, recipes[i].name, color);
        shown++;
    }
}

void DrawInventory(Player *player)
{
    int scale = 0;
    int panelW = 0;
    Rectangle gui = GuiOrigin(player, &scale, &panelW);
    Texture2D sheet = { 0 };
    int row = 0;
    int col = 0;
    BlockType result = BLOCK_AIR;
    int resultCount = 0;
    Vector2 mouse = GetMousePosition();

    LoadGui();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.65f));
    if (player->recipeBookOpen && (panelW > 0)) DrawRecipePanel(player, panelW, scale, gui);
    else
    {
        MenuButton book = { 0 };
        book.bounds = (Rectangle){ gui.x - (float)(78*scale), gui.y + (float)(8*scale), (float)(74*scale), (float)(20*scale) };
        book.label = "Recipes";
        book.enabled = true;
        book.hovered = CheckCollisionPointRec(mouse, book.bounds);
        DrawStoneButton(&book, false);
    }

    sheet = (player->craftSize >= 3) ? guiCrafting : guiInventory;
    if (sheet.id != 0)
    {
        DrawTexturePro(sheet, (Rectangle){ 0, 0, 176, 166 }, gui, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
    else DrawRectangleRec(gui, (Color){ 198, 198, 198, 240 });

    if (player->craftSize >= 3)
    {
        for (row = 0; row < 3; row++)
        {
            for (col = 0; col < 3; col++)
            {
                int index = row*3 + col;
                Rectangle box = SlotBox(gui, scale, 30 + col*18, 17 + row*18);
                DrawIcon(box, scale, player->craft[index], player->craftCount[index]);
                if (CheckCollisionPointRec(mouse, box)) DrawFrame(box, WHITE);
            }
        }
    }
    else
    {
        int coords[4][2] = { { 98, 18 }, { 116, 18 }, { 98, 36 }, { 116, 36 } };
        for (col = 0; col < 4; col++)
        {
            Rectangle box = SlotBox(gui, scale, coords[col][0], coords[col][1]);
            DrawIcon(box, scale, player->craft[col], player->craftCount[col]);
            if (CheckCollisionPointRec(mouse, box)) DrawFrame(box, WHITE);
        }
    }

    MatchCraft(player, &result, &resultCount);
    {
        Rectangle box = (player->craftSize >= 3) ? SlotBox(gui, scale, 124, 35) : SlotBox(gui, scale, 154, 28);
        DrawIcon(box, scale, result, resultCount);
        if ((result != BLOCK_AIR) && CheckCollisionPointRec(mouse, box)) DrawFrame(box, YELLOW);
    }

    for (row = 0; row < 3; row++)
    {
        for (col = 0; col < 9; col++)
        {
            int index = row*9 + col;
            Rectangle box = SlotBox(gui, scale, 8 + col*18, 84 + row*18);
            if (index < INVENTORY_SIZE) DrawIcon(box, scale, player->inventory.blocks[index], player->inventory.quantities[index]);
            if (CheckCollisionPointRec(mouse, box)) DrawFrame(box, WHITE);
        }
    }
    for (col = 0; col < 9; col++)
    {
        Rectangle box = SlotBox(gui, scale, 8 + col*18, 142);
        DrawIcon(box, scale, player->hotbar[col], (player->hotbar[col] == BLOCK_AIR) ? 0 : 1);
        if (col == player->hotbarSlot) DrawFrame(box, YELLOW);
        else if (CheckCollisionPointRec(mouse, box)) DrawFrame(box, WHITE);
    }

    if ((player->cursorBlock != BLOCK_AIR) && (player->cursorCount > 0))
    {
        Rectangle cursor = { mouse.x - 8.0f*(float)scale, mouse.y - 8.0f*(float)scale, 16.0f*(float)scale, 16.0f*(float)scale };
        DrawIcon(cursor, scale, player->cursorBlock, player->cursorCount);
    }

    DrawMenuText(8*scale, GetScreenHeight() - 12*scale, 8*scale,
        (player->craftSize >= 3) ? "Crafting table. Pick a recipe or place blocks. E closes."
                                 : "Pick a recipe or drag blocks into the grid. E on a crafting table opens the 3x3.",
        (Color){ 224, 224, 224, 255 });
}

static int HitStorage(Rectangle gui, int scale, Vector2 mouse, int *index)
{
    int row = 0;
    int col = 0;

    for (row = 0; row < 3; row++)
    {
        for (col = 0; col < 9; col++)
        {
            Rectangle box = SlotBox(gui, scale, 8 + col*18, 84 + row*18);
            if (CheckCollisionPointRec(mouse, box))
            {
                *index = row*9 + col;
                return 1;
            }
        }
    }
    return 0;
}

static int HitHotbar(Rectangle gui, int scale, Vector2 mouse, int *index)
{
    int col = 0;

    for (col = 0; col < 9; col++)
    {
        Rectangle box = SlotBox(gui, scale, 8 + col*18, 142);
        if (CheckCollisionPointRec(mouse, box))
        {
            *index = col;
            return 1;
        }
    }
    return 0;
}

static int HitCraft(const Player *player, Rectangle gui, int scale, Vector2 mouse, int *index)
{
    int row = 0;
    int col = 0;

    if (player->craftSize >= 3)
    {
        for (row = 0; row < 3; row++)
        {
            for (col = 0; col < 3; col++)
            {
                Rectangle box = SlotBox(gui, scale, 30 + col*18, 17 + row*18);
                if (CheckCollisionPointRec(mouse, box))
                {
                    *index = row*3 + col;
                    return 1;
                }
            }
        }
        return 0;
    }
    {
        int coords[4][2] = { { 98, 18 }, { 116, 18 }, { 98, 36 }, { 116, 36 } };
        for (col = 0; col < 4; col++)
        {
            Rectangle box = SlotBox(gui, scale, coords[col][0], coords[col][1]);
            if (CheckCollisionPointRec(mouse, box))
            {
                *index = col;
                return 1;
            }
        }
    }
    return 0;
}

static int HitResult(const Player *player, Rectangle gui, int scale, Vector2 mouse)
{
    Rectangle box = (player->craftSize >= 3) ? SlotBox(gui, scale, 124, 35) : SlotBox(gui, scale, 154, 28);
    return CheckCollisionPointRec(mouse, box);
}

static int HitRecipe(const Player *player, int panelW, int scale, Rectangle gui, Vector2 mouse)
{
    int i = 0;
    int shown = 0;
    Rectangle panel = { gui.x - (float)panelW, gui.y, (float)(panelW - 4*scale), gui.height };

    if (!player->recipeBookOpen) return -1;
    for (i = 0; i < RecipeCount(); i++)
    {
        Rectangle row = { 0 };
        if (recipes[i].width > player->craftSize) continue;
        row = (Rectangle){ panel.x + 4*scale, panel.y + (float)((16 + shown*14)*scale), panel.width - 8*scale, (float)(12*scale) };
        if (CheckCollisionPointRec(mouse, row)) return i;
        shown++;
    }
    return -1;
}

static void ClickHotbar(Player *player, int index)
{
    BlockType slot = player->hotbar[index];

    if ((player->cursorBlock == BLOCK_AIR) || (player->cursorCount <= 0))
    {
        if (slot == BLOCK_AIR) return;
        player->cursorBlock = slot;
        player->cursorCount = 1;
        player->hotbar[index] = BLOCK_AIR;
    }
    else
    {
        player->hotbar[index] = player->cursorBlock;
        player->cursorCount--;
        if (player->cursorCount <= 0)
        {
            player->cursorBlock = BLOCK_AIR;
            player->cursorCount = 0;
        }
    }
    player->hotbarSlot = index;
    RememberHeld(player);
}

void InventoryHandleInput(Player *player)
{
    int scale = 0;
    int panelW = 0;
    Rectangle gui = { 0 };
    Vector2 mouse = GetMousePosition();
    int index = 0;
    MenuButton book = { 0 };

    if (!player->inventoryOpen) return;
    gui = GuiOrigin(player, &scale, &panelW);
    if (!player->recipeBookOpen)
    {
        book.bounds = (Rectangle){ gui.x - (float)(78*scale), gui.y + (float)(8*scale), (float)(74*scale), (float)(20*scale) };
        book.label = "Recipes";
        book.enabled = true;
    }
    else
    {
        book.bounds = (Rectangle){ gui.x - (float)panelW, gui.y - (float)(24*scale), (float)(panelW - 4*scale), (float)(20*scale) };
        book.label = "Recipe Book";
        book.enabled = true;
    }
    UpdateMenuButton(&book);
    if (book.clicked)
    {
        player->recipeBookOpen = !player->recipeBookOpen;
        PlaySound(fxCoin);
        return;
    }
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    index = HitRecipe(player, panelW, scale, gui, mouse);
    if (index >= 0)
    {
        FillRecipe(player, index);
        PlaySound(fxCoin);
        return;
    }
    if (HitResult(player, gui, scale, mouse))
    {
        TakeResult(player);
        PlaySound(fxCoin);
        return;
    }
    if (HitCraft(player, gui, scale, mouse, &index))
    {
        ClickStack(&player->craft[index], &player->craftCount[index], player);
        if (player->craftCount[index] <= 0) player->craft[index] = BLOCK_AIR;
        return;
    }
    if (HitStorage(gui, scale, mouse, &index) && (index >= 0) && (index < INVENTORY_SIZE))
    {
        ClickStack(&player->inventory.blocks[index], &player->inventory.quantities[index], player);
        return;
    }
    if (HitHotbar(gui, scale, mouse, &index)) ClickHotbar(player, index);
}

int GetInventorySlotAtMouse(Vector2 mousePos)
{
    int scale = 0;
    int panelW = 0;
    Rectangle gui = { 0 };
    int index = 0;
    Player dummy = { 0 };

    dummy.recipeBookOpen = 0;
    dummy.craftSize = 2;
    gui = GuiOrigin(&dummy, &scale, &panelW);
    if (HitStorage(gui, scale, mousePos, &index)) return index;
    return -1;
}
