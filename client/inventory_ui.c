#include "player.h"
#include "crafting.h"
#include "gui.h"
#include "hud.h"
#include "player_model.h"
#include "text_field.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// The three container screens: the player's own inventory with its 2x2 grid,
// the crafting table, and the creative tabs. Slot positions, the click rules,
// the drag spreading, and the draw order follow the vanilla screens.

#define MENU_SLOT_MAX 64
#define PICKER_MAX 512
#define SLOT_NONE (-1)
#define SLOT_OUTSIDE (-999)
#define SURVIVAL_WIDTH 176
#define SURVIVAL_HEIGHT 166
#define CREATIVE_WIDTH 195
#define CREATIVE_HEIGHT 136
#define DOUBLE_CLICK_SECONDS 0.25
#define OFFHAND_BUTTON 40

typedef enum {
    SCREEN_INVENTORY = 0,
    SCREEN_CRAFTING,
    SCREEN_CREATIVE
} ScreenKind;

typedef enum {
    SLOT_RESULT = 0,
    SLOT_CRAFT,
    SLOT_ARMOR,
    SLOT_STORAGE,
    SLOT_HOTBAR,
    SLOT_OFFHAND,
    SLOT_PICKER,
    SLOT_DESTROY
} SlotKind;

typedef enum {
    CLICK_PICKUP = 0,
    CLICK_QUICK_MOVE,
    CLICK_SWAP,
    CLICK_CLONE,
    CLICK_THROW,
    CLICK_PICKUP_ALL
} ClickType;

typedef enum {
    TAB_KIND_CATEGORY = 0,
    TAB_KIND_HOTBAR,
    TAB_KIND_SEARCH,
    TAB_KIND_INVENTORY
} TabKind;

enum {
    TAB_BUILDING = 0,
    TAB_COLORED,
    TAB_NATURAL,
    TAB_FUNCTIONAL,
    TAB_REDSTONE,
    TAB_HOTBAR,
    TAB_SEARCH,
    TAB_TOOLS,
    TAB_COMBAT,
    TAB_FOOD,
    TAB_INGREDIENTS,
    TAB_SPAWN_EGGS,
    TAB_INVENTORY,
    TAB_COUNT
};

typedef struct {
    SlotKind kind;
    int index;
    int x;
    int y;
} MenuSlot;

typedef struct {
    BlockType block;
    int count;
} Stack;

typedef struct {
    BlockType block;
    int count;
    int lockedRow;          // An empty saved hotbar row shows a locked paper, -1 otherwise
} PickerEntry;

typedef struct {
    const char *name;
    bool top;
    int column;
    TabKind kind;
    BlockType iconBlock;
    const char *iconTexture;
    const BlockType *items;
    int itemCount;
} CreativeTab;

#define LIST_COUNT(list) ((int)(sizeof(list)/sizeof((list)[0])))

static const BlockType buildingItems[] = {
    BLOCK_OAK_LOG, BLOCK_OAK_PLANKS, BLOCK_BIRCH_LOG, BLOCK_BIRCH_PLANKS, BLOCK_ACACIA_LOG, BLOCK_ACACIA_PLANKS,
    BLOCK_DARK_OAK_LOG, BLOCK_DARK_OAK_PLANKS, BLOCK_STONE, BLOCK_COBBLESTONE, BLOCK_MOSSY_COBBLESTONE, BLOCK_SMOOTH_STONE,
    BLOCK_STONE_BRICKS, BLOCK_CRACKED_STONE_BRICKS, BLOCK_MOSSY_STONE_BRICKS, BLOCK_GRANITE, BLOCK_DIORITE, BLOCK_ANDESITE,
    BLOCK_BRICKS, BLOCK_SANDSTONE, BLOCK_CHISELED_SANDSTONE, BLOCK_CUT_SANDSTONE, BLOCK_RED_SANDSTONE, BLOCK_PRISMARINE,
    BLOCK_QUARTZ_BLOCK, BLOCK_CHISELED_QUARTZ_BLOCK, BLOCK_QUARTZ_PILLAR, BLOCK_PURPUR_BLOCK, BLOCK_COAL_BLOCK,
    BLOCK_IRON_BLOCK, BLOCK_GOLD_BLOCK, BLOCK_REDSTONE_BLOCK, BLOCK_EMERALD_BLOCK, BLOCK_LAPIS_BLOCK, BLOCK_DIAMOND_BLOCK
};

static const BlockType coloredItems[] = {
    BLOCK_WHITE_WOOL, BLOCK_LIGHT_GRAY_WOOL, BLOCK_GRAY_WOOL, BLOCK_BLACK_WOOL, BLOCK_BROWN_WOOL, BLOCK_RED_WOOL,
    BLOCK_ORANGE_WOOL, BLOCK_YELLOW_WOOL, BLOCK_LIME_WOOL, BLOCK_GREEN_WOOL, BLOCK_CYAN_WOOL, BLOCK_LIGHT_BLUE_WOOL,
    BLOCK_BLUE_WOOL, BLOCK_PURPLE_WOOL, BLOCK_MAGENTA_WOOL, BLOCK_PINK_WOOL,
    BLOCK_TERRACOTTA, BLOCK_WHITE_TERRACOTTA, BLOCK_LIGHT_GRAY_TERRACOTTA, BLOCK_GRAY_TERRACOTTA, BLOCK_BLACK_TERRACOTTA,
    BLOCK_BROWN_TERRACOTTA, BLOCK_RED_TERRACOTTA, BLOCK_ORANGE_TERRACOTTA, BLOCK_YELLOW_TERRACOTTA, BLOCK_LIME_TERRACOTTA,
    BLOCK_GREEN_TERRACOTTA, BLOCK_CYAN_TERRACOTTA, BLOCK_LIGHT_BLUE_TERRACOTTA, BLOCK_BLUE_TERRACOTTA,
    BLOCK_PURPLE_TERRACOTTA, BLOCK_MAGENTA_TERRACOTTA, BLOCK_PINK_TERRACOTTA,
    BLOCK_WHITE_CONCRETE, BLOCK_LIGHT_GRAY_CONCRETE, BLOCK_GRAY_CONCRETE, BLOCK_BLACK_CONCRETE, BLOCK_BROWN_CONCRETE,
    BLOCK_RED_CONCRETE, BLOCK_ORANGE_CONCRETE, BLOCK_YELLOW_CONCRETE, BLOCK_LIME_CONCRETE, BLOCK_GREEN_CONCRETE,
    BLOCK_CYAN_CONCRETE, BLOCK_LIGHT_BLUE_CONCRETE, BLOCK_BLUE_CONCRETE, BLOCK_PURPLE_CONCRETE, BLOCK_MAGENTA_CONCRETE,
    BLOCK_PINK_CONCRETE,
    BLOCK_GLASS, BLOCK_WHITE_STAINED_GLASS, BLOCK_LIGHT_GRAY_STAINED_GLASS, BLOCK_GRAY_STAINED_GLASS,
    BLOCK_BLACK_STAINED_GLASS, BLOCK_BROWN_STAINED_GLASS, BLOCK_RED_STAINED_GLASS, BLOCK_ORANGE_STAINED_GLASS,
    BLOCK_YELLOW_STAINED_GLASS, BLOCK_LIME_STAINED_GLASS, BLOCK_GREEN_STAINED_GLASS, BLOCK_CYAN_STAINED_GLASS,
    BLOCK_LIGHT_BLUE_STAINED_GLASS, BLOCK_BLUE_STAINED_GLASS, BLOCK_PURPLE_STAINED_GLASS, BLOCK_MAGENTA_STAINED_GLASS,
    BLOCK_PINK_STAINED_GLASS
};

static const BlockType naturalItems[] = {
    BLOCK_GRASS, BLOCK_DIRT, BLOCK_CLAY, BLOCK_GRAVEL, BLOCK_SAND, BLOCK_SANDSTONE, BLOCK_RED_SAND, BLOCK_RED_SANDSTONE,
    BLOCK_ICE, BLOCK_PACKED_ICE, BLOCK_BLUE_ICE, BLOCK_SNOW_BLOCK, BLOCK_STONE, BLOCK_GRANITE, BLOCK_DIORITE, BLOCK_ANDESITE,
    BLOCK_OBSIDIAN, BLOCK_NETHERRACK, BLOCK_SOUL_SAND, BLOCK_MAGMA_BLOCK, BLOCK_BONE_BLOCK, BLOCK_END_STONE,
    BLOCK_COAL_ORE, BLOCK_IRON_ORE, BLOCK_GOLD_ORE, BLOCK_REDSTONE_ORE, BLOCK_EMERALD_ORE, BLOCK_LAPIS_ORE,
    BLOCK_DIAMOND_ORE, BLOCK_GLOWSTONE, BLOCK_OAK_LOG, BLOCK_BIRCH_LOG, BLOCK_ACACIA_LOG, BLOCK_DARK_OAK_LOG,
    BLOCK_OAK_LEAVES, BLOCK_BIRCH_LEAVES, BLOCK_ACACIA_LEAVES, BLOCK_DARK_OAK_LEAVES, BLOCK_CACTUS, BLOCK_MELON,
    BLOCK_PUMPKIN, BLOCK_JACK_O_LANTERN, BLOCK_HAY_BLOCK, BLOCK_HONEYCOMB_BLOCK, BLOCK_SPONGE, BLOCK_WET_SPONGE,
    BLOCK_BEDROCK
};

