/**********************************************************************************************
*
*   OpenCraft Multiplayer / Realms
*   Both are listed here so the menu can show them, and both stay unavailable.
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "menu_ui.h"

#include <stddef.h>

//----------------------------------------------------------------------------------
// Module Variables
//----------------------------------------------------------------------------------
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Local Functions
//----------------------------------------------------------------------------------
static void LayoutMultiplayer(MenuButton *buttons, bool realms)
{
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int buttonH = 20*scale;
    int y1 = GetScreenHeight() - 52*scale;
    int y2 = GetScreenHeight() - 28*scale;

    if (realms)
    {
        buttons[0].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y1, (float)(150*scale), (float)buttonH };
        buttons[0].label = "Join Realm";
        buttons[0].enabled = false;

        buttons[1].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y1, (float)(150*scale), (float)buttonH };
        buttons[1].label = "Cancel";
        buttons[1].enabled = true;

        buttons[2].enabled = false;
        buttons[2].label = NULL;
        buttons[2].bounds = (Rectangle){ 0, 0, 0, 0 };
        buttons[3] = buttons[2];
        buttons[4] = buttons[2];
        buttons[5] = buttons[2];
        return;
    }

    buttons[0].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y1, (float)(150*scale), (float)buttonH };
    buttons[0].label = "Join Server";
    buttons[0].enabled = false;

    buttons[1].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y1, (float)(150*scale), (float)buttonH };
    buttons[1].label = "Direct Connect";
    buttons[1].enabled = false;

    buttons[2].bounds = (Rectangle){ (float)(cx - 154*scale), (float)y2, (float)(72*scale), (float)buttonH };
    buttons[2].label = "Add Server";
    buttons[2].enabled = false;

    buttons[3].bounds = (Rectangle){ (float)(cx - 76*scale), (float)y2, (float)(72*scale), (float)buttonH };
    buttons[3].label = "Edit";
    buttons[3].enabled = false;

    buttons[4].bounds = (Rectangle){ (float)(cx + 4*scale), (float)y2, (float)(72*scale), (float)buttonH };
    buttons[4].label = "Refresh";
    buttons[4].enabled = false;

    buttons[5].bounds = (Rectangle){ (float)(cx + 82*scale), (float)y2, (float)(72*scale), (float)buttonH };
    buttons[5].label = "Cancel";
    buttons[5].enabled = true;
}

//----------------------------------------------------------------------------------
// Multiplayer Screen
//----------------------------------------------------------------------------------
void InitMultiplayerScreen(void)
{
    finishScreen = 0;
    EnableCursor();
}

void UpdateMultiplayerScreen(void)
{
    MenuButton buttons[6] = { 0 };
    bool realms = GetUnavailableMenu() == UNAVAILABLE_REALMS;

    LayoutMultiplayer(buttons, realms);

    for (int i = 0; i < 6; i++)
    {
        if (buttons[i].label == NULL) continue;
        UpdateMenuButton(&buttons[i]);
        if (buttons[i].clicked)
        {
            PlaySound(fxCoin);
            finishScreen = 1;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER))
    {
        PlaySound(fxCoin);
        finishScreen = 1;
    }
}

void DrawMultiplayerScreen(void)
{
    MenuButton buttons[6] = { 0 };
    bool realms = GetUnavailableMenu() == UNAVAILABLE_REALMS;
    int scale = MenuScale();
    int cx = GetScreenWidth()/2;
    int fontSize = 8*scale;
    int listTop = 48*scale;
    int listBottom = GetScreenHeight() - 64*scale;
    int mid = (listTop + listBottom)/2;
    const char *title = realms ? "Realms" : "Play Multiplayer";
    const char *detail = realms ? "Realms are unavailable for now." : "Multiplayer is unavailable for now.";
    Rectangle searchBox = { (float)(cx - 154*scale), (float)(22*scale), (float)(308*scale), (float)(20*scale) };

    LayoutMultiplayer(buttons, realms);
    for (int i = 0; i < 6; i++) buttons[i].hovered = CheckCollisionPointRec(GetMousePosition(), buttons[i].bounds);
    DrawDirtBackground();
    DrawMenuTextCentered(cx, 4*scale, fontSize, title, WHITE);

    if (!realms) DrawMenuTextField(searchBox, "", false, 0);

    DrawMenuTextCentered(cx, mid - fontSize, fontSize*2, "Coming soon", WHITE);
    DrawMenuTextCentered(cx, mid + fontSize, fontSize, detail, (Color){ 180, 180, 180, 255 });

    DrawRectangle(0, listTop, GetScreenWidth(), scale, (Color){ 0, 0, 0, 180 });
    DrawRectangle(0, GetScreenHeight() - 64*scale, GetScreenWidth(), scale, (Color){ 0, 0, 0, 180 });

    for (int i = 0; i < 6; i++)
    {
        if (buttons[i].label == NULL) continue;
        DrawStoneButton(&buttons[i], false);
    }
}

void UnloadMultiplayerScreen(void)
{
}

int FinishMultiplayerScreen(void)
{
    return finishScreen;
}
