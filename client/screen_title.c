/**********************************************************************************************
*
*   OpenCraft Title Screen
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "menu_ui.h"

#include <math.h>
#include <stddef.h>

//----------------------------------------------------------------------------------
// Module Variables
//----------------------------------------------------------------------------------
static int finishScreen = 0;
static int selection = 0;
static float noticeTimer = 0.0f;
static const char *notice = NULL;

static const char *splashes[] = {
    "Now with chunks!",
    "Open blocks!",
    "Also try digging!",
    "Built in C!",
    "Procedural!",
    "Watch your step!",
    "Hello, world!",
    "Infinite-ish!"
};

static int splashIndex = 0;

//----------------------------------------------------------------------------------
// Local Functions
//----------------------------------------------------------------------------------
static void LayoutTitle(MenuButton *buttons)
{
    int scale = MenuScale();
    int guiH = GetScreenHeight()/scale;
    int cx = GetScreenWidth()/2;
    int top = (guiH/4 + 48)*scale;
    int bottom = top + (72 + 12)*scale;
    int buttonH = 20*scale;

    buttons[0].bounds = (Rectangle){ (float)(cx - 100*scale), (float)top, (float)(200*scale), (float)buttonH };
    buttons[0].label = "Singleplayer";
    buttons[0].enabled = true;

    buttons[1].bounds = (Rectangle){ (float)(cx - 100*scale), (float)(top + 24*scale), (float)(200*scale), (float)buttonH };
    buttons[1].label = "Multiplayer";
    buttons[1].enabled = true;

    buttons[2].bounds = (Rectangle){ (float)(cx - 100*scale), (float)(top + 48*scale), (float)(200*scale), (float)buttonH };
    buttons[2].label = "Realms";
    buttons[2].enabled = true;

    buttons[3].bounds = (Rectangle){ (float)(cx - 100*scale), (float)bottom, (float)(98*scale), (float)buttonH };
    buttons[3].label = "Options...";
    buttons[3].enabled = true;

    buttons[4].bounds = (Rectangle){ (float)(cx + 2*scale), (float)bottom, (float)(98*scale), (float)buttonH };
    buttons[4].label = "Quit Game";
    buttons[4].enabled = true;

    buttons[5].bounds = (Rectangle){ (float)(cx - 124*scale), (float)bottom, (float)(20*scale), (float)buttonH };
    buttons[5].label = "";
    buttons[5].enabled = true;

    buttons[6].bounds = (Rectangle){ (float)(cx + 104*scale), (float)bottom, (float)(20*scale), (float)buttonH };
    buttons[6].label = "";
    buttons[6].enabled = true;
}

static void ActivateTitleButton(int index)
{
    PlaySound(fxCoin);

    switch (index)
    {
        case 0: finishScreen = 3; break;
        case 1:
            SetUnavailableMenu(UNAVAILABLE_MULTIPLAYER);
            finishScreen = 4;
            break;
        case 2:
            SetUnavailableMenu(UNAVAILABLE_REALMS);
            finishScreen = 4;
            break;
        case 3: finishScreen = 1; break;
        case 4: finishScreen = 5; break;
        case 5:
            notice = "Languages are coming soon.";
            noticeTimer = 2.5f;
            break;
        case 6:
            notice = "Accessibility options are coming soon.";
            noticeTimer = 2.5f;
            break;
        default: break;
    }
}

static void DrawSplash(Rectangle logo)
{
    int scale = MenuScale();
    const char *text = splashes[splashIndex];
    float pulse = 1.0f + 0.08f*sinf((float)GetTime()*6.0f);
    float fontSize = 12.0f*scale*pulse;
    Vector2 size = MeasureTextEx(GetMenuFont(), text, fontSize, 0.0f);
    Vector2 origin = { size.x, size.y*0.5f };
    Vector2 pos = { (float)GetScreenWidth() - 12.0f*scale, logo.y + logo.height*0.55f };

    DrawTextPro(GetMenuFont(), text, (Vector2){ pos.x + scale, pos.y + scale }, origin, -20.0f, fontSize, 0.0f, (Color){ 40, 40, 0, 180 });
    DrawTextPro(GetMenuFont(), text, pos, origin, -20.0f, fontSize, 0.0f, YELLOW);
}

//----------------------------------------------------------------------------------
// Title Screen
//----------------------------------------------------------------------------------
void InitTitleScreen(void)
{
    finishScreen = 0;
    selection = 0;
    noticeTimer = 0.0f;
    notice = NULL;
    splashIndex = (int)(GetTime()*10.0)%8;
    if (splashIndex < 0) splashIndex = 0;
    EnableCursor();
}

void UpdateTitleScreen(void)
{
    MenuButton buttons[7] = { 0 };

    UpdateMenuUi();
    LayoutTitle(buttons);

    if (noticeTimer > 0.0f) noticeTimer -= GetFrameTime();

    for (int i = 0; i < 7; i++)
    {
        UpdateMenuButton(&buttons[i]);
        if (buttons[i].hovered && (i < 5)) selection = i;
        if (buttons[i].clicked) ActivateTitleButton(i);
    }

    if (IsKeyPressed(KEY_UP))
    {
        selection--;
        if (selection < 0) selection = 4;
    }

    if (IsKeyPressed(KEY_DOWN))
    {
        selection++;
        if (selection > 4) selection = 0;
    }

    if (IsKeyPressed(KEY_ENTER)) ActivateTitleButton(selection);
    if (IsKeyPressed(KEY_ESCAPE)) ActivateTitleButton(4);
}

void DrawTitleScreen(void)
{
    MenuButton buttons[7] = { 0 };
    int scale = MenuScale();
    int guiH = GetScreenHeight()/scale;
    int buttonTop = (guiH/4 + 48)*scale;
    int cell = 6*scale;
    int mainH = 7*cell;
    int editionCell = 3*scale;
    int editionH = 0;
    int topY = 0;
    Rectangle logo = { 0 };
    const char *version = "OpenCraft " OPENCRAFT_VERSION;
    const char *credit = "Textures by Acaitart";
    int fontSize = 8*scale;

    if (editionCell < 4) editionCell = 4;
    editionH = 7*editionCell;
    topY = buttonTop - 8*scale - (mainH + 4*scale + editionH);
    if (topY < cell) topY = cell;

    LayoutTitle(buttons);
    for (int i = 0; i < 7; i++) buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
    DrawPanoramaBackground();
    logo = DrawOpenCraftLogo(GetScreenWidth()/2, topY, scale);
    DrawSplash(logo);

    for (int i = 0; i < 5; i++) DrawStoneButton(&buttons[i], i == selection);
    DrawLanguageButton(&buttons[5], false);
    DrawAccessibilityButton(&buttons[6], false);

    DrawMenuText(2*scale, GetScreenHeight() - (fontSize + 2*scale), fontSize, version, (Color){ 224, 224, 224, 255 });
    DrawMenuText(GetScreenWidth() - MenuTextWidth(credit, fontSize) - 2*scale, GetScreenHeight() - (fontSize + 2*scale), fontSize, credit, (Color){ 224, 224, 224, 255 });

    if ((notice != NULL) && (noticeTimer > 0.0f))
    {
        DrawMenuTextCentered(GetScreenWidth()/2, GetScreenHeight() - (fontSize*3 + 4*scale), fontSize, notice, YELLOW);
    }
}

void UnloadTitleScreen(void)
{
}

int FinishTitleScreen(void)
{
    return finishScreen;
}