static const BlockType functionalItems[] = {
    BLOCK_GLOWSTONE, BLOCK_SEA_LANTERN, BLOCK_CRAFTING_TABLE, BLOCK_FURNACE, BLOCK_CHEST, BLOCK_BOOKSHELF
};

static const BlockType redstoneItems[] = { BLOCK_REDSTONE_BLOCK };

static const BlockType toolItems[] = { BLOCK_BUCKET, BLOCK_WATER_BUCKET };

// Columns 5 and 6 sit against the right edge. A category with nothing in it
// is not shown, which hides the tabs for item kinds that do not exist yet.
static const CreativeTab tabs[TAB_COUNT] = {
    { "Building Blocks", true, 0, TAB_KIND_CATEGORY, BLOCK_BRICKS, NULL, buildingItems, LIST_COUNT(buildingItems) },
    { "Colored Blocks", true, 1, TAB_KIND_CATEGORY, BLOCK_CYAN_WOOL, NULL, coloredItems, LIST_COUNT(coloredItems) },
    { "Natural Blocks", true, 2, TAB_KIND_CATEGORY, BLOCK_GRASS, NULL, naturalItems, LIST_COUNT(naturalItems) },
    { "Functional Blocks", true, 3, TAB_KIND_CATEGORY, BLOCK_AIR, "sign", functionalItems, LIST_COUNT(functionalItems) },
    { "Redstone Blocks", true, 4, TAB_KIND_CATEGORY, BLOCK_AIR, "redstone", redstoneItems, LIST_COUNT(redstoneItems) },
    { "Saved Hotbars", true, 5, TAB_KIND_HOTBAR, BLOCK_BOOKSHELF, NULL, NULL, 0 },
    { "Search Items", true, 6, TAB_KIND_SEARCH, BLOCK_AIR, "compass_00", NULL, 0 },
    { "Tools & Utilities", false, 0, TAB_KIND_CATEGORY, BLOCK_AIR, "diamond_pickaxe", toolItems, LIST_COUNT(toolItems) },
    { "Combat", false, 1, TAB_KIND_CATEGORY, BLOCK_AIR, NULL, NULL, 0 },
    { "Food & Drinks", false, 2, TAB_KIND_CATEGORY, BLOCK_AIR, NULL, NULL, 0 },
    { "Ingredients", false, 3, TAB_KIND_CATEGORY, BLOCK_AIR, NULL, NULL, 0 },
    { "Spawn Eggs", false, 4, TAB_KIND_CATEGORY, BLOCK_AIR, NULL, NULL, 0 },
    { "Survival Inventory", false, 6, TAB_KIND_INVENTORY, BLOCK_CHEST, NULL, NULL, 0 }
};

static const Color labelColor = { 64, 64, 64, 255 };
static const Color tabNameColor = { 85, 85, 255, 255 };
static const Color cappedCountColor = { 255, 255, 85, 255 };

//----------------------------------------------------------------------------------
// Module state
//----------------------------------------------------------------------------------
static ScreenKind screen = SCREEN_INVENTORY;
static MenuSlot slots[MENU_SLOT_MAX] = { 0 };
static int slotCount = 0;
static int imageWidth = SURVIVAL_WIDTH;
static int imageHeight = SURVIVAL_HEIGHT;
static int leftPos = 0;
static int topPos = 0;

static bool quickCrafting = false;
static int quickCraftButton = 0;
static int quickCraftType = 0;
static int quickCraftSlots[MENU_SLOT_MAX] = { 0 };
static int quickCraftCount = 0;
static int quickCraftRemainder = 0;
static bool skipNextRelease = false;
static bool doubleClick = false;
static int lastClickSlot = SLOT_NONE;
static double lastClickTime = 0.0;
static int lastClickButton = -1;
static Stack lastQuickMoved = { BLOCK_AIR, 0 };
static bool clickedOutside = false;

static PickerEntry picker[PICKER_MAX] = { 0 };
static int pickerCount = 0;
static BlockType searchOrder[PICKER_MAX] = { 0 };
static int searchOrderCount = 0;
static TextField searchField = { 0 };
static bool scrolling = false;
static bool ignoreTextInput = false;
static Stack savedHotbars[HOTBAR_SIZE][HOTBAR_SIZE] = { 0 };

//----------------------------------------------------------------------------------
// Stacks and slots
//----------------------------------------------------------------------------------
static bool ShiftDown(void)
{
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

static bool Creative(const Player *player)
{
    return player->gameMode == GAME_MODE_CREATIVE;
}

static Stack MakeStack(BlockType block, int count)
{
    Stack stack = { block, count };

    if ((block <= BLOCK_AIR) || (block >= BLOCK_COUNT) || (count <= 0)) stack = (Stack){ BLOCK_AIR, 0 };
    return stack;
}

static Stack EmptyStack(void)
{
    return MakeStack(BLOCK_AIR, 0);
}

static bool IsEmpty(Stack stack)
{
    return stack.block == BLOCK_AIR;
}

static int MaxStack(Stack stack)
{
    return GuiMaxStack(stack.block);
}

static Stack Carried(const Player *player)
{
    return MakeStack(player->cursorBlock, player->cursorCount);
}

static void SetCarried(Player *player, Stack stack)
{
    stack = MakeStack(stack.block, stack.count);
    player->cursorBlock = stack.block;
    player->cursorCount = stack.count;
}

static bool TabVisible(int tab)
{
    return (tabs[tab].kind != TAB_KIND_CATEGORY) || (tabs[tab].itemCount > 0);
}

static TabKind CurrentTabKind(const Player *player)
{
    return tabs[player->creativeTab].kind;
}

static int PickerRows(void)
{
    return (pickerCount + 8)/9 - 5;
}

static bool CanScroll(const Player *player)
{
    return (CurrentTabKind(player) != TAB_KIND_INVENTORY) && (pickerCount > 45);
}

static PickerEntry PickerAt(const Player *player, int cell)
{
    PickerEntry none = { BLOCK_AIR, 0, -1 };
    int offset = (int)((double)(player->creativeScroll*(float)PickerRows()) + 0.5);
    int index = 0;

    if (offset < 0) offset = 0;
    index = (cell/9 + offset)*9 + cell%9;
    if ((index < 0) || (index >= pickerCount)) return none;
    return picker[index];
}

static Stack ResultStack(const Player *player)
{
    const CraftingRecipe *recipe = CraftingGetRecipe(CraftingFindRecipe(player->craft, player->craftCount, player->craftSize));

    if (recipe == NULL) return EmptyStack();
    return MakeStack(recipe->result, recipe->count);
}

static Stack GetSlot(const Player *player, const MenuSlot *slot)
{
    switch (slot->kind)
    {
        case SLOT_RESULT: return ResultStack(player);
        case SLOT_CRAFT: return MakeStack(player->craft[slot->index], player->craftCount[slot->index]);
        case SLOT_STORAGE: return MakeStack(player->inventory.blocks[slot->index], player->inventory.quantities[slot->index]);
        case SLOT_HOTBAR: return MakeStack(player->hotbar[slot->index], player->hotbarCount[slot->index]);
        case SLOT_OFFHAND: return MakeStack(player->offhand, player->offhandCount);
        case SLOT_PICKER:
        {
            PickerEntry entry = PickerAt(player, slot->index);

            return MakeStack(entry.block, entry.count);
        }
        default: return EmptyStack();
    }
}

static void SetSlot(Player *player, const MenuSlot *slot, Stack stack)
{
    stack = MakeStack(stack.block, stack.count);
    switch (slot->kind)
    {
        case SLOT_CRAFT:
        {
            player->craft[slot->index] = stack.block;
            player->craftCount[slot->index] = stack.count;
        } break;
        case SLOT_STORAGE:
        {
            player->inventory.blocks[slot->index] = stack.block;
            player->inventory.quantities[slot->index] = stack.count;
        } break;
        case SLOT_HOTBAR:
        {
            player->hotbar[slot->index] = stack.block;
            player->hotbarCount[slot->index] = stack.count;
        } break;
        case SLOT_OFFHAND:
        {
            player->offhand = stack.block;
            player->offhandCount = stack.count;
        } break;
        default: break;
    }
}

// Armor slots only take armor, and there is no armor yet.
static bool SlotMayPlace(const MenuSlot *slot, Stack stack)
{
    if (IsEmpty(stack)) return false;
    return (slot->kind == SLOT_CRAFT) || (slot->kind == SLOT_STORAGE) || (slot->kind == SLOT_HOTBAR) || (slot->kind == SLOT_OFFHAND);
}

static bool SlotMayPickup(const MenuSlot *slot)
{
    return (slot->kind != SLOT_PICKER) && (slot->kind != SLOT_DESTROY);
}

static bool CanDragTo(const MenuSlot *slot)
{
    return (slot->kind != SLOT_RESULT) && (slot->kind != SLOT_PICKER) && (slot->kind != SLOT_DESTROY);
}

static bool CanPickAllFrom(const MenuSlot *slot)
{
    return (slot->kind != SLOT_RESULT) && (slot->kind != SLOT_PICKER) && (slot->kind != SLOT_DESTROY);
}

// Slots that belong to one container: the player's own slots, the grid, the result.
static int SlotContainer(const MenuSlot *slot)
{
    switch (slot->kind)
    {
        case SLOT_ARMOR:
        case SLOT_STORAGE:
        case SLOT_HOTBAR:
        case SLOT_OFFHAND: return 0;
        case SLOT_CRAFT: return 1;
        case SLOT_RESULT: return 2;
        default: return 3;
    }
}

static bool CanQuickReplace(Stack there, Stack stack)
{
    return IsEmpty(there) || (there.block == stack.block);
}

static Stack InventoryStack(const Player *player, int button)
{
    if (button == OFFHAND_BUTTON) return MakeStack(player->offhand, player->offhandCount);
    return MakeStack(player->hotbar[button], player->hotbarCount[button]);
}

static void SetInventoryStack(Player *player, int button, Stack stack)
{
    stack = MakeStack(stack.block, stack.count);
    if (button == OFFHAND_BUTTON)
    {
        player->offhand = stack.block;
        player->offhandCount = stack.count;
        return;
    }
    player->hotbar[button] = stack.block;
    player->hotbarCount[button] = stack.count;
}

static void ConsumeCraft(Player *player)
{
    CraftingConsume(player->craft, player->craftCount, player->craftSize);
}

//----------------------------------------------------------------------------------
// Layout
//----------------------------------------------------------------------------------
static void AddSlot(SlotKind kind, int index, int x, int y)
{
    if (slotCount >= MENU_SLOT_MAX) return;
    slots[slotCount] = (MenuSlot){ kind, index, x, y };
    slotCount++;
}

static void AddPlayerRows(int x, int storageY, int hotbarY)
{
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 9; col++) AddSlot(SLOT_STORAGE, row*9 + col, x + 18*col, storageY + 18*row);
    }
    for (int col = 0; col < 9; col++) AddSlot(SLOT_HOTBAR, col, x + 18*col, hotbarY);
}

