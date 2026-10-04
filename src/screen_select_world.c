/**********************************************************************************************
*
*   OpenCraft World Select
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "menu_ui.h"
#include "world_catalog.h"

#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Module Variables
//----------------------------------------------------------------------------------
static int finishScreen = 0;
static int framesCounter = 0;
static int selectedId = -1;
static int scroll = 0;
static int mode = 0; // 0 browse, 1 create, 2 edit, 3 delete, 4 recreate
static bool searchFocused = false;
static char search[WORLD_NAME_LENGTH] = "";
static char nameField[WORLD_NAME_LENGTH] = "";
static char seedField[12] = "";
static bool nameFocused = true;
static int visible[MAX_WORLDS] = { 0 };
static int visibleCount = 0;
static Texture2D icons[MAX_WORLDS] = { 0 };

//----------------------------------------------------------------------------------
// Local Functions
//----------------------------------------------------------------------------------
static bool ContainsText(const char *haystack, const char *needle)
{
    int hayLen = 0;
    int needleLen = 0;

    if ((needle == NULL) || (needle[0] == '\0')) return true;
    if (haystack == NULL) return false;

    hayLen = (int)strlen(haystack);
    needleLen = (int)strlen(needle);

    for (int i = 0; i <= hayLen - needleLen; i++)
    {
        int j = 0;
        while ((j < needleLen) && ((haystack[i + j] | 32) == (needle[j] | 32))) j++;
        if (j == needleLen) return true;
    }

    return false;
}

static void RebuildFilter(void)
{
    visibleCount = 0;

    for (int i = 0; i < GetWorldCount(); i++)
    {
        const WorldInfo *world = GetWorld(i);
        if ((world != NULL) && ContainsText(world->name, search)) visible[visibleCount++] = i;
    }
}

static Texture2D MakeWorldIcon(unsigned int seed)
{
    Image image = GenImageColor(32, 32, (Color){ 120, 176, 214, 255 });
    int ground = 14 + (int)(seed%6);
    int trunkX = 6 + (int)((seed/7)%14);
    Texture2D texture = { 0 };

    ImageDrawRectangle(&image, 0, ground, 32, 32 - ground, (Color){ 92, 148, 64, 255 });
    if (ground + 5 < 32) ImageDrawRectangle(&image, 0, ground + 5, 32, 32 - (ground + 5), (Color){ 134, 96, 67, 255 });

    if ((seed%3) != 0)
    {
        ImageDrawRectangle(&image, trunkX, ground - 8, 3, 8, (Color){ 96, 64, 32, 255 });
        ImageDrawRectangle(&image, trunkX - 4, ground - 14, 11, 7, (Color){ 54, 122, 46, 255 });
    }

    if ((seed%4) == 0) ImageDrawRectangle(&image, 0, 20, 12, 12, (Color){ 48, 92, 196, 255 });

    texture = LoadTextureFromImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    UnloadImage(image);
    return texture;
}

static void RebuildIcons(void)
{
    int count = GetWorldCount();

    for (int i = 0; i < MAX_WORLDS; i++)
    {
        if (icons[i].id != 0) UnloadTexture(icons[i]);
        icons[i] = (Texture2D){ 0 };
    }

    for (int i = 0; i < count; i++)
    {
        const WorldInfo *world = GetWorld(i);
        if (world != NULL) icons[i] = MakeWorldIcon(world->seed);
    }
}

static int SelectedVisibleRow(void)
{
    for (int i = 0; i < visibleCount; i++)
    {
        const WorldInfo *world = GetWorld(visible[i]);
        if ((world != NULL) && (world->id == selectedId)) return i;
    }

    return -1;
}

static void TrimField(char *text)
{
    int start = 0;
    int end = (int)strlen(text);

    while ((text[start] == ' ') && (text[start] != '\0')) start++;
    while ((end > start) && (text[end - 1] == ' ')) end--;

    if (start > 0)
    {
        int j = 0;
        for (int i = start; i < end; i++) text[j++] = text[i];
        text[j] = '\0';
    }
    else text[end] = '\0';
}

static void SuggestName(char *out, int outSize)
{
    char candidate[WORLD_NAME_LENGTH] = "New World";

    for (int n = 1; n < 100; n++)
    {
        bool taken = false;

        if (n > 1) snprintf(candidate, sizeof(candidate), "New World %d", n);

        for (int i = 0; i < GetWorldCount(); i++)
        {
            const WorldInfo *world = GetWorld(i);
            if ((world != NULL) && (strcmp(world->name, candidate) == 0)) taken = true;
        }

        if (!taken)
        {
            snprintf(out, outSize, "%s", candidate);
            return;
        }
    }

    snprintf(out, outSize, "New World");
}

static bool SeedWouldOverflow(const char *text, int digit)
{
    unsigned long value = 0;

    for (int i = 0; text[i] != '\0'; i++)
    {
        value = value*10ul + (unsigned long)(text[i] - '0');
    }

    value = value*10ul + (unsigned long)digit;
    return value > 4294967295ul;
}

static void TypeDigits(char *text, int cap)
{
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
    {
        int length = (int)strlen(text);
        if (length > 0) text[length - 1] = '\0';
    }

    int ch = GetCharPressed();
    while (ch > 0)
    {
        int length = (int)strlen(text);
        if ((ch >= '0') && (ch <= '9') && (length < cap - 1) && !SeedWouldOverflow(text, ch - '0'))
        {
            text[length] = (char)ch;
            text[length + 1] = '\0';
        }
        ch = GetCharPressed();
    }
}

static bool ParseSeedField(const char *text, unsigned int *out)
{
    unsigned long value = 0;

    if ((text == NULL) || (text[0] == '\0') || (out == NULL)) return false;

    for (int i = 0; text[i] != '\0'; i++)
    {
        if ((text[i] < '0') || (text[i] > '9')) return false;
        value = value*10ul + (unsigned long)(text[i] - '0');
        if (value > 4294967295ul) return false;
    }

    *out = (unsigned int)value;
    return true;
}

static int DialogTitleY(void)
{
    int scale = MenuScale();

    if (mode == 1) return GetScreenHeight()/2 - 96*scale;
    return GetScreenHeight()/2 - 48*scale;
}

static Rectangle DialogNameField(void)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int y = DialogTitleY();

    return (Rectangle){ (float)(cx - 154*scale), (float)(y + 30*scale), (float)(308*scale), (float)(20*scale) };
}

static Rectangle DialogSeedField(void)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int y = DialogTitleY();

    return (Rectangle){ (float)(cx - 154*scale), (float)(y + 80*scale), (float)(308*scale), (float)(20*scale) };
}

static void TypeInto(char *text, int cap)
{
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
    {
        int length = (int)strlen(text);
        if (length > 0) text[length - 1] = '\0';
    }

    int ch = GetCharPressed();
    while (ch > 0)
    {
        int length = (int)strlen(text);
        if ((ch >= 32) && (ch < 127) && (length < cap - 1))
        {
            text[length] = (char)ch;
            text[length + 1] = '\0';
        }
        ch = GetCharPressed();
    }
}

static void DrainChars(void)
{
    while (GetCharPressed() > 0) { }
}

static void PlaySelected(void)
{
    if (SelectedVisibleRow() < 0) return;
    SetActiveWorldId(selectedId);
    PlaySound(fxCoin);
    finishScreen = 2;
}

static void LayoutBrowse(MenuButton *buttons, bool hasSelection)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int buttonH = 20*scale;
    int y1 = GetScreenHeight() - 52*scale;
    int y2 = GetScreenHeight() - 28*scale;
    const char *labels[] = {
        "Play Selected World",
        "Create New World",
        "Edit",
        "Delete",
        "Re-Create",
        "Cancel"
    };
    int xs[] = { cx - 154*scale, cx + 4*scale, cx - 154*scale, cx - 76*scale, cx + 4*scale, cx + 82*scale };
    int ws[] = { 150*scale, 150*scale, 72*scale, 72*scale, 72*scale, 72*scale };
    int ys[] = { y1, y1, y2, y2, y2, y2 };
    bool enabled[] = { hasSelection, true, hasSelection, hasSelection, hasSelection, true };

    for (int i = 0; i < 6; i++)
    {
        buttons[i].bounds = (Rectangle){ (float)xs[i], (float)ys[i], (float)ws[i], (float)buttonH };
        buttons[i].label = labels[i];
        buttons[i].enabled = enabled[i];
    }
}

static Rectangle ListBounds(void)
{
    int scale = MenuScale();
    return (Rectangle){ 0, (float)(48*scale), (float)GetScreenWidth(), (float)(GetScreenHeight() - (48 + 64)*scale) };
}

static void UpdateBrowse(void)
{
    MenuButton buttons[6] = { 0 };
    Rectangle list = ListBounds();
    Rectangle searchBox = { 0 };
    int scale = MenuScale();
    int rowH = 36*scale;
    int cx = GetScreenWidth()/2;
    bool hasSelection = SelectedVisibleRow() >= 0;
    Vector2 mouse = GetMousePosition();

    searchBox = (Rectangle){ (float)(cx - 154*scale), (float)(22*scale), (float)(308*scale), (float)(20*scale) };
    LayoutBrowse(buttons, hasSelection);

    for (int i = 0; i < 6; i++) UpdateMenuButton(&buttons[i]);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        searchFocused = CheckCollisionPointRec(mouse, searchBox);
    }

    if (CheckCollisionPointRec(mouse, list))
    {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) scroll -= (int)(wheel*rowH);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            int row = (int)((mouse.y - list.y + scroll)/rowH);
            if ((row >= 0) && (row < visibleCount))
            {
                const WorldInfo *world = GetWorld(visible[row]);
                if (world != NULL)
                {
                    static int lastRow = -1;
                    static double lastTime = 0.0;
                    if ((lastRow == row) && (GetTime() - lastTime < 0.35)) PlaySelected();
                    selectedId = world->id;
                    lastRow = row;
                    lastTime = GetTime();
                }
            }
        }
    }

    if (searchFocused) TypeInto(search, WORLD_NAME_LENGTH);
    else DrainChars();

    if (IsKeyPressed(KEY_UP))
    {
        int row = SelectedVisibleRow();
        if (row > 0)
        {
            const WorldInfo *world = GetWorld(visible[row - 1]);
            if (world != NULL) selectedId = world->id;
        }
    }

    if (IsKeyPressed(KEY_DOWN))
    {
        int row = SelectedVisibleRow();
        if ((row >= 0) && (row + 1 < visibleCount))
        {
            const WorldInfo *world = GetWorld(visible[row + 1]);
            if (world != NULL) selectedId = world->id;
        }
        else if ((row < 0) && (visibleCount > 0))
        {
            const WorldInfo *world = GetWorld(visible[0]);
            if (world != NULL) selectedId = world->id;
        }
    }

    RebuildFilter();

    if (buttons[0].clicked || IsKeyPressed(KEY_ENTER)) PlaySelected();
    if (buttons[1].clicked)
    {
        SuggestName(nameField, sizeof(nameField));
        seedField[0] = '\0';
        nameFocused = true;
        mode = 1;
        searchFocused = false;
        PlaySound(fxCoin);
    }
    if (buttons[2].clicked && hasSelection)
    {
        const WorldInfo *world = NULL;
        int row = SelectedVisibleRow();
        if (row >= 0) world = GetWorld(visible[row]);
        if (world != NULL)
        {
            snprintf(nameField, sizeof(nameField), "%s", world->name);
            mode = 2;
            PlaySound(fxCoin);
        }
    }
    if (buttons[3].clicked && hasSelection)
    {
        mode = 3;
        PlaySound(fxCoin);
    }
    if (buttons[4].clicked && hasSelection)
    {
        mode = 4;
        PlaySound(fxCoin);
    }
    if (buttons[5].clicked || IsKeyPressed(KEY_ESCAPE))
    {
        if (searchFocused && (search[0] != '\0') && IsKeyPressed(KEY_ESCAPE))
        {
            search[0] = '\0';
            searchFocused = false;
        }
        else
        {
            PlaySound(fxCoin);
            finishScreen = 1;
        }
    }

    int contentH = visibleCount*rowH;
    int viewH = (int)list.height;
    int maxScroll = contentH - viewH;
    int row = SelectedVisibleRow();

    if (maxScroll < 0) maxScroll = 0;
    if ((row >= 0) && (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)))
    {
        int rowTop = row*rowH;
        if (rowTop < scroll) scroll = rowTop;
        if (rowTop + rowH > scroll + viewH) scroll = rowTop + rowH - viewH;
    }
    if (scroll < 0) scroll = 0;
    if (scroll > maxScroll) scroll = maxScroll;
}

static void DrawBrowse(void)
{
    MenuButton buttons[6] = { 0 };
    Rectangle list = ListBounds();
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int fontSize = 8*scale;
    int rowH = 36*scale;
    int line = 10*scale;
    bool hasSelection = SelectedVisibleRow() >= 0;
    Rectangle searchBox = { (float)(cx - 154*scale), (float)(22*scale), (float)(308*scale), (float)(20*scale) };

    LayoutBrowse(buttons, hasSelection);
    for (int i = 0; i < 6; i++) buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
    DrawMenuTextCentered(cx, 4*scale, fontSize, "Select World", WHITE);
    DrawMenuTextField(searchBox, search, searchFocused, framesCounter);

    BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
        for (int row = 0; row < visibleCount; row++)
        {
            const WorldInfo *world = GetWorld(visible[row]);
            float y = list.y + row*rowH - scroll;
            Rectangle rowRect = { 4.0f*scale, y, list.width - 8.0f*scale, (float)rowH };
            char line2[96] = { 0 };
            char line3[96] = { 0 };

            if (world == NULL) continue;
            if (y + rowH < list.y) continue;
            if (y > list.y + list.height) break;

            if (world->id == selectedId)
            {
                DrawRectangleRec(rowRect, (Color){ 255, 255, 255, 28 });
                DrawRectangleLinesEx(rowRect, (float)scale, WHITE);
            }

            if (icons[visible[row]].id != 0)
            {
                Rectangle icon = { rowRect.x + 2.0f*scale, y + 2.0f*scale, 32.0f*scale, 32.0f*scale };
                DrawTexturePro(icons[visible[row]], (Rectangle){ 0, 0, 32, 32 }, icon, (Vector2){ 0, 0 }, 0.0f, WHITE);
                DrawRectangleLinesEx(icon, 1.0f, BLACK);
            }

            snprintf(line2, sizeof(line2), "%s (%s)", world->name, world->createdText);
            snprintf(line3, sizeof(line3), "%s Mode, Seed: %u", world->mode, world->seed);
            DrawMenuText((int)(rowRect.x + 38*scale), (int)(y + (rowH - line*3)/2), fontSize, world->name, WHITE);
            DrawMenuText((int)(rowRect.x + 38*scale), (int)(y + (rowH - line*3)/2 + line), fontSize, line2, (Color){ 160, 160, 160, 255 });
            DrawMenuText((int)(rowRect.x + 38*scale), (int)(y + (rowH - line*3)/2 + line*2), fontSize, line3, (Color){ 160, 160, 160, 255 });
        }
    EndScissorMode();

    DrawRectangle(0, (int)list.y, GetScreenWidth(), scale, (Color){ 0, 0, 0, 180 });
    DrawRectangle(0, GetScreenHeight() - 64*scale, GetScreenWidth(), scale, (Color){ 0, 0, 0, 180 });

    for (int i = 0; i < 6; i++) DrawStoneButton(&buttons[i], false);
}

static void LayoutDialog(MenuButton *buttons, const char *confirmLabel, bool confirmEnabled)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int y = GetScreenHeight()/2 + ((mode == 1) ? 32 : 24)*scale;
    int buttonH = 20*scale;

    buttons[0].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y, (float)(150*scale), (float)buttonH };
    buttons[0].label = confirmLabel;
    buttons[0].enabled = confirmEnabled;

    buttons[1].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y, (float)(150*scale), (float)buttonH };
    buttons[1].label = "Cancel";
    buttons[1].enabled = true;
}

static void UpdateDialog(void)
{
    MenuButton buttons[2] = { 0 };
    bool naming = (mode == 1) || (mode == 2);
    bool canConfirm = true;

    if (naming)
    {
        if (mode == 1)
        {
            if (IsKeyPressed(KEY_TAB)) nameFocused = !nameFocused;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, DialogNameField())) nameFocused = true;
                if (CheckCollisionPointRec(mouse, DialogSeedField())) nameFocused = false;
            }
            if (nameFocused) TypeInto(nameField, WORLD_NAME_LENGTH);
            else TypeDigits(seedField, (int)sizeof(seedField));
        }
        else TypeInto(nameField, WORLD_NAME_LENGTH);
        canConfirm = nameField[0] != '\0';
    }
    else DrainChars();

    LayoutDialog(buttons, (mode == 1) ? "Create" : (mode == 2) ? "Save" : (mode == 3) ? "Delete" : "Re-Create", canConfirm);
    UpdateMenuButton(&buttons[0]);
    UpdateMenuButton(&buttons[1]);

    if (buttons[1].clicked || IsKeyPressed(KEY_ESCAPE))
    {
        mode = 0;
        PlaySound(fxCoin);
        return;
    }

    if ((buttons[0].clicked || IsKeyPressed(KEY_ENTER)) && canConfirm)
    {
        if (mode == 1)
        {
            int id = 0;
            unsigned int seed = 0;
            bool chooseSeed = false;

            TrimField(nameField);
            TrimField(seedField);
            chooseSeed = ParseSeedField(seedField, &seed);
            id = CreateWorldRecord(nameField, OPENCRAFT_VERSION, seed, chooseSeed);
            if (id > 0)
            {
                SetActiveWorldId(id);
                PlaySound(fxCoin);
                finishScreen = 2;
            }
        }
        else if (mode == 2)
        {
            TrimField(nameField);
            if (RenameWorldRecord(selectedId, nameField))
            {
                PlaySound(fxCoin);
                mode = 0;
                RebuildFilter();
            }
        }
        else if (mode == 3)
        {
            DeleteWorldRecord(selectedId);
            RebuildIcons();
            RebuildFilter();
            selectedId = (visibleCount > 0) ? GetWorld(visible[0])->id : -1;
            PlaySound(fxCoin);
            mode = 0;
        }
        else if (mode == 4)
        {
            RecreateWorldRecord(selectedId);
            RebuildIcons();
            PlaySound(fxCoin);
            mode = 0;
        }
    }
}

static void DrawDialog(void)
{
    MenuButton buttons[2] = { 0 };
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int fontSize = 8*scale;
    int y = DialogTitleY();
    bool naming = (mode == 1) || (mode == 2);
    bool canConfirm = !naming || (nameField[0] != '\0');
    const char *title = "Create New World";
    const char *confirm = "Create";

    if (mode == 2) { title = "Edit World"; confirm = "Save"; }
    if (mode == 3) { title = "Are you sure you want to delete this world?"; confirm = "Delete"; }
    if (mode == 4) { title = "Re-create this world?"; confirm = "Re-Create"; }

    LayoutDialog(buttons, confirm, canConfirm);
    buttons[0].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[0].bounds);
    buttons[1].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[1].bounds);
    DrawMenuTextCentered(cx, y, fontSize, title, WHITE);

    if (naming)
    {
        Rectangle field = DialogNameField();
        DrawMenuTextCentered(cx, y + 16*scale, fontSize, "World Name", (Color){ 160, 160, 160, 255 });
        DrawMenuTextField(field, nameField, (mode != 1) || nameFocused, framesCounter);
        if (mode == 1)
        {
            Rectangle seedBox = DialogSeedField();
            DrawMenuTextCentered(cx, y + 56*scale, fontSize, "Seed", (Color){ 160, 160, 160, 255 });
            DrawMenuTextCentered(cx, y + 68*scale, fontSize, "Leave blank for a random seed.", (Color){ 160, 160, 160, 255 });
            DrawMenuTextField(seedBox, seedField, !nameFocused, framesCounter);
        }
    }
    else if (mode == 3)
    {
        int row = SelectedVisibleRow();
        const WorldInfo *world = (row >= 0) ? GetWorld(visible[row]) : NULL;
        if (world != NULL) DrawMenuTextCentered(cx, y + 20*scale, fontSize, world->name, YELLOW);
    }
    else if (mode == 4)
    {
        DrawMenuTextCentered(cx, y + 20*scale, fontSize, "This resets the seed and the created time.", (Color){ 160, 160, 160, 255 });
    }

    DrawStoneButton(&buttons[0], false);
    DrawStoneButton(&buttons[1], false);
}

//----------------------------------------------------------------------------------
// Select World Screen
//----------------------------------------------------------------------------------
void InitSelectWorldScreen(void)
{
    finishScreen = 0;
    framesCounter = 0;
    scroll = 0;
    mode = 0;
    searchFocused = false;
    search[0] = '\0';
    nameField[0] = '\0';
    seedField[0] = '\0';
    nameFocused = true;
    LoadWorldCatalog(OPENCRAFT_VERSION);
    RebuildIcons();
    RebuildFilter();
    selectedId = (visibleCount > 0) ? GetWorld(visible[0])->id : -1;
    EnableCursor();
}

void UpdateSelectWorldScreen(void)
{
    framesCounter++;
    if (mode == 0) UpdateBrowse();
    else UpdateDialog();
}

void DrawSelectWorldScreen(void)
{
    DrawDirtBackground();
    if (mode == 0) DrawBrowse();
    else DrawDialog();
}

void UnloadSelectWorldScreen(void)
{
    for (int i = 0; i < MAX_WORLDS; i++)
    {
        if (icons[i].id != 0) UnloadTexture(icons[i]);
        icons[i] = (Texture2D){ 0 };
    }
}

int FinishSelectWorldScreen(void)
{
    return finishScreen;
}
