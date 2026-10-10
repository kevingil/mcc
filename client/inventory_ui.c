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

static Texture2D guiIcons = { 0 };
static Texture2D guiSteve = { 0 };
static int guiReady = 0;

static int RecipeCount(void)
{
    return (int)(sizeof(recipes)/sizeof(recipes[0]));
}

static void LoadGui(void)
{
    if (guiReady) return;
    guiReady = 1;
    guiIcons = LoadTexture("resources/textures/gui/icons.png");
    guiSteve = LoadTexture("resources/textures/entity/steve.png");
    if (guiIcons.id != 0) SetTextureFilter(guiIcons, TEXTURE_FILTER_POINT);
    if (guiSteve.id != 0) SetTextureFilter(guiSteve, TEXTURE_FILTER_POINT);
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

static int TextHas(const char *haystack, const char *needle)
{
    int i = 0;
    int j = 0;

    if ((needle == NULL) || (needle[0] == '\0')) return 1;
    if (haystack == NULL) return 0;
    for (i = 0; haystack[i] != '\0'; i++)
    {
        for (j = 0; needle[j] != '\0'; j++)
        {
            char a = haystack[i + j];
            char b = needle[j];
            if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
            if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
            if (a != b) break;
        }
        if (needle[j] == '\0') return 1;
    }
    return 0;
}

static int Catalog(const char *query, BlockType *out, int cap)
{
    int i = 0;
    int n = 0;

    for (i = 1; (i < BLOCK_COUNT) && (n < cap); i++)
    {
        BlockType block = (BlockType)i;
        if (IsWaterBlock(block) && (block != BLOCK_WATER)) continue;
        if (!TextHas(GetBlockName(block), query)) continue;
        out[n++] = block;
    }
    return n;
}

static Rectangle GuiOrigin(const Player *player, int *scaleOut, int *panelW)
{
    int scale = MenuScale();
    int guiW = 176*scale;
    int left = player->recipeBookOpen ? 124*scale : 0;
    int right = player->showAllItems ? 180*scale : 0;
    int ox = (GetScreenWidth() - guiW - left - right)/2 + left;

    if (scaleOut != NULL) *scaleOut = scale;
    if (panelW != NULL) *panelW = left;
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

static void BlitGui(Texture2D tex, int sx, int sy, int sw, int sh, int dx, int dy, int scale)
{
    if (tex.id == 0) return;
    DrawTexturePro(tex,
        (Rectangle){ (float)sx, (float)sy, (float)sw, (float)sh },
        (Rectangle){ (float)dx, (float)dy, (float)(sw*scale), (float)(sh*scale) },
        (Vector2){ 0, 0 }, 0.0f, WHITE);
}

static void DrawStatusIcon(int sx, int sy, int dx, int dy, int scale)
{
    BlitGui(guiIcons, sx, sy, 9, 9, dx, dy, scale);
}

static void DrawGraySlot(int x, int y, int size, int hot)
{
    int rim = size/18;
    if (rim < 1) rim = 1;
    DrawRectangle(x, y, size, size, hot ? WHITE : (Color){ 55, 55, 55, 255 });
    DrawRectangle(x + rim, y + rim, size - 2*rim, size - 2*rim, (Color){ 139, 139, 139, 255 });
    DrawRectangle(x + rim, y + rim, size - 2*rim, rim, (Color){ 55, 55, 55, 255 });
    DrawRectangle(x + rim, y + size - 2*rim, size - 2*rim, rim, (Color){ 255, 255, 255, 180 });
}

static void DrawSteve(int x, int y, int px)
{
    int head = 8*px;
    int bodyW = 8*px;
    int bodyH = 12*px;
    int armW = 4*px;

    if (guiSteve.id == 0) return;
    BlitGui(guiSteve, 8, 8, 8, 8, x, y, px);
    BlitGui(guiSteve, 40, 8, 8, 8, x, y, px);
    BlitGui(guiSteve, 20, 20, 8, 12, x, y + head, px);
    BlitGui(guiSteve, 44, 20, 4, 12, x - armW, y + head, px);
    BlitGui(guiSteve, 36, 52, 4, 12, x + bodyW, y + head, px);
    BlitGui(guiSteve, 4, 20, 4, 12, x, y + head + bodyH, px);
    BlitGui(guiSteve, 20, 52, 4, 12, x + armW, y + head + bodyH, px);
}

void DrawHotbar(Player *player)
{
    int scale = MenuScale();
    int barW = 182*scale;
    int barH = 22*scale;
    int x = (GetScreenWidth() - barW)/2;
    int y = GetScreenHeight() - barH;
    int i = 0;
    const char *held = NULL;
    int heldWidth = 0;

    LoadGui();
    DrawRectangle(x, y - 6*scale, barW, 5*scale, BLACK);
    DrawRectangle(x + scale, y - 5*scale, barW - 2*scale, 3*scale, (Color){ 128, 255, 32, 255 });

    for (i = 0; i < 10; i++)
    {
        int hx = x + scale + i*8*scale;
        int hy = y - 16*scale;
        int fx = x + barW - scale - 9*scale - i*8*scale;

        DrawStatusIcon(16, 0, hx, hy, scale);
        DrawStatusIcon(52, 0, hx, hy, scale);
        DrawStatusIcon(16, 9, hx, hy - 10*scale, scale);
        DrawStatusIcon(16, 27, fx, hy, scale);
        DrawStatusIcon(52, 27, fx, hy, scale);
        if (player->inWater) DrawStatusIcon(16, 18, fx, hy - 10*scale, scale);
    }

    DrawRectangle(x, y, barW, barH, (Color){ 12, 12, 12, 255 });
    for (i = 0; i < 9; i++)
    {
        int sx = x + (1 + i*20)*scale;
        int sy = y + scale;
        int inner = 18*scale;
        Rectangle icon = { (float)(sx + scale), (float)(sy + scale), (float)(16*scale), (float)(16*scale) };

        DrawRectangle(sx, sy, 20*scale, 20*scale, (i == player->hotbarSlot) ? WHITE : (Color){ 90, 90, 90, 255 });
        DrawRectangle(sx + scale, sy + scale, inner, inner, (Color){ 28, 28, 28, 255 });
        if (player->hotbar[i] != BLOCK_AIR) DrawIcon(icon, scale, player->hotbar[i], 1);
    }

    DrawRectangle(x - 26*scale, y + scale, 20*scale, 20*scale, (Color){ 90, 90, 90, 255 });
    DrawRectangle(x - 25*scale, y + 2*scale, 18*scale, 18*scale, (Color){ 28, 28, 28, 255 });

    if ((player->hotbarSlot >= 0) && (player->hotbarSlot < 9) && (player->hotbar[player->hotbarSlot] != BLOCK_AIR))
    {
        held = GetBlockName(player->hotbar[player->hotbarSlot]);
        heldWidth = MenuTextWidth(held, 8*scale);
        DrawMenuText(GetScreenWidth()/2 - heldWidth/2 + scale, y - 28*scale + scale, 8*scale, held, BLACK);
        DrawMenuText(GetScreenWidth()/2 - heldWidth/2, y - 28*scale, 8*scale, held, WHITE);
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

static void DrawSlotContents(Rectangle box, int scale, BlockType block, int count, int hot)
{
    DrawGraySlot((int)box.x, (int)box.y, (int)box.width, hot);
    if ((block != BLOCK_AIR) && (count > 0))
    {
        Rectangle icon = { box.x + (float)scale, box.y + (float)scale, box.width - 2.0f*(float)scale, box.height - 2.0f*(float)scale };
        DrawIcon(icon, scale, block, count);
    }
}

static Rectangle SearchPanel(Rectangle gui, int scale)
{
    return (Rectangle){ gui.x + gui.width + 4.0f*(float)scale, gui.y, 176.0f*(float)scale, gui.height };
}

static Rectangle SearchField(Rectangle panel, int scale)
{
    return (Rectangle){ panel.x + 8.0f*(float)scale, panel.y + 22.0f*(float)scale, panel.width - 16.0f*(float)scale, 16.0f*(float)scale };
}

static void DrawSearchPanel(Player *player, Rectangle panel, int scale)
{
    BlockType items[512];
    int count = Catalog(player->itemSearch, items, 512);
    int cols = 9;
    int rows = 5;
    int visible = cols*rows;
    int maxScroll = 0;
    int i = 0;
    Vector2 mouse = GetMousePosition();
    Rectangle field = SearchField(panel, scale);

    if (count > visible) maxScroll = ((count - visible + cols - 1)/cols)*cols;
    if (player->itemScroll < 0) player->itemScroll = 0;
    if (player->itemScroll > maxScroll) player->itemScroll = maxScroll;

    DrawRectangleRec(panel, (Color){ 198, 198, 198, 255 });
    DrawRectangleLinesEx(panel, (float)scale, (Color){ 55, 55, 55, 255 });
    DrawMenuText((int)panel.x + 8*scale, (int)panel.y + 6*scale, 8*scale, "Search Items", (Color){ 64, 64, 64, 255 });
    DrawRectangleRec(field, player->searchFocused ? WHITE : (Color){ 0, 0, 0, 255 });
    DrawMenuText((int)field.x + 2*scale, (int)field.y + 4*scale, 8*scale,
        (player->itemSearch[0] != '\0') ? player->itemSearch : "Search...",
        (player->itemSearch[0] != '\0') ? WHITE : (Color){ 160, 160, 160, 255 });
    if (player->searchFocused && (player->itemSearch[0] != '\0'))
    {
        DrawMenuText((int)field.x + 2*scale, (int)field.y + 4*scale, 8*scale, player->itemSearch, BLACK);
    }

    for (i = 0; i < visible; i++)
    {
        int index = player->itemScroll + i;
        int col = i%cols;
        int row = i/cols;
        Rectangle box = {
            panel.x + (8 + col*18)*(float)scale,
            panel.y + (44 + row*18)*(float)scale,
            18.0f*(float)scale,
            18.0f*(float)scale
        };
        BlockType block = BLOCK_AIR;

        if (index < count) block = items[index];
        DrawSlotContents(box, scale, block, (block == BLOCK_AIR) ? 0 : 1, CheckCollisionPointRec(mouse, box));
        if ((block != BLOCK_AIR) && CheckCollisionPointRec(mouse, box))
        {
            DrawMenuText((int)panel.x + 8*scale, (int)panel.y + (int)panel.height - 14*scale, 8*scale, GetBlockName(block), (Color){ 64, 64, 64, 255 });
        }
    }
}

void DrawInventory(Player *player)
{
    int scale = 0;
    int panelW = 0;
    Rectangle gui = GuiOrigin(player, &scale, &panelW);
    int row = 0;
    int col = 0;
    int armor = 0;
    BlockType result = BLOCK_AIR;
    int resultCount = 0;
    Vector2 mouse = GetMousePosition();
    int portraitX = (int)gui.x + 26*scale;
    int portraitY = (int)gui.y + 8*scale;
    int pixel = scale;
    MenuButton book = { 0 };
    MenuButton allItems = { 0 };

    if (pixel < 2) pixel = 2;
    LoadGui();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.65f));

    book.bounds = (Rectangle){ gui.x - (float)(78*scale), gui.y + (float)(8*scale), (float)(74*scale), (float)(20*scale) };
    book.label = "Recipes";
    book.enabled = true;
    book.hovered = CheckCollisionPointRec(mouse, book.bounds);
    DrawStoneButton(&book, player->recipeBookOpen);
    if (player->recipeBookOpen && (panelW > 0)) DrawRecipePanel(player, panelW, scale, gui);

    allItems.bounds = (Rectangle){ gui.x + gui.width - (float)(78*scale), gui.y - (float)(24*scale), (float)(74*scale), (float)(20*scale) };
    allItems.label = player->showAllItems ? "Close" : "All Items";
    allItems.enabled = true;
    allItems.hovered = CheckCollisionPointRec(mouse, allItems.bounds);
    DrawStoneButton(&allItems, player->showAllItems);

    DrawRectangle((int)gui.x, (int)gui.y, (int)gui.width, (int)gui.height, (Color){ 198, 198, 198, 255 });
    DrawRectangleLinesEx(gui, (float)scale, (Color){ 55, 55, 55, 255 });
    DrawMenuText((int)gui.x + 96*scale, (int)gui.y + 6*scale, 8*scale, "Crafting", (Color){ 64, 64, 64, 255 });

    DrawRectangle(portraitX, portraitY, 50*scale, 70*scale, BLACK);
    DrawSteve(portraitX + 16*pixel, portraitY + 4*pixel, pixel);
    for (armor = 0; armor < 4; armor++)
    {
        Rectangle box = SlotBox(gui, scale, 8, 8 + armor*18);
        DrawSlotContents(box, scale, BLOCK_AIR, 0, 0);
    }
    DrawSlotContents(SlotBox(gui, scale, 77, 62), scale, BLOCK_AIR, 0, 0);

    if (player->craftSize >= 3)
    {
        for (row = 0; row < 3; row++)
        {
            for (col = 0; col < 3; col++)
            {
                int index = row*3 + col;
                Rectangle box = SlotBox(gui, scale, 98 + col*18, 18 + row*18);
                DrawSlotContents(box, scale, player->craft[index], player->craftCount[index], CheckCollisionPointRec(mouse, box));
            }
        }
    }
    else
    {
        int coords[4][2] = { { 98, 18 }, { 116, 18 }, { 98, 36 }, { 116, 36 } };
        for (col = 0; col < 4; col++)
        {
            Rectangle box = SlotBox(gui, scale, coords[col][0], coords[col][1]);
            DrawSlotContents(box, scale, player->craft[col], player->craftCount[col], CheckCollisionPointRec(mouse, box));
        }
    }

    MatchCraft(player, &result, &resultCount);
    {
        Rectangle box = (player->craftSize >= 3) ? SlotBox(gui, scale, 154, 36) : SlotBox(gui, scale, 154, 28);
        DrawSlotContents(box, scale, result, resultCount, (result != BLOCK_AIR) && CheckCollisionPointRec(mouse, box));
        DrawRectangle((int)box.x - 14*scale, (int)box.y + 6*scale, 10*scale, 2*scale, (Color){ 64, 64, 64, 255 });
    }

    for (row = 0; row < 3; row++)
    {
        for (col = 0; col < 9; col++)
        {
            int index = row*9 + col;
            Rectangle box = SlotBox(gui, scale, 8 + col*18, 84 + row*18);
            BlockType block = (index < INVENTORY_SIZE) ? player->inventory.blocks[index] : BLOCK_AIR;
            int count = (index < INVENTORY_SIZE) ? player->inventory.quantities[index] : 0;
            DrawSlotContents(box, scale, block, count, CheckCollisionPointRec(mouse, box));
        }
    }
    for (col = 0; col < 9; col++)
    {
        Rectangle box = SlotBox(gui, scale, 8 + col*18, 142);
        DrawSlotContents(box, scale, player->hotbar[col], (player->hotbar[col] == BLOCK_AIR) ? 0 : 1,
            (col == player->hotbarSlot) || CheckCollisionPointRec(mouse, box));
    }

    if (player->showAllItems) DrawSearchPanel(player, SearchPanel(gui, scale), scale);

    if ((player->cursorBlock != BLOCK_AIR) && (player->cursorCount > 0))
    {
        Rectangle cursor = { mouse.x - 8.0f*(float)scale, mouse.y - 8.0f*(float)scale, 16.0f*(float)scale, 16.0f*(float)scale };
        DrawIcon(cursor, scale, player->cursorBlock, player->cursorCount);
    }
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
                Rectangle box = SlotBox(gui, scale, 98 + col*18, 18 + row*18);
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
    Rectangle box = (player->craftSize >= 3) ? SlotBox(gui, scale, 154, 36) : SlotBox(gui, scale, 154, 28);
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

static int HitCatalog(Player *player, Rectangle panel, int scale, Vector2 mouse, BlockType *block)
{
    BlockType items[512];
    int count = Catalog(player->itemSearch, items, 512);
    int cols = 9;
    int rows = 5;
    int i = 0;

    for (i = 0; i < cols*rows; i++)
    {
        int index = player->itemScroll + i;
        int col = i%cols;
        int row = i/cols;
        Rectangle box = {
            panel.x + (8 + col*18)*(float)scale,
            panel.y + (44 + row*18)*(float)scale,
            18.0f*(float)scale,
            18.0f*(float)scale
        };

        if (!CheckCollisionPointRec(mouse, box)) continue;
        if (index >= count) return 0;
        *block = items[index];
        return 1;
    }
    return 0;
}

static void TypeSearch(Player *player)
{
    int len = (int)strlen(player->itemSearch);
    int key = GetCharPressed();

    while (key > 0)
    {
        if ((key >= 32) && (key < 127) && (len < (int)sizeof(player->itemSearch) - 1))
        {
            player->itemSearch[len] = (char)key;
            len++;
            player->itemSearch[len] = '\0';
            player->itemScroll = 0;
        }
        key = GetCharPressed();
    }
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && (len > 0))
    {
        player->itemSearch[len - 1] = '\0';
        player->itemScroll = 0;
    }
}

void InventoryHandleInput(Player *player)
{
    int scale = 0;
    int panelW = 0;
    Rectangle gui = { 0 };
    Vector2 mouse = GetMousePosition();
    int index = 0;
    MenuButton book = { 0 };
    MenuButton allItems = { 0 };
    Rectangle panel = { 0 };
    BlockType picked = BLOCK_AIR;

    if (!player->inventoryOpen) return;
    gui = GuiOrigin(player, &scale, &panelW);
    book.bounds = (Rectangle){ gui.x - (float)(78*scale), gui.y + (float)(8*scale), (float)(74*scale), (float)(20*scale) };
    book.label = "Recipes";
    book.enabled = true;
    allItems.bounds = (Rectangle){ gui.x + gui.width - (float)(78*scale), gui.y - (float)(24*scale), (float)(74*scale), (float)(20*scale) };
    allItems.label = player->showAllItems ? "Close" : "All Items";
    allItems.enabled = true;
    UpdateMenuButton(&book);
    UpdateMenuButton(&allItems);
    if (book.clicked)
    {
        player->recipeBookOpen = !player->recipeBookOpen;
        PlaySound(fxCoin);
        return;
    }
    if (allItems.clicked)
    {
        player->showAllItems = !player->showAllItems;
        player->searchFocused = player->showAllItems;
        PlaySound(fxCoin);
        return;
    }

    if (player->showAllItems)
    {
        panel = SearchPanel(gui, scale);
        if (CheckCollisionPointRec(mouse, panel))
        {
            float wheel = GetMouseWheelMove();
            if (wheel > 0.0f) player->itemScroll -= 9;
            if (wheel < 0.0f) player->itemScroll += 9;
            if (player->itemScroll < 0) player->itemScroll = 0;
        }
    }
    if (player->searchFocused) TypeSearch(player);
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    if (player->showAllItems && CheckCollisionPointRec(mouse, SearchField(panel, scale)))
    {
        player->searchFocused = 1;
        return;
    }
    player->searchFocused = 0;
    if (player->showAllItems && HitCatalog(player, panel, scale, mouse, &picked))
    {
        player->cursorBlock = picked;
        player->cursorCount = 64;
        PlaySound(fxCoin);
        return;
    }
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