static void BuildMenu(const Player *player)
{
    slotCount = 0;
    imageWidth = (screen == SCREEN_CREATIVE)? CREATIVE_WIDTH : SURVIVAL_WIDTH;
    imageHeight = (screen == SCREEN_CREATIVE)? CREATIVE_HEIGHT : SURVIVAL_HEIGHT;
    leftPos = (GuiWidth() - imageWidth)/2;
    topPos = (GuiHeight() - imageHeight)/2;

    if (screen == SCREEN_INVENTORY)
    {
        AddSlot(SLOT_RESULT, 0, 154, 28);
        for (int row = 0; row < 2; row++)
        {
            for (int col = 0; col < 2; col++) AddSlot(SLOT_CRAFT, row*2 + col, 98 + 18*col, 18 + 18*row);
        }
        for (int i = 0; i < 4; i++) AddSlot(SLOT_ARMOR, i, 8, 8 + 18*i);
        AddPlayerRows(8, 84, 142);
        AddSlot(SLOT_OFFHAND, 0, 77, 62);
    }
    else if (screen == SCREEN_CRAFTING)
    {
        AddSlot(SLOT_RESULT, 0, 124, 35);
        for (int row = 0; row < 3; row++)
        {
            for (int col = 0; col < 3; col++) AddSlot(SLOT_CRAFT, row*3 + col, 30 + 18*col, 17 + 18*row);
        }
        AddPlayerRows(8, 84, 142);
    }
    else if (CurrentTabKind(player) == TAB_KIND_INVENTORY)
    {
        AddSlot(SLOT_ARMOR, 0, 54, 6);
        AddSlot(SLOT_ARMOR, 1, 54, 33);
        AddSlot(SLOT_ARMOR, 2, 108, 6);
        AddSlot(SLOT_ARMOR, 3, 108, 33);
        AddPlayerRows(9, 54, 112);
        AddSlot(SLOT_OFFHAND, 0, 35, 20);
        AddSlot(SLOT_DESTROY, 0, 173, 112);
    }
    else
    {
        for (int cell = 0; cell < 45; cell++) AddSlot(SLOT_PICKER, cell, 9 + 18*(cell%9), 18 + 18*(cell/9));
        for (int col = 0; col < 9; col++) AddSlot(SLOT_HOTBAR, col, 9 + 18*col, 112);
    }
}

static bool SlotHovered(int index, int mx, int my)
{
    int x = leftPos + slots[index].x;
    int y = topPos + slots[index].y;

    return (mx >= x - 1) && (mx < x + 17) && (my >= y - 1) && (my < y + 17);
}

static int SlotAt(int mx, int my)
{
    for (int i = 0; i < slotCount; i++)
    {
        if (SlotHovered(i, mx, my)) return i;
    }
    return SLOT_NONE;
}

static int TabX(int tab)
{
    if (tabs[tab].column >= 5) return CREATIVE_WIDTH - 27*(7 - tabs[tab].column) + 1;
    return 27*tabs[tab].column;
}

static int TabY(int tab)
{
    return tabs[tab].top? -32 : CREATIVE_HEIGHT;
}

static bool TabClicked(int tab, int mx, int my)
{
    int x = mx - leftPos;
    int y = my - topPos;

    return (x >= TabX(tab)) && (x <= TabX(tab) + 26) && (y >= TabY(tab)) && (y <= TabY(tab) + 32);
}

static bool TabHovered(int tab, int mx, int my)
{
    int x = mx - leftPos;
    int y = my - topPos;

    return (x >= TabX(tab) + 2) && (x < TabX(tab) + 25) && (y >= TabY(tab) + 2) && (y < TabY(tab) + 31);
}

static bool ClickedOutside(const Player *player, int mx, int my)
{
    bool outside = (mx < leftPos) || (my < topPos) || (mx >= leftPos + imageWidth) || (my >= topPos + imageHeight);

    if (screen == SCREEN_CREATIVE) outside = outside && !TabClicked(player->creativeTab, mx, my);
    return outside;
}

static bool InsideScrollbar(int mx, int my)
{
    return (mx >= leftPos + 175) && (my >= topPos + 18) && (mx < leftPos + 189) && (my < topPos + 130);
}

//----------------------------------------------------------------------------------
// Creative tabs and search
//----------------------------------------------------------------------------------
static bool TabContains(int tab, BlockType block)
{
    for (int i = 0; i < tabs[tab].itemCount; i++)
    {
        if (tabs[tab].items[i] == block) return true;
    }
    return false;
}

static void BuildSearchOrder(void)
{
    if (searchOrderCount > 0) return;
    for (int tab = 0; tab < TAB_COUNT; tab++)
    {
        if (tabs[tab].kind != TAB_KIND_CATEGORY) continue;
        for (int i = 0; i < tabs[tab].itemCount; i++)
        {
            BlockType block = tabs[tab].items[i];
            bool seen = false;

            for (int k = 0; (k < searchOrderCount) && !seen; k++) seen = (searchOrder[k] == block);
            if (!seen && (searchOrderCount < PICKER_MAX)) searchOrder[searchOrderCount++] = block;
        }
    }
}

static void LowerCopy(const char *text, char *out, int size)
{
    int i = 0;

    for (i = 0; (text[i] != '\0') && (i < size - 1); i++) out[i] = (char)tolower((unsigned char)text[i]);
    out[i] = '\0';
}

static void TrimCopy(const char *start, const char *end, char *out, int size)
{
    int length = 0;

    while ((start < end) && (*start == ' ')) start++;
    while ((end > start) && (end[-1] == ' ')) end--;
    length = (int)(end - start);
    if (length > size - 1) length = size - 1;
    memcpy(out, start, (size_t)length);
    out[length] = '\0';
}

// A query with a colon matches the id's namespace and path separately.
// Otherwise it may appear anywhere in the name or the id.
static bool ItemMatches(BlockType block, const char *query)
{
    char name[64] = { 0 };
    const char *id = GetBlockId(block);
    const char *colon = strchr(query, ':');

    if (colon != NULL)
    {
        char space[TEXT_FIELD_CAPACITY] = { 0 };
        char path[TEXT_FIELD_CAPACITY] = { 0 };

        TrimCopy(query, colon, space, (int)sizeof(space));
        TrimCopy(colon + 1, query + strlen(query), path, (int)sizeof(path));
        return (strstr("minecraft", space) != NULL) && (strstr(id, path) != NULL);
    }
    LowerCopy(GetBlockName(block), name, (int)sizeof(name));
    return (strstr(name, query) != NULL) || (strstr(id, query) != NULL);
}

static void AddPicker(BlockType block, int count, int lockedRow)
{
    if (pickerCount >= PICKER_MAX) return;
    picker[pickerCount] = (PickerEntry){ block, count, lockedRow };
    pickerCount++;
}

static void RefreshSearch(Player *player)
{
    char query[TEXT_FIELD_CAPACITY] = { 0 };

    LowerCopy(searchField.value, query, (int)sizeof(query));
    BuildSearchOrder();
    pickerCount = 0;
    if (query[0] != '#')
    {
        for (int i = 0; i < searchOrderCount; i++)
        {
            if ((query[0] == '\0') || ItemMatches(searchOrder[i], query)) AddPicker(searchOrder[i], 1, -1);
        }
    }
    player->creativeScroll = 0.0f;
}

static bool HotbarRowEmpty(int row)
{
    for (int col = 0; col < HOTBAR_SIZE; col++)
    {
        if (!IsEmpty(savedHotbars[row][col])) return false;
    }
    return true;
}

static void SelectTab(Player *player, int tab, bool reopening)
{
    int previous = reopening? -1 : player->creativeTab;

    player->creativeTab = tab;
    quickCrafting = false;
    quickCraftCount = 0;
    lastClickSlot = SLOT_NONE;
    pickerCount = 0;
    if (tabs[tab].kind == TAB_KIND_HOTBAR)
    {
        for (int row = 0; row < HOTBAR_SIZE; row++)
        {
            bool empty = HotbarRowEmpty(row);

            for (int col = 0; col < HOTBAR_SIZE; col++)
            {
                if (empty) AddPicker(BLOCK_AIR, 0, (col == row)? row : -1);
                else AddPicker(savedHotbars[row][col].block, savedHotbars[row][col].count, -1);
            }
        }
    }
    else if (tabs[tab].kind == TAB_KIND_CATEGORY)
    {
        for (int i = 0; i < tabs[tab].itemCount; i++) AddPicker(tabs[tab].items[i], 1, -1);
    }
    if (tabs[tab].kind == TAB_KIND_SEARCH)
    {
        TextFieldSetFocus(&searchField, true);
        if (previous != tab) TextFieldSetValue(&searchField, "");
        RefreshSearch(player);
    }
    else
    {
        TextFieldSetFocus(&searchField, false);
        TextFieldSetValue(&searchField, "");
    }
    player->creativeScroll = 0.0f;
    player->searchFocused = searchField.focused? 1 : 0;
    BuildMenu(player);
}

//----------------------------------------------------------------------------------
// Clicks
//----------------------------------------------------------------------------------
static void ClickPickup(Player *player, int index, int button)
{
    const MenuSlot *slot = &slots[index];
    Stack carried = Carried(player);
    Stack item = GetSlot(player, slot);
    bool primary = (button == 0);

    // The output always comes out whole: into an empty hand, or onto the
    // same item when all of it fits.
    if (slot->kind == SLOT_RESULT)
    {
        if (IsEmpty(item)) return;
        if (IsEmpty(carried))
        {
            SetCarried(player, item);
            ConsumeCraft(player);
        }
        else if ((carried.block == item.block) && (carried.count + item.count <= MaxStack(carried)))
        {
            carried.count += item.count;
            SetCarried(player, carried);
            ConsumeCraft(player);
        }
        return;
    }
    if (IsEmpty(item))
    {
        if (!IsEmpty(carried) && SlotMayPlace(slot, carried))
        {
            int amount = primary? carried.count : 1;

            if (amount > MaxStack(carried)) amount = MaxStack(carried);
            SetSlot(player, slot, MakeStack(carried.block, amount));
            carried.count -= amount;
            SetCarried(player, carried);
        }
        return;
    }
    if (!SlotMayPickup(slot)) return;
    if (IsEmpty(carried))
    {
        int amount = primary? item.count : (item.count + 1)/2;

        SetCarried(player, MakeStack(item.block, amount));
        item.count -= amount;
        SetSlot(player, slot, item);
        return;
    }
    if (!SlotMayPlace(slot, carried)) return;
    if (carried.block == item.block)
    {
        int amount = primary? carried.count : 1;
        int room = MaxStack(item) - item.count;

        if (amount > room) amount = room;
        if (amount <= 0) return;
        item.count += amount;
        carried.count -= amount;
        SetSlot(player, slot, item);
        SetCarried(player, carried);
    }
    else if (carried.count <= MaxStack(carried))
    {
        SetSlot(player, slot, carried);
        SetCarried(player, item);
    }
}

static int AppendKind(int *out, int count, SlotKind kind, bool reverse)
{
    for (int k = 0; k < slotCount; k++)
    {
        int i = reverse? slotCount - 1 - k : k;

        if (slots[i].kind == kind) out[count++] = i;
    }
    return count;
}

// First tops up stacks of the same item in target order, then puts what is
// left into the first empty slot that takes it.
static bool MoveStackTo(Player *player, Stack *stack, const int *targets, int targetCount)
{
    bool moved = false;

    for (int i = 0; (i < targetCount) && !IsEmpty(*stack); i++)
    {
        const MenuSlot *slot = &slots[targets[i]];
        Stack there = GetSlot(player, slot);
        int room = MaxStack(*stack) - there.count;

        if ((there.block != stack->block) || (room <= 0)) continue;
        if (room > stack->count) room = stack->count;
        there.count += room;
        SetSlot(player, slot, there);
        *stack = MakeStack(stack->block, stack->count - room);
        moved = true;
    }
    for (int i = 0; (i < targetCount) && !IsEmpty(*stack); i++)
    {
        const MenuSlot *slot = &slots[targets[i]];

        if (!IsEmpty(GetSlot(player, slot)) || !SlotMayPlace(slot, *stack)) continue;
        SetSlot(player, slot, *stack);
        *stack = EmptyStack();
        moved = true;
    }
    return moved;
}

static int RoomFor(const Player *player, Stack stack, const int *targets, int targetCount)
{
    int room = 0;
    bool emptySlot = false;

    for (int i = 0; i < targetCount; i++)
    {
        Stack there = GetSlot(player, &slots[targets[i]]);

        if (IsEmpty(there)) emptySlot = true;
        else if (there.block == stack.block) room += MaxStack(stack) - there.count;
    }
    return room + (emptySlot? MaxStack(stack) : 0);
}

// One shift-click step. Returns false once nothing more moves.
static bool QuickMoveOnce(Player *player, int index)
{
    const MenuSlot *slot = &slots[index];
    Stack item = GetSlot(player, slot);
    int before = item.count;
    int targets[MENU_SLOT_MAX] = { 0 };
    int count = 0;

    if (IsEmpty(item)) return false;
    if (slot->kind == SLOT_RESULT)
    {
        // Crafted items fill the hotbar from the right first. Nothing is
        // crafted unless the whole output fits, since there is nowhere to drop the rest.
        count = AppendKind(targets, count, SLOT_HOTBAR, true);
        count = AppendKind(targets, count, SLOT_STORAGE, true);
        if (RoomFor(player, item, targets, count) < item.count) return false;
        if (!MoveStackTo(player, &item, targets, count)) return false;
        ConsumeCraft(player);
        return true;
    }
    if ((screen == SCREEN_CRAFTING) && ((slot->kind == SLOT_STORAGE) || (slot->kind == SLOT_HOTBAR)))
    {
        count = AppendKind(targets, 0, SLOT_CRAFT, false);
        if (!MoveStackTo(player, &item, targets, count))
        {
            count = AppendKind(targets, 0, (slot->kind == SLOT_STORAGE)? SLOT_HOTBAR : SLOT_STORAGE, false);
            MoveStackTo(player, &item, targets, count);
        }
    }
    else if (slot->kind == SLOT_STORAGE)
    {
        count = AppendKind(targets, 0, SLOT_HOTBAR, false);
        MoveStackTo(player, &item, targets, count);
    }
    else if (slot->kind == SLOT_HOTBAR)
    {
        count = AppendKind(targets, 0, SLOT_STORAGE, false);
        MoveStackTo(player, &item, targets, count);
    }
    else
    {
        count = AppendKind(targets, 0, SLOT_STORAGE, false);
        count = AppendKind(targets, count, SLOT_HOTBAR, false);
        MoveStackTo(player, &item, targets, count);
    }
    SetSlot(player, slot, item);
    return item.count != before;
}

static void QuickMove(Player *player, int index)
{
    BlockType block = GetSlot(player, &slots[index]).block;

    for (int step = 0; step < 4*INVENTORY_SIZE; step++)
    {
        if (!QuickMoveOnce(player, index)) break;
        if (GetSlot(player, &slots[index]).block != block) break;
    }
}

static void ClickSwap(Player *player, int index, int button)
{
    const MenuSlot *slot = &slots[index];
    Stack other = InventoryStack(player, button);
    Stack item = GetSlot(player, slot);

    if (IsEmpty(other) && IsEmpty(item)) return;
    if (IsEmpty(other))
    {
        if (!SlotMayPickup(slot)) return;
        SetInventoryStack(player, button, item);
        if (slot->kind == SLOT_RESULT) ConsumeCraft(player);
        else SetSlot(player, slot, EmptyStack());
        return;
    }
    if (!SlotMayPlace(slot, other)) return;
    if (IsEmpty(item))
    {
        SetInventoryStack(player, button, EmptyStack());
        SetSlot(player, slot, other);
        return;
    }
    SetInventoryStack(player, button, item);
    SetSlot(player, slot, other);
}

static void ClickClone(Player *player, int index)
{
    Stack item = GetSlot(player, &slots[index]);

    if (!Creative(player) || !IsEmpty(Carried(player)) || IsEmpty(item)) return;
    SetCarried(player, MakeStack(item.block, MaxStack(item)));
}

// A double-click with something in hand pulls in the same item from every
// slot, partial stacks first.
static void ClickPickupAll(Player *player, int index, int button)
{
    Stack carried = Carried(player);
    Stack clicked = GetSlot(player, &slots[index]);
    int max = MaxStack(carried);

    if (IsEmpty(carried) || (!IsEmpty(clicked) && SlotMayPickup(&slots[index]))) return;
    for (int pass = 0; pass < 2; pass++)
    {
        for (int k = 0; (k < slotCount) && (carried.count < max); k++)
        {
            int i = (button == 0)? k : slotCount - 1 - k;
            Stack there = GetSlot(player, &slots[i]);
            int take = 0;

            if ((there.block != carried.block) || !CanPickAllFrom(&slots[i])) continue;
            if ((pass == 0) && (there.count == MaxStack(there))) continue;
            take = there.count;
            if (take > max - carried.count) take = max - carried.count;
            there.count -= take;
            carried.count += take;
            SetSlot(player, &slots[i], there);
        }
    }
    SetCarried(player, carried);
}

static void MenuClicked(Player *player, int target, int button, ClickType type)
{
    if (target < 0) return;
    switch (type)
    {
        case CLICK_PICKUP: ClickPickup(player, target, button); break;
        case CLICK_QUICK_MOVE: QuickMove(player, target); break;
        case CLICK_SWAP: ClickSwap(player, target, button); break;
        case CLICK_CLONE: ClickClone(player, target); break;
        case CLICK_PICKUP_ALL: ClickPickupAll(player, target, button); break;
        default: break;
    }
}

static void ClearInventory(Player *player)
{
    for (int i = 0; i < HOTBAR_SIZE; i++) SetInventoryStack(player, i, EmptyStack());
    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        player->inventory.blocks[i] = BLOCK_AIR;
        player->inventory.quantities[i] = 0;
    }
    for (int i = 0; i < 9; i++)
    {
        player->craft[i] = BLOCK_AIR;
        player->craftCount[i] = 0;
    }
    SetInventoryStack(player, OFFHAND_BUTTON, EmptyStack());
}

// Picker cells hand out copies: one item, a full stack with Shift, and a
// click with another item in hand throws that item away.
static void PickerClicked(Player *player, const MenuSlot *slot, int button, ClickType type)
{
    Stack carried = Carried(player);
    Stack item = GetSlot(player, slot);
    bool quick = (type == CLICK_QUICK_MOVE);

    if (type == CLICK_SWAP)
    {
        if (!IsEmpty(item)) SetInventoryStack(player, button, MakeStack(item.block, MaxStack(item)));
        return;
    }
    if (type == CLICK_CLONE)
    {
        if (IsEmpty(carried) && !IsEmpty(item)) SetCarried(player, MakeStack(item.block, MaxStack(item)));
        return;
    }
    if (type == CLICK_THROW) return;
    if (!IsEmpty(carried) && !IsEmpty(item) && (carried.block == item.block))
    {
        if (button == 0)
        {
            if (quick) carried.count = MaxStack(carried);
            else if (carried.count < MaxStack(carried)) carried.count++;
        }
        else carried.count--;
    }
    else if (!IsEmpty(item) && IsEmpty(carried))
    {
        carried = item;
        if (quick) carried.count = MaxStack(carried);
    }
    else if (button == 0) carried = EmptyStack();
    else carried.count--;
    SetCarried(player, carried);
}

static void CreativeClicked(Player *player, int target, int button, ClickType type)
{
    TabKind kind = CurrentTabKind(player);
    const MenuSlot *slot = (target >= 0)? &slots[target] : NULL;
    Stack carried = Carried(player);

    if ((target == SLOT_OUTSIDE) && (type == CLICK_PICKUP)) type = CLICK_THROW;
    if ((slot == NULL) && (kind != TAB_KIND_INVENTORY))
    {
        if (!IsEmpty(carried) && clickedOutside)
        {
            if (button == 0) SetCarried(player, EmptyStack());
            else if (button == 1) SetCarried(player, MakeStack(carried.block, carried.count - 1));
        }
        return;
    }
    if ((slot != NULL) && (slot->kind == SLOT_PICKER) && (PickerAt(player, slot->index).lockedRow >= 0)) return;
    if ((slot != NULL) && (slot->kind == SLOT_DESTROY) && (type == CLICK_QUICK_MOVE))
    {
        ClearInventory(player);
        return;
    }
    if (kind == TAB_KIND_INVENTORY)
    {
        if ((slot != NULL) && (slot->kind == SLOT_DESTROY)) SetCarried(player, EmptyStack());
        else if ((type == CLICK_THROW) && (slot != NULL) && !IsEmpty(GetSlot(player, slot)))
        {
            Stack item = GetSlot(player, slot);

            SetSlot(player, slot, MakeStack(item.block, (button == 0)? item.count - 1 : 0));
        }
        else if (type == CLICK_THROW) SetCarried(player, EmptyStack());
        else MenuClicked(player, target, button, type);
        return;
    }
    if (slot->kind == SLOT_PICKER)
    {
        PickerClicked(player, slot, button, type);
        return;
    }
    // Shift-clicking the hotbar under the item grid clears that slot.
    if (type == CLICK_QUICK_MOVE)
    {
        SetSlot(player, slot, EmptyStack());
        return;
    }
    MenuClicked(player, target, button, type);
}

static void SlotClicked(Player *player, int target, int button, ClickType type)
{
    if (screen == SCREEN_CREATIVE) CreativeClicked(player, target, button, type);
    else if ((target == SLOT_OUTSIDE) && Creative(player)) SetCarried(player, EmptyStack());
    else MenuClicked(player, target, button, type);
}

//----------------------------------------------------------------------------------
// Dragging a stack across slots
//----------------------------------------------------------------------------------
static bool InQuickCraft(int index)
{
    for (int i = 0; i < quickCraftCount; i++)
    {
        if (quickCraftSlots[i] == index) return true;
    }
    return false;
}

// What a dragged-over slot ends up holding before the stack size cap.
static int QuickCraftTarget(Stack carried, int existing)
{
    int share = MaxStack(carried);

    if (quickCraftType == 0) share = carried.count/quickCraftCount;
    else if (quickCraftType == 1) share = 1;
    return share + existing;
}

static void RecalculateQuickCraft(const Player *player)
{
    Stack carried = Carried(player);

    quickCraftRemainder = carried.count;
    if (IsEmpty(carried)) return;
    for (int i = 0; i < quickCraftCount; i++)
    {
        Stack there = GetSlot(player, &slots[quickCraftSlots[i]]);
        int target = QuickCraftTarget(carried, there.count);

        if (target > MaxStack(carried)) target = MaxStack(carried);
        quickCraftRemainder -= target - there.count;
    }
}

static void FinishQuickCraft(Player *player)
{
    Stack carried = Carried(player);
    int remaining = carried.count;

    if (quickCraftCount == 1)
    {
        int index = quickCraftSlots[0];

        quickCraftCount = 0;
        SlotClicked(player, index, (quickCraftType == 0)? 0 : 1, CLICK_PICKUP);
        return;
    }
    for (int i = 0; i < quickCraftCount; i++)
    {
        const MenuSlot *slot = &slots[quickCraftSlots[i]];
        Stack there = GetSlot(player, slot);
        int target = 0;

        if (!CanQuickReplace(there, carried) || !SlotMayPlace(slot, carried) || !CanDragTo(slot)) continue;
        if ((quickCraftType != 2) && (carried.count < quickCraftCount)) continue;
        target = QuickCraftTarget(carried, there.count);
        if (target > MaxStack(carried)) target = MaxStack(carried);
        remaining -= target - there.count;
        SetSlot(player, slot, MakeStack(carried.block, target));
    }
    SetCarried(player, MakeStack(carried.block, remaining));
    quickCraftCount = 0;
}

//----------------------------------------------------------------------------------
// Mouse and keys
//----------------------------------------------------------------------------------
static void ResetMouseState(void)
{
    quickCrafting = false;
    quickCraftCount = 0;
    quickCraftRemainder = 0;
    skipNextRelease = false;
    doubleClick = false;
    lastClickSlot = SLOT_NONE;
    lastClickTime = 0.0;
    lastClickButton = -1;
    lastQuickMoved = EmptyStack();
    clickedOutside = false;
    scrolling = false;
    ignoreTextInput = false;
}

static void ScrollTo(Player *player, float position)
{
    if (position < 0.0f) position = 0.0f;
    if (position > 1.0f) position = 1.0f;
    player->creativeScroll = position;
}

// Tabs, the scroller, and the search field take a press before the slots do.
static bool CreativePressed(Player *player, int mx, int my, int button)
{
    if (button == 0)
    {
        for (int tab = 0; tab < TAB_COUNT; tab++)
        {
            if (TabVisible(tab) && TabClicked(tab, mx, my)) return true;
        }
        if ((CurrentTabKind(player) != TAB_KIND_INVENTORY) && InsideScrollbar(mx, my))
        {
            scrolling = CanScroll(player);
            return true;
        }
    }
    if (CurrentTabKind(player) == TAB_KIND_SEARCH)
    {
        if (TextFieldMouseClicked(&searchField, leftPos + 82, topPos + 6, 9, mx, my, button, false)) return true;
    }
    return false;
}

static void MousePressed(Player *player, int mx, int my, int button)
{
    int slot = SlotAt(mx, my);
    double now = GetTime();
    bool clone = (button == 2) && Creative(player);
    int target = SLOT_NONE;

    if ((screen == SCREEN_CREATIVE) && CreativePressed(player, mx, my, button)) return;
    doubleClick = (lastClickSlot == slot) && (now - lastClickTime < DOUBLE_CLICK_SECONDS) && (lastClickButton == button);
    skipNextRelease = false;
    clickedOutside = ClickedOutside(player, mx, my);
    target = clickedOutside? SLOT_OUTSIDE : slot;
    if (((button == 0) || (button == 1) || clone) && (target != SLOT_NONE) && !quickCrafting)
    {
        if (IsEmpty(Carried(player)))
        {
            if (clone) SlotClicked(player, target, button, CLICK_CLONE);
            else if ((target != SLOT_OUTSIDE) && ShiftDown())
            {
                // The second press of a Shift double-click lands on the slot the
                // first press just emptied, so it keeps the item that moved.
                Stack item = GetSlot(player, &slots[slot]);

                if (!IsEmpty(item) || !doubleClick) lastQuickMoved = item;
                SlotClicked(player, target, button, CLICK_QUICK_MOVE);
            }
            else SlotClicked(player, target, button, (target == SLOT_OUTSIDE)? CLICK_THROW : CLICK_PICKUP);
            skipNextRelease = true;
        }
        else
        {
            quickCrafting = true;
            quickCraftButton = button;
            quickCraftCount = 0;
            quickCraftType = (button == 0)? 0 : ((button == 1)? 1 : 2);
        }
    }
    lastClickSlot = slot;
    lastClickTime = now;
    lastClickButton = button;
}

static void MouseDragged(Player *player, int mx, int my, float exactY)
{
    int slot = SlotAt(mx, my);
    Stack carried = Carried(player);

    if (scrolling)
    {
        ScrollTo(player, (exactY - (float)(topPos + 18) - 7.5f)/(112.0f - 15.0f));
        return;
    }
    if (!quickCrafting || (slot < 0) || IsEmpty(carried) || InQuickCraft(slot)) return;
    if ((carried.count <= quickCraftCount) && (quickCraftType != 2)) return;
    if (!CanQuickReplace(GetSlot(player, &slots[slot]), carried) || !SlotMayPlace(&slots[slot], carried) || !CanDragTo(&slots[slot])) return;
    quickCraftSlots[quickCraftCount] = slot;
    quickCraftCount++;
    RecalculateQuickCraft(player);
}

static void MouseReleased(Player *player, int mx, int my, int button)
{
    int slot = SlotAt(mx, my);
    int target = SLOT_NONE;

    if ((screen == SCREEN_CREATIVE) && (button == 0))
    {
        scrolling = false;
        for (int tab = 0; tab < TAB_COUNT; tab++)
        {
            if (TabVisible(tab) && TabClicked(tab, mx, my))
            {
                SelectTab(player, tab, false);
                return;
            }
        }
    }
    clickedOutside = ClickedOutside(player, mx, my);
    target = clickedOutside? SLOT_OUTSIDE : slot;
    if (doubleClick && (slot >= 0) && (button == 0) && CanPickAllFrom(&slots[slot]))
    {
        if (ShiftDown())
        {
            if (!IsEmpty(lastQuickMoved))
            {
                int container = SlotContainer(&slots[slot]);

                for (int i = 0; i < slotCount; i++)
                {
                    Stack there = GetSlot(player, &slots[i]);

                    if ((there.block != lastQuickMoved.block) || !SlotMayPickup(&slots[i])) continue;
                    if (SlotContainer(&slots[i]) != container) continue;
                    SlotClicked(player, i, button, CLICK_QUICK_MOVE);
                }
            }
        }
        else SlotClicked(player, slot, button, CLICK_PICKUP_ALL);
        doubleClick = false;
        lastClickTime = 0.0;
    }
    else
    {
        if (quickCrafting && (quickCraftButton != button))
        {
            quickCrafting = false;
            quickCraftCount = 0;
            skipNextRelease = true;
            return;
        }
        if (skipNextRelease)
        {
            skipNextRelease = false;
            return;
        }
        if (quickCrafting && (quickCraftCount > 0)) FinishQuickCraft(player);
        else if (!IsEmpty(Carried(player)))
        {
            if ((button == 2) && Creative(player)) SlotClicked(player, target, button, CLICK_CLONE);
            else
            {
                bool quick = (target != SLOT_OUTSIDE) && ShiftDown();

                if (quick) lastQuickMoved = (slot >= 0)? GetSlot(player, &slots[slot]) : EmptyStack();
                SlotClicked(player, target, button, quick? CLICK_QUICK_MOVE : CLICK_PICKUP);
            }
        }
    }
    if (IsEmpty(Carried(player))) lastClickTime = 0.0;
    quickCrafting = false;
    quickCraftCount = 0;
}

static bool HotbarKey(Player *player, int key, int hovered)
{
    if (!IsEmpty(Carried(player)) || (hovered < 0)) return false;
    if (key == KEY_F)
    {
        SlotClicked(player, hovered, OFFHAND_BUTTON, CLICK_SWAP);
        return true;
    }
    if ((key >= KEY_ONE) && (key <= KEY_NINE))
    {
        SlotClicked(player, hovered, key - KEY_ONE, CLICK_SWAP);
        return true;
    }
    return false;
}

static void ContainerKeyPressed(Player *player, int key, int hovered)
{
    if (key == KEY_E)
    {
        InventoryClose(player);
        return;
    }
    HotbarKey(player, key, hovered);
}

static void KeyPressed(Player *player, int key, int hovered)
{
    char before[TEXT_FIELD_CAPACITY] = { 0 };
    bool usable = false;
    bool numeric = (key >= KEY_ZERO) && (key <= KEY_NINE);

    ignoreTextInput = false;
    if (screen != SCREEN_CREATIVE)
    {
        ContainerKeyPressed(player, key, hovered);
        return;
    }
    if (CurrentTabKind(player) != TAB_KIND_SEARCH)
    {
        if (key == KEY_T)
        {
            ignoreTextInput = true;
            SelectTab(player, TAB_SEARCH, false);
            return;
        }
        ContainerKeyPressed(player, key, hovered);
        return;
    }
    // In the search tab number keys still swap a hovered item into the
    // hotbar. Everything else goes to the search field, which keeps every
    // key but Escape while it has focus.
    usable = (hovered < 0) || (slots[hovered].kind != SLOT_PICKER) || !IsEmpty(GetSlot(player, &slots[hovered]));
    if (usable && numeric && HotbarKey(player, key, hovered))
    {
        ignoreTextInput = true;
        return;
    }
    memcpy(before, searchField.value, sizeof(before));
    if (TextFieldKeyPressed(&searchField, key))
    {
        if (strcmp(before, searchField.value) != 0) RefreshSearch(player);
        return;
    }
    if (searchField.focused && (key != KEY_ESCAPE)) return;
    ContainerKeyPressed(player, key, hovered);
}

static void CharTyped(Player *player, int codepoint)
{
    char before[TEXT_FIELD_CAPACITY] = { 0 };

    if ((screen != SCREEN_CREATIVE) || ignoreTextInput || (CurrentTabKind(player) != TAB_KIND_SEARCH)) return;
    memcpy(before, searchField.value, sizeof(before));
    if (TextFieldCharTyped(&searchField, codepoint) && (strcmp(before, searchField.value) != 0)) RefreshSearch(player);
}

static void MouseScrolled(Player *player, float wheel)
{
    int rows = PickerRows();

    if ((screen != SCREEN_CREATIVE) || !CanScroll(player) || (rows <= 0)) return;
    ScrollTo(player, player->creativeScroll - wheel/(float)rows);
}

//----------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------
void InventoryOpen(Player *player, int craftSize)
{
    player->craftSize = (craftSize == 3)? 3 : 2;
    if (craftSize == 3) screen = SCREEN_CRAFTING;
    else screen = (player->gameMode == GAME_MODE_CREATIVE)? SCREEN_CREATIVE : SCREEN_INVENTORY;
    for (int i = 0; i < 9; i++)
    {
        player->craft[i] = BLOCK_AIR;
        player->craftCount[i] = 0;
    }
    player->inventoryOpen = true;
    ResetMouseState();
    if (screen == SCREEN_CREATIVE)
    {
        if (searchField.maxLength == 0) TextFieldInit(&searchField, 50, 80);
        if ((player->creativeTab < 0) || (player->creativeTab >= TAB_COUNT) || !TabVisible(player->creativeTab)) player->creativeTab = TAB_BUILDING;
        SelectTab(player, player->creativeTab, true);
    }
    BuildMenu(player);
    EnableCursor();
    SetMousePosition(GetScreenWidth()/2, GetScreenHeight()/2);
}

// Survival puts the held stack and the grid back into the inventory. The
// creative hand only ever held copies.
void InventoryClose(Player *player)
{
    Stack carried = Carried(player);

    if (!player->inventoryOpen) return;
    if ((screen != SCREEN_CREATIVE) && !IsEmpty(carried)) StowItem(player, carried.block, carried.count);
    SetCarried(player, EmptyStack());
    for (int i = 0; i < 9; i++)
    {
        if (player->craftCount[i] > 0) StowItem(player, player->craft[i], player->craftCount[i]);
        player->craft[i] = BLOCK_AIR;
        player->craftCount[i] = 0;
    }
    player->inventoryOpen = false;
    player->searchFocused = 0;
    TextFieldSetFocus(&searchField, false);
    ResetMouseState();
    DisableCursor();
    SelectHotbarSlot(player, player->hotbarSlot);
}

void InventorySaveHotbar(Player *player, int row)
{
    if ((row < 0) || (row >= HOTBAR_SIZE)) return;
    for (int col = 0; col < HOTBAR_SIZE; col++) savedHotbars[row][col] = MakeStack(player->hotbar[col], player->hotbarCount[col]);
    HudActionBar(TextFormat("Saved hotbar (restore with X+%d)", row + 1));
}

void InventoryLoadHotbar(Player *player, int row)
{
    if ((row < 0) || (row >= HOTBAR_SIZE)) return;
    for (int col = 0; col < HOTBAR_SIZE; col++) SetInventoryStack(player, col, savedHotbars[row][col]);
    SelectHotbarSlot(player, player->hotbarSlot);
}

void InventoryHandleInput(Player *player)
{
    static const int repeatKeys[] = { KEY_BACKSPACE, KEY_DELETE, KEY_LEFT, KEY_RIGHT };
    Vector2 mouse = { 0 };
    Vector2 delta = GetMouseDelta();
    int mx = 0;
    int my = 0;
    int hovered = SLOT_NONE;
    int key = 0;
    int codepoint = 0;
    float wheel = 0.0f;

    if (!player->inventoryOpen) return;
    BuildMenu(player);
    mouse = GuiMouse();
    mx = (int)mouse.x;
    my = (int)mouse.y;
    for (int button = 0; (button < 3) && player->inventoryOpen; button++)
    {
        if (IsMouseButtonPressed(button)) MousePressed(player, mx, my, button);
    }
    if (((delta.x != 0.0f) || (delta.y != 0.0f)) && (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)))
    {
        MouseDragged(player, mx, my, GetMousePosition().y/(float)GuiScale());
    }
    for (int button = 0; (button < 3) && player->inventoryOpen; button++)
    {
        if (IsMouseButtonReleased(button)) MouseReleased(player, mx, my, button);
    }
    hovered = SlotAt(mx, my);
    while (player->inventoryOpen && ((key = GetKeyPressed()) != 0)) KeyPressed(player, key, hovered);
    for (int i = 0; (i < LIST_COUNT(repeatKeys)) && player->inventoryOpen; i++)
    {
        if (IsKeyPressedRepeat(repeatKeys[i])) KeyPressed(player, repeatKeys[i], hovered);
    }
    while (player->inventoryOpen && ((codepoint = GetCharPressed()) != 0)) CharTyped(player, codepoint);
    wheel = GetMouseWheelMove();
    if (player->inventoryOpen && (wheel != 0.0f)) MouseScrolled(player, wheel);
    if (player->inventoryOpen && quickCrafting) RecalculateQuickCraft(player);
    player->searchFocused = (player->inventoryOpen && (screen == SCREEN_CREATIVE) && searchField.focused)? 1 : 0;
    SelectHotbarSlot(player, player->hotbarSlot);
}

//----------------------------------------------------------------------------------
// Drawing
//----------------------------------------------------------------------------------
static void DrawStack(Stack stack, int x, int y, const char *countText, Color countColor)
{
    if (IsEmpty(stack)) return;
    GuiDrawItem(stack.block, x, y);
    if (countText != NULL) GuiDrawItemCountText(countText, x, y, countColor);
    else GuiDrawItemCount(stack.count, x, y);
}

static void DrawEmptyIcon(const MenuSlot *slot, int x, int y)
{
    static const GuiSprite armorIcons[4] = {
        GUI_SPRITE_EMPTY_HELMET, GUI_SPRITE_EMPTY_CHESTPLATE, GUI_SPRITE_EMPTY_LEGGINGS, GUI_SPRITE_EMPTY_BOOTS
    };

    if (slot->kind == SLOT_ARMOR) GuiDrawSprite(armorIcons[slot->index], x, y);
    else if (slot->kind == SLOT_OFFHAND) GuiDrawSprite(GUI_SPRITE_EMPTY_SHIELD, x, y);
}

static void DrawSlot(const Player *player, int index)
{
    const MenuSlot *slot = &slots[index];
    int x = leftPos + slot->x;
    int y = topPos + slot->y;
    Stack stack = GetSlot(player, slot);
    Stack carried = Carried(player);
    const char *countText = NULL;

    if (slot->kind == SLOT_PICKER)
    {
        PickerEntry entry = PickerAt(player, slot->index);

        if (entry.lockedRow >= 0)
        {
            GuiDrawItemTexture("paper", x, y);
            return;
        }
    }
    if (quickCrafting && InQuickCraft(index) && !IsEmpty(carried))
    {
        int target = 0;

        // Dragging over a single slot shows it empty until the button is let go.
        if (quickCraftCount == 1) return;
        target = QuickCraftTarget(carried, stack.count);
        if (target > MaxStack(carried))
        {
            target = MaxStack(carried);
            countText = TextFormat("%d", target);
        }
        stack = MakeStack(carried.block, target);
        GuiFill(x, y, x + 16, y + 16, (Color){ 255, 255, 255, 128 });
    }
    if (IsEmpty(stack))
    {
        DrawEmptyIcon(slot, x, y);
        return;
    }
    DrawStack(stack, x, y, countText, cappedCountColor);
}

static void DrawSlotFrames(void)
{
    for (int i = 0; i < slotCount; i++)
    {
        int x = leftPos + slots[i].x;
        int y = topPos + slots[i].y;

        if ((slots[i].kind == SLOT_RESULT) && (screen == SCREEN_CRAFTING)) GuiDrawSprite(GUI_SPRITE_SLOT_LARGE, x - 5, y - 5);
        else if (slots[i].kind == SLOT_DESTROY) GuiDrawSprite(GUI_SPRITE_DESTROY_SLOT, x - 1, y - 1);
        else GuiDrawSprite(GUI_SPRITE_SLOT, x - 1, y - 1);
    }
}

// The arrow between the grid and the output: a head that widens one pixel a
// row to the middle and a three-row shaft behind it.
static void DrawCraftArrow(int x, int headX, int y, int rows)
{
    Color color = { 139, 139, 139, 255 };
    int middle = rows/2;

    for (int k = 0; k < rows; k++)
    {
        int width = (k <= middle)? k + 1 : rows - k;

        GuiFill(headX, y + k, headX + width, y + k + 1, color);
        if ((k >= middle - 1) && (k <= middle + 1)) GuiFill(x, y + k, headX, y + k + 1, color);
    }
}

static void DrawSurvivalBackground(int mx, int my)
{
    GuiDrawPanel(leftPos, topPos, imageWidth, imageHeight);
    DrawSlotFrames();
    if (screen == SCREEN_INVENTORY)
    {
        DrawCraftArrow(leftPos + 135, leftPos + 144, topPos + 29, 13);
        GuiDrawInset(leftPos + 25, topPos + 7, 51, 72, BLACK);
        PlayerModelDraw(leftPos + 51, topPos + 75, 30, (float)(leftPos + 51 - mx), (float)(topPos + 75 - 50 - my),
            leftPos + 26, topPos + 8, leftPos + 75, topPos + 78);
    }
    else DrawCraftArrow(leftPos + 90, leftPos + 104, topPos + 35, 15);
}

static void DrawTab(int tab, bool selected)
{
    int x = leftPos + TabX(tab);
    int y = tabs[tab].top? topPos - 28 : topPos + CREATIVE_HEIGHT - 4;
    GuiSprite sprite = tabs[tab].top? GUI_SPRITE_TAB_TOP : GUI_SPRITE_TAB_BOTTOM;

    if (selected)
    {
        if (tabs[tab].top) sprite = (tabs[tab].column == 0)? GUI_SPRITE_TAB_TOP_SELECTED_FIRST :
            ((tabs[tab].column == 6)? GUI_SPRITE_TAB_TOP_SELECTED_LAST : GUI_SPRITE_TAB_TOP_SELECTED);
        else sprite = (tabs[tab].column == 0)? GUI_SPRITE_TAB_BOTTOM_SELECTED_FIRST :
            ((tabs[tab].column == 6)? GUI_SPRITE_TAB_BOTTOM_SELECTED_LAST : GUI_SPRITE_TAB_BOTTOM_SELECTED);
    }
    GuiDrawSprite(sprite, x, y);
    y += tabs[tab].top? 9 : 7;
    if (tabs[tab].iconBlock != BLOCK_AIR) GuiDrawItem(tabs[tab].iconBlock, x + 5, y);
    else if (tabs[tab].iconTexture != NULL) GuiDrawItemTexture(tabs[tab].iconTexture, x + 5, y);
}

// The search box frame is an inset like a slot with a softer top-left edge.
static void DrawSearchFrame(int x, int y, int w, int h)
{
    Color edge = { 85, 85, 85, 255 };
    Color fill = { 139, 139, 139, 255 };

    GuiFill(x, y, x + w - 1, y + 1, edge);
    GuiFill(x + w - 1, y, x + w, y + 1, fill);
    GuiFill(x, y + 1, x + 1, y + h - 1, edge);
    GuiFill(x + 1, y + 1, x + w - 1, y + h - 1, fill);
    GuiFill(x + w - 1, y + 1, x + w, y + h - 1, WHITE);
    GuiFill(x, y + h - 1, x + 1, y + h, fill);
    GuiFill(x + 1, y + h - 1, x + w, y + h, WHITE);
}

static void DrawCreativeBackground(const Player *player, int mx, int my)
{
    TabKind kind = CurrentTabKind(player);

    for (int tab = 0; tab < TAB_COUNT; tab++)
    {
        if (TabVisible(tab) && (tab != player->creativeTab)) DrawTab(tab, false);
    }
    GuiDrawPanel(leftPos, topPos, CREATIVE_WIDTH, CREATIVE_HEIGHT);
    DrawSlotFrames();
    if (kind == TAB_KIND_INVENTORY) GuiDrawInset(leftPos + 72, topPos + 5, 34, 45, BLACK);
    else
    {
        GuiDrawInset(leftPos + 174, topPos + 17, 14, 112, (Color){ 139, 139, 139, 255 });
        if (kind == TAB_KIND_SEARCH)
        {
            DrawSearchFrame(leftPos + 80, topPos + 4, 90, 12);
            TextFieldDraw(&searchField, leftPos + 82, topPos + 6, WHITE);
        }
        GuiDrawSprite(CanScroll(player)? GUI_SPRITE_SCROLLER : GUI_SPRITE_SCROLLER_DISABLED, leftPos + 175,
            topPos + 18 + (int)(95.0f*player->creativeScroll));
    }
    DrawTab(player->creativeTab, true);
    if (kind == TAB_KIND_INVENTORY)
    {
        PlayerModelDraw(leftPos + 88, topPos + 45, 20, (float)(leftPos + 88 - mx), (float)(topPos + 45 - 30 - my),
            leftPos + 73, topPos + 6, leftPos + 105, topPos + 49);
    }
}

static void DrawLabels(const Player *player)
{
    if (screen == SCREEN_INVENTORY) GuiDrawText("Crafting", leftPos + 97, topPos + 6, labelColor, false);
    else if (screen == SCREEN_CRAFTING)
    {
        GuiDrawText("Crafting", leftPos + 29, topPos + 6, labelColor, false);
        GuiDrawText("Inventory", leftPos + 8, topPos + imageHeight - 94, labelColor, false);
    }
    else if (CurrentTabKind(player) != TAB_KIND_INVENTORY)
    {
        GuiDrawText(tabs[player->creativeTab].name, leftPos + 8, topPos + 6, labelColor, false);
    }
}

static void DrawCarried(const Player *player, int mx, int my)
{
    Stack carried = Carried(player);

    if (IsEmpty(carried)) return;
    if (quickCrafting && (quickCraftCount > 1)) carried = MakeStack(carried.block, quickCraftRemainder);
    DrawStack(carried, mx - 8, my - 8, NULL, WHITE);
}

// Creative adds the tabs an item belongs to under its name, except on the
// items of the tab being browsed.
static void DrawItemTooltip(const Player *player, int hovered, int mx, int my)
{
    const char *lines[TAB_COUNT + 1] = { 0 };
    Color colors[TAB_COUNT + 1] = { 0 };
    bool italic[TAB_COUNT + 1] = { false };
    int count = 0;
    const MenuSlot *slot = NULL;
    Stack item = { 0 };

    if ((hovered < 0) || !IsEmpty(Carried(player))) return;
    slot = &slots[hovered];
    if ((slot->kind == SLOT_PICKER) && (PickerAt(player, slot->index).lockedRow >= 0))
    {
        lines[0] = TextFormat("Save hotbar with C+%d", PickerAt(player, slot->index).lockedRow + 1);
        colors[0] = WHITE;
        italic[0] = true;
        GuiDrawTooltipEx(lines, colors, italic, 1, mx, my);
        return;
    }
    item = GetSlot(player, slot);
    if (IsEmpty(item)) return;
    lines[count] = GetBlockName(item.block);
    colors[count] = WHITE;
    count++;
    if ((screen == SCREEN_CREATIVE) && !((slot->kind == SLOT_PICKER) && (CurrentTabKind(player) == TAB_KIND_CATEGORY)))
    {
        for (int tab = 0; tab < TAB_COUNT; tab++)
        {
            if ((tabs[tab].kind != TAB_KIND_CATEGORY) || !TabContains(tab, item.block)) continue;
            lines[count] = tabs[tab].name;
            colors[count] = tabNameColor;
            count++;
        }
    }
    GuiDrawTooltipEx(lines, colors, italic, count, mx, my);
}

static void DrawCreativeTooltips(const Player *player, int mx, int my)
{
    for (int tab = 0; tab < TAB_COUNT; tab++)
    {
        if (!TabVisible(tab) || !TabHovered(tab, mx, my)) continue;
        GuiDrawTooltip(&tabs[tab].name, NULL, 1, mx, my);
        break;
    }
    for (int i = 0; i < slotCount; i++)
    {
        const char *trash = "Destroy Item";

        if ((slots[i].kind == SLOT_DESTROY) && SlotHovered(i, mx, my)) GuiDrawTooltip(&trash, NULL, 1, mx, my);
    }
}

void DrawInventory(Player *player)
{
    Vector2 mouse = GuiMouse();
    int mx = (int)mouse.x;
    int my = (int)mouse.y;
    int hovered = SLOT_NONE;

    if (!player->inventoryOpen) return;
    BuildMenu(player);
    GuiBegin();
    GuiFillGradient(0, 0, GuiWidth(), GuiHeight(), (Color){ 16, 16, 16, 192 }, (Color){ 16, 16, 16, 208 });
    if (screen == SCREEN_CREATIVE) DrawCreativeBackground(player, mx, my);
    else DrawSurvivalBackground(mx, my);
    for (int i = 0; i < slotCount; i++)
    {
        DrawSlot(player, i);
        if (!SlotHovered(i, mx, my)) continue;
        hovered = i;
        GuiDrawSlotHighlight(leftPos + slots[i].x, topPos + slots[i].y);
    }
    DrawLabels(player);
    DrawCarried(player, mx, my);
    if (screen == SCREEN_CREATIVE) DrawCreativeTooltips(player, mx, my);
    DrawItemTooltip(player, hovered, mx, my);
    GuiEnd();
}
